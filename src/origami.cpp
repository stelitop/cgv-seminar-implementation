#pragma once
#include "origami.h"
#include <fstream>
#include "../external_code/third_party/json/single_include/nlohmann/json.hpp"
#include <iostream>
#include <glm/gtc/type_ptr.hpp>
#include "origamiexception.h"
#include <cmath>
#include <corecrt_math_defines.h>
#include <framework/ray.h>
#include "settings.h"

using json = nlohmann::json;

Origami::Origami() {

}

Origami Origami::loadFromFile(std::filesystem::path filePath) {
	std::ifstream f(filePath);		
	json data = json::parse(f);	

	Origami origami = Origami();

	if (data.contains("frame_title")) {
		origami.name = data["frame_title"];
	}
	else {
		origami.name = filePath.string();
	}

	// load vertices and initial velocities
	for (json vert : data["vertices_coords"]) {		
		origami.vertices.push_back(VertexData(glm::vec3(vert[0], vert[1], vert[2]), glm::vec3(0), glm::vec3(0)));
	}

	origami.normalizeVertices();

	// load edges(creases)
	for (size_t i = 0; i < data["edges_vertices"].size(); i++) {
		int crease_type = 0;
		if (data["edges_assignment"][i] == "M") {
			crease_type = MOUNTAIN_EDGE;
		}
		else if (data["edges_assignment"][i] == "V") {
			crease_type = VALLEY_EDGE;
		} 
		else if (data["edges_assignment"][i] == "B") {
			crease_type = BOUNDARY_EDGE;
		}
		else if (data["edges_assignment"][i] == "F") {
			crease_type = FACET_EDGE;
		}
		else {
			continue;
		}
		origami.edges.push_back(glm::uvec3(data["edges_vertices"][i][0], data["edges_vertices"][i][1], crease_type));
	}
	
	// load and triangulate faces
	for (json face : data["faces_vertices"]) {
		std::vector<unsigned int> face_verts;
		for (unsigned int vert_index : face) {
			face_verts.push_back(vert_index);
		}
		origami.triangulate(face_verts);
	}

	// remember nominal angles
	for (glm::uvec3 face : origami.faces) {
		origami.nominal_angles.push_back(origami.angles(face));
	}

	// precalculate nominal lengths and faces adjacent to each edge
	for (int i = 0; i < origami.edges.size(); i++) {
		// calculate nominal length
		origami.nominal_length.push_back(glm::length(origami.vertices[origami.edges[i].x].coords - origami.vertices[origami.edges[i].y].coords));

		// precompute adjacent faces
		unsigned int f1, f2;
		f1 = f2 = origami.faces.size();
		for (int j = 0; j < origami.faces.size(); j++) {
			// check that both vertices of then edge are vertices of then face
			if (
				(origami.edges[i].x == origami.faces[j].x || origami.edges[i].x == origami.faces[j].y || origami.edges[i].x == origami.faces[j].z) &&
				(origami.edges[i].y == origami.faces[j].x || origami.edges[i].y == origami.faces[j].y || origami.edges[i].y == origami.faces[j].z)
				) {
				if (f1 == origami.faces.size()) {
					f1 = j;
				}
				else if (f2 == origami.faces.size()) {
					f2 = j;
				}
				else {
					std::string msg = "Edge " + std::to_string(i) + " is adjacent to more than 2 faces.";
					throw OrigamiException(msg.c_str());
				}
			}
		}
		if (f2 == origami.faces.size()) {
			f2 = f1;
		}
		origami.edge_to_faces.push_back(glm::uvec2(f1, f2));
	}

	origami.calculateOptimalTimeStep();
	origami.prepareGpuMesh();

	return origami;
}

float areaOfTriangle(glm::vec3 p1, glm::vec3 p2, glm::vec3 p3) {
	return glm::length(glm::cross(p2 - p1, p3 - p1)) / 2.0f;
}

void Origami::triangulate(std::vector<unsigned int> verts) {
	if (verts.size() == 3) {
		this->faces.push_back(glm::uvec3(verts[0], verts[1], verts[2]));
		return;
	}
	if (verts.size() < 3) {
		throw "Face with less than 2 vertices found!";
	}
	glm::vec3 outside_point(0);
	for (unsigned int v : verts) {
		outside_point = glm::max(outside_point, this->vertices[v].coords);
	}
	outside_point += 1.0f;
	// try to find an ear of the polygon
	for (int i = 0; i < verts.size(); i++) {
		unsigned int v0 = verts[i];
		unsigned int v1 = verts[(i + 1) % verts.size()];
		unsigned int v2 = verts[(i + 2) % verts.size()];
		// try to connect v0 and v2
		// 1) the line must not cross any other lines <=> no other point should be inside v0-v1-v2
		// 2) the line must be inside of the polygon <=> the line's midpoint must be inside
		bool is_ear = true;
		// 1)
		float area = areaOfTriangle(this->vertices[v0].coords, this->vertices[v1].coords, this->vertices[v2].coords);
		for (int j = 0; j < verts.size() - 3; j++) {
			unsigned int vx = verts[(i + 3 + j) % verts.size()];
			float area_around_point = 
				areaOfTriangle(this->vertices[vx].coords, this->vertices[v1].coords, this->vertices[v2].coords) + 
				areaOfTriangle(this->vertices[v0].coords, this->vertices[vx].coords, this->vertices[v2].coords) + 
				areaOfTriangle(this->vertices[v0].coords, this->vertices[v1].coords, this->vertices[vx].coords);

			if (std::abs(area - area_around_point) <= 1.0e-4) {
				is_ear = false;
				break;
			}
		}
		if (!is_ear) {
			continue;
		}

		// 2)
		// ignore for now, not needed for convex polygons
		//glm::vec3 midpoint = (this->vertices[v0] + this->vertices[v2]) / 2.0f;

		// add a face and a facet crease and recursively apply the algorithm
		this->edges.push_back(glm::uvec3(v0, v2, FACET_EDGE));
		this->faces.push_back(glm::uvec3(v0, v1, v2));
		verts.erase(verts.begin() + (i + 1) % verts.size());
		this->triangulate(verts);
		return;
	}
	throw "Triangulation did not find an ear!";
}

void Origami::draw(const Shader& face_shader, const Shader& edge_shader, glm::mat4 mvpMatrix, Settings& settings) {

	// draw faces
	face_shader.bind();
	glUniformMatrix4fv(face_shader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
	glUniform1i(face_shader.getUniformLocation("renderMode"), settings.renderMode);
	glUniform1f(face_shader.getUniformLocation("magnitudeCutoff"), settings.magnitudeCutoff);

	glm::vec3 selectedPoint = getSelectedPoint(settings);
	glUniform1i(face_shader.getUniformLocation("useSelectedPoint"), settings.useSelectedPoint ? 1 : 0);
	glUniform3f(face_shader.getUniformLocation("selectedPoint"), selectedPoint.x, selectedPoint.y, selectedPoint.z);
	glUniform1f(face_shader.getUniformLocation("selectedPointRadius"), settings.selectedPointRadius);

	glBindVertexArray(m_vao_faces);
	glDrawElements(GL_TRIANGLES, 3*this->faces.size(), GL_UNSIGNED_INT, nullptr);

	//draw edges
	edge_shader.bind();
	glUniformMatrix4fv(edge_shader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
	glBindVertexArray(m_vao_edges);
	glDrawElements(GL_LINES, 2*this->edges.size(), GL_UNSIGNED_INT, nullptr);
}
//void Origami::draw(const Shader& face_shader, const Shader& edge_shader, glm::mat4 mvpMatrix, int renderMode, float magnitudeCutoff) {
//
//	// draw faces
//	face_shader.bind();
//	glUniformMatrix4fv(face_shader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
//	glUniform1i(face_shader.getUniformLocation("renderMode"), renderMode);
//	glUniform1f(face_shader.getUniformLocation("magnitudeCutoff"), magnitudeCutoff);
//	glBindVertexArray(m_vao_faces);
//	glDrawElements(GL_TRIANGLES, 3*this->faces.size(), GL_UNSIGNED_INT, nullptr);
//
//	//draw edges
//	edge_shader.bind();
//	glUniformMatrix4fv(edge_shader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
//	glBindVertexArray(m_vao_edges);
//	glDrawElements(GL_LINES, 2*this->edges.size(), GL_UNSIGNED_INT, nullptr);
//}

void Origami::updateVertexBuffers()
{
	glBindVertexArray(m_vao_faces);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_faces);

	auto formattedVertices = formatVertices();
	glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(formattedVertices.size() * sizeof(decltype(formattedVertices)::value_type)), formattedVertices.data(), GL_STATIC_DRAW);
	
	std::vector<glm::vec3> vertexDataEdgeShader;
	std::vector<glm::uvec3> faceDataEdgeShader;
	prepareEdgeShaderData(vertexDataEdgeShader, faceDataEdgeShader);
	std::vector<glm::vec3> only_vertices = getVertices();
	glBindVertexArray(m_vao_edges);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_edges);
	glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(only_vertices.size() * sizeof(decltype(only_vertices)::value_type)), only_vertices.data(), GL_STATIC_DRAW);
}

void Origami::normalizeVertices()
{
	glm::vec3 min = glm::vec3(std::numeric_limits<float>::max()), max = glm::vec3(std::numeric_limits<float>::min());
	for (int i = 0; i < vertices.size(); i++) {
		min = glm::min(min, vertices[i].coords);
		max = glm::max(max, vertices[i].coords);
	}
	float maxLength = std::max(max.x - min.x, std::max(max.y - min.y, max.z - min.z));
	min /= maxLength;
	max /= maxLength;
	for (int i = 0; i < vertices.size(); i++) {
		vertices[i].coords = vertices[i].coords / maxLength + (max - min) / 2.0f;
	}
}

void Origami::prepareGpuMesh() {

	// Create VAO and bind it so subsequent creations of VBO and IBO are bound to this VAO
	glGenVertexArrays(1, &m_vao_faces);
	glBindVertexArray(m_vao_faces);

	// Create vertex buffer object (VBO)
	glGenBuffers(1, &m_vbo_faces);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_faces);

	// Create index buffer object (IBO)
	glGenBuffers(1, &m_ibo_faces);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo_faces);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(this->faces.size() * sizeof(decltype(this->faces)::value_type)), this->faces.data(), GL_STATIC_DRAW);

	// We tell OpenGL what each vertex looks like and how they are mapped to the shader (location = ...).
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, coords));
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, force));
	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (void*)offsetof(VertexData, velocity));
	glVertexAttribDivisor(0, 0);
	glVertexAttribDivisor(1, 0);
	glVertexAttribDivisor(2, 0);

	std::vector<glm::uvec2> edges_gpu;
	for (auto e : this->edges) {
		edges_gpu.push_back(glm::vec2(e.x, e.y));
	}

	// Create VAO and bind it so subsequent creations of VBO and IBO are bound to this VAO
	glGenVertexArrays(1, &m_vao_edges);
	glBindVertexArray(m_vao_edges);

	// Create vertex buffer object (VBO)
	glGenBuffers(1, &m_vbo_edges);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_edges);

	// Create index buffer object (IBO)
	glGenBuffers(1, &m_ibo_edges);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo_edges);

	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(newfaces.size() * sizeof(decltype(newfaces)::value_type)), newfaces.data(), GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(edges_gpu.size() * sizeof(decltype(edges_gpu)::value_type)), edges_gpu.data(), GL_STATIC_DRAW);

	// We tell OpenGL what each vertex looks like and how they are mapped to the shader (location = ...).
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

	// update the data in the buffers
	updateVertexBuffers();
}

float Origami::cot(unsigned int p1, unsigned int p2, unsigned int p3)
{
	glm::vec3 a = vertices[p2].coords - vertices[p1].coords;
	glm::vec3 b = vertices[p3].coords - vertices[p1].coords;
	return glm::dot(a, b) / glm::length(glm::cross(a, b));
}

glm::vec3 Origami::angles(glm::uvec3 face)
{
	glm::vec3 vYX = glm::normalize(vertices[face.y].coords - vertices[face.x].coords);
	glm::vec3 vZX = glm::normalize(vertices[face.z].coords - vertices[face.x].coords);
	glm::vec3 vZY = glm::normalize(vertices[face.z].coords - vertices[face.y].coords);
	return glm::vec3(
		std::acos(std::clamp(glm::dot(vYX, vZX), -1.0f, 1.0f)),
		std::acos(std::clamp(glm::dot(-vYX, vZY), -1.0f, 1.0f)),
		std::acos(std::clamp(glm::dot(-vZX, -vZY), -1.0f, 1.0f)) // yes I can cancel the -, but this is clearer
	);
}

std::vector<Origami::VertexData> Origami::formatVertices()
{
	return vertices;
}

void Origami::prepareEdgeShaderData(std::vector<glm::vec3>& vertexData, std::vector<glm::uvec3>& faceData)
{
	vertexData.clear();
	faceData.clear();
	
	std::vector<glm::vec3> only_vertices = getVertices();
}

void Origami::updateNormals()
{
	normals.clear();
	for (int i = 0; i < faces.size(); i++) {
		normals.push_back(glm::normalize(glm::cross(vertices[faces[i].y].coords - vertices[faces[i].x].coords, vertices[faces[i].z].coords - vertices[faces[i].x].coords)));
	}
}

unsigned int opposite_vertex(glm::uvec3 face, glm::uvec3 edge)
{
	if (face.x != edge.x && face.x != edge.y) {
		return face.x;
	} else if (face.y != edge.x && face.y != edge.y) {
		return face.y;
	} else {
		return face.z;
	}
}

void Origami::step() {
	updateNormals();
	std::vector<glm::vec3> totalForce = getTotalForce();
	for (int i = 0; i < vertices.size(); i++) {
		vertices[i].force = totalForce[i];
		glm::vec3 a = totalForce[i];
		vertices[i].velocity += a * deltaT;
		vertices[i].coords += vertices[i].velocity * deltaT;
	}
	m_force_cache_used = false;
}

void Origami::calculateOptimalTimeStep()
{
	// calculate optimal time step
	float maxfreq = 0.0f;
	for (int i = 0; i < edges.size(); i++) {

		float k_axial = EA / nominal_length[i];
		float freq = std::sqrt(k_axial);
		if (freq > maxfreq) {
			maxfreq = freq;
		}
	}

	deltaT = 1.0f / (2.0f * M_PI * maxfreq);
}

std::vector<glm::vec3> Origami::axialConstraints()
{
	std::vector<glm::vec3> forces(this->vertices.size(), glm::vec3(0));

	for (int i = 0; i < edges.size(); i++) {
		float l = glm::length(vertices[edges[i].x].coords - vertices[edges[i].y].coords);
		glm::vec3 dldp1 = glm::normalize(vertices[edges[i].x].coords - vertices[edges[i].y].coords);
		glm::vec3 dldp2 = -dldp1;
		float k_axial = EA / nominal_length[i];
		forces[edges[i].x] -= k_axial * (l - nominal_length[i]) * dldp1;
		forces[edges[i].y] -= k_axial * (l - nominal_length[i]) * dldp2;
	}

	return forces;
}

std::vector<glm::vec3> Origami::creaseConstraints()
{
	std::vector<glm::vec3> forces(this->vertices.size(), glm::vec3(0));

	for (int i = 0; i < this->edges.size(); i++) {
		if (this->edges[i].z == BOUNDARY_EDGE) {
			continue;
		}
		const float k_crease = this->edges[i].z == FACET_EDGE ? nominal_length[i] * k_facet : nominal_length[i] * k_fold;
		// TODO: Update this so that it's controllable
		float theta_target = this->edges[i].z == FACET_EDGE ? 0 : (this->edges[i].z == MOUNTAIN_EDGE ? -M_PI : M_PI);
		theta_target *= target_angle_percent;

		unsigned int f1 = edge_to_faces[i].x;
		unsigned int f2 = edge_to_faces[i].y;
		unsigned int p1 = opposite_vertex(faces[f1], edges[i]);
		unsigned int p2 = opposite_vertex(faces[f2], edges[i]);
		unsigned int p3 = edges[i].x;
		unsigned int p4 = edges[i].y;

		glm::vec3 n1 = normals[f1];
		glm::vec3 n2 = normals[f2];
		float h1 = areaOfTriangle(vertices[faces[f1].x].coords, vertices[faces[f1].y].coords, vertices[faces[f1].z].coords) * 2.0f / glm::length(vertices[p4].coords - vertices[p3].coords);
		float h2 = areaOfTriangle(vertices[faces[f2].x].coords, vertices[faces[f2].y].coords, vertices[faces[f2].z].coords) * 2.0f / glm::length(vertices[p4].coords - vertices[p3].coords);
		glm::vec3 dthdp1 = n1 / h1;
		glm::vec3 dthdp2 = n2 / h2;
		glm::vec3 dthdp3 = -(cot(p4, p3, p1) / (cot(p3, p1, p4) + cot(p4, p3, p1))) * n1 / h1 - (cot(p4, p2, p3) / (cot(p3, p4, p2) + cot(p4, p2, p3))) * n2 / h2;
		glm::vec3 dthdp4 = -(cot(p3, p1, p4) / (cot(p3, p1, p4) + cot(p4, p3, p1))) * n1 / h1 - (cot(p3, p4, p2) / (cot(p3, p4, p2) + cot(p4, p2, p3))) * n2 / h2;

		glm::vec3 p1proj = vertices[p1].coords - (vertices[p3].coords + glm::normalize(vertices[p4].coords - vertices[p3].coords) * std::sqrtf(std::abs(std::pow(glm::length(vertices[p3].coords - vertices[p1].coords), 2) - h1*h1)));
		glm::vec3 p2proj = vertices[p2].coords - (vertices[p3].coords + glm::normalize(vertices[p4].coords - vertices[p3].coords) * std::sqrtf(std::abs(std::pow(glm::length(vertices[p3].coords - vertices[p2].coords), 2) - h2*h2)));
		
		
		//glm::vec3 creaseVector = glm::normalize(vertices[p4].coords - vertices[p3].coords);
		//float dotNormals = glm::dot(n1, n2);
		//float theta = std::atan2(glm::dot(glm::cross(n1, creaseVector), n2), dotNormals);
		float theta = std::acos(std::clamp(glm::dot(-p1proj, p2proj) / (h1 * h2), -1.0f, 1.0f));
		// flip orientation based on normals if needed
		if (glm::dot(n1, p1proj + p2proj) < 0) {
			theta *= -1.0f;
		}

		while (theta_target - theta > M_PI) {
			theta += 2 * M_PI;
		}
		while (theta - theta_target > M_PI) {
			theta -= 2 * M_PI;
		}

		// apply forces
		forces[p1] -= k_crease * (theta - theta_target) * dthdp1;
		forces[p2] -= k_crease * (theta - theta_target) * dthdp2;
		forces[p3] -= k_crease * (theta - theta_target) * dthdp3;
		forces[p4] -= k_crease * (theta - theta_target) * dthdp4;

		//std::cout << i << ": " << theta << " " << theta_target << std::endl;
		
		/*std::cout << "------------------" << std::endl;
		std::cout << vertices[p1].x << " " << vertices[p1].y << " " << vertices[p1].z << std::endl;
		std::cout << vertices[p3].x << " " << vertices[p3].y << " " << vertices[p3].z << std::endl;
		std::cout << vertices[p4].x << " " << vertices[p4].y << " " << vertices[p4].z << std::endl;
		std::cout << areaOfTriangle(vertices[faces[f1].x], vertices[faces[f1].y], vertices[faces[f1].z]) << std::endl;
		std::cout << p1 << " " << p2 << " " << p3 << " " << p4 << std::endl;
		std::cout << n1.x << " " << n1.y << " " << n1.z << std::endl;
		std::cout << n2.x << " " << n2.y << " " << n2.z << std::endl;
		std::cout << h1 << " " << h2 << std::endl;
		std::cout << p1proj.x << " " << p1proj.y << " " << p1proj.z << std::endl;
		std::cout << p2proj.x << " " << p2proj.y << " " << p2proj.z << std::endl;
		std::cout << theta << std::endl;
		std::cout << forces[p1].x << " " << forces[p1].y << " " << forces[p1].z << std::endl;
		std::cout << forces[p2].x << " " << forces[p2].y << " " << forces[p2].z << std::endl;
		std::cout << forces[p3].x << " " << forces[p3].y << " " << forces[p3].z << std::endl;
		std::cout << forces[p4].x << " " << forces[p4].y << " " << forces[p4].z << std::endl;*/
	}
	return forces;
}

std::vector<glm::vec3> Origami::faceConstraints()
{
	std::vector<glm::vec3> forces(this->vertices.size(), glm::vec3(0));

	for (int i = 0; i < faces.size(); i++) {
		glm::vec3 n = normals[i];
		glm::vec3 angles = this->angles(faces[i]);
		const unsigned int p1 = faces[i].x;
		const unsigned int p2 = faces[i].y;
		const unsigned int p3 = faces[i].z;
		glm::vec3 dp1a231 = glm::cross(n, vertices[p1].coords - vertices[p2].coords) / std::powf(glm::length(vertices[p1].coords - vertices[p2].coords), 2);
		glm::vec3 dp3a231 = -glm::cross(n, vertices[p3].coords - vertices[p2].coords) / std::powf(glm::length(vertices[p3].coords - vertices[p2].coords), 2);
		glm::vec3 dp2a231 = -dp1a231 - dp3a231;

		glm::vec3 dp2a312 = glm::cross(n, vertices[p2].coords - vertices[p3].coords) / std::powf(glm::length(vertices[p2].coords - vertices[p3].coords), 2);
		glm::vec3 dp1a312 = -glm::cross(n, vertices[p1].coords - vertices[p3].coords) / std::powf(glm::length(vertices[p1].coords - vertices[p3].coords), 2);
		glm::vec3 dp3a312 = -dp2a312 - dp1a312;

		glm::vec3 dp3a123 = glm::cross(n, vertices[p3].coords - vertices[p1].coords) / std::powf(glm::length(vertices[p3].coords - vertices[p1].coords), 2);
		glm::vec3 dp2a123 = -glm::cross(n, vertices[p2].coords - vertices[p1].coords) / std::powf(glm::length(vertices[p2].coords - vertices[p1].coords), 2);
		glm::vec3 dp1a123 = -dp3a123 - dp2a123;

		/*std::cout << "------------------" << std::endl;
		std::cout << dp1a231.x << " " << dp1a231.y << " " << dp1a231.z << std::endl;
		std::cout << dp3a231.x << " " << dp3a231.y << " " << dp3a231.z << std::endl;
		std::cout << dp2a231.x << " " << dp2a231.y << " " << dp2a231.z << std::endl;
		std::cout << dp2a312.x << " " << dp2a312.y << " " << dp2a312.z << std::endl;
		std::cout << dp1a312.x << " " << dp1a312.y << " " << dp1a312.z << std::endl;
		std::cout << dp3a312.x << " " << dp3a312.y << " " << dp3a312.z << std::endl;
		std::cout << dp3a123.x << " " << dp3a123.y << " " << dp3a123.z << std::endl;
		std::cout << dp2a123.x << " " << dp2a123.y << " " << dp2a123.z << std::endl;
		std::cout << dp1a123.x << " " << dp1a123.y << " " << dp1a123.z << std::endl;*/

		forces[p1] -= k_face * (angles.x - nominal_angles[i].x) * dp1a123;
		forces[p1] -= k_face * (angles.y - nominal_angles[i].y) * dp1a231;
		forces[p1] -= k_face * (angles.z - nominal_angles[i].z) * dp1a312;

		forces[p2] -= k_face * (angles.x - nominal_angles[i].x) * dp2a123;
		forces[p2] -= k_face * (angles.y - nominal_angles[i].y) * dp2a231;
		forces[p2] -= k_face * (angles.z - nominal_angles[i].z) * dp2a312;

		forces[p3] -= k_face * (angles.x - nominal_angles[i].x) * dp3a123;
		forces[p3] -= k_face * (angles.y - nominal_angles[i].y) * dp3a231;
		forces[p3] -= k_face * (angles.z - nominal_angles[i].z) * dp3a312;
	}

	return forces;
}

std::vector<glm::vec3> Origami::dampingForce()
{
	std::vector<glm::vec3> forces(this->vertices.size(), glm::vec3(0));
	for (int i = 0; i < edges.size(); i++) {
		float c = 2 * damping_ratio * std::sqrtf(EA / nominal_length[i]);
		forces[edges[i].x] += c * (vertices[edges[i].y].velocity - vertices[edges[i].x].velocity);
		forces[edges[i].y] += c * (vertices[edges[i].x].velocity - vertices[edges[i].y].velocity);
	}
	return forces;
}

std::vector<glm::vec3> Origami::getTotalForce()
{
	if (m_force_cache_used) {
		return m_total_force_cache;
	}
	m_total_force_cache = std::vector(vertices.size(), glm::vec3(0));
	if (enable_axial_constraints) {
		std::vector<glm::vec3> axialForces = this->axialConstraints();
		for (int i = 0; i < vertices.size(); i++) m_total_force_cache[i] += axialForces[i];
	}
	if (enable_crease_constraints) {
		std::vector<glm::vec3> creaseForces = this->creaseConstraints();
		for (int i = 0; i < vertices.size(); i++) m_total_force_cache[i] += creaseForces[i];
	}
	if (enable_face_constraints) {
		std::vector<glm::vec3> faceForces = this->faceConstraints();
		for (int i = 0; i < vertices.size(); i++) m_total_force_cache[i] += faceForces[i];
	}
	if (enable_damping_force) {
		std::vector<glm::vec3> dampingForces = this->dampingForce();
		for (int i = 0; i < vertices.size(); i++) m_total_force_cache[i] += dampingForces[i];
	}
	m_force_cache_used = true;
	return m_total_force_cache;
}

std::vector<glm::vec3> Origami::getVelocities()
{
	std::vector<glm::vec3> ret;
	for (int i = 0; i < vertices.size(); i++) {
		ret.push_back(vertices[i].velocity);
	}
	return ret;
}

std::vector<glm::vec3> Origami::getVertices()
{
	std::vector<glm::vec3> verts;
	for (int i = 0; i < vertices.size(); i++) {
		verts.push_back(vertices[i].coords);
	}
	return verts;
}

void Origami::setDefaultSettings()
{
	EA = 20.0f;
	k_fold = 0.7f;
	k_facet = 0.7f;
	k_face = 0.2f;
	damping_ratio = 0.45f;
	enable_axial_constraints = true;
	enable_crease_constraints = true;
	enable_face_constraints = true;
	enable_damping_force = true;
	calculateOptimalTimeStep();
}

void Origami::free() 
{
	glDeleteVertexArrays(1, &m_vao_faces);
	glDeleteBuffers(1, &m_vbo_faces);
	glDeleteBuffers(1, &m_ibo_faces);

	glDeleteVertexArrays(1, &m_vao_edges);
	glDeleteBuffers(1, &m_vbo_edges);
	glDeleteBuffers(1, &m_ibo_edges);
}

bool Origami::intersectWithFace(Ray& ray, unsigned int face)
{
	glm::vec3 A = vertices[faces[face].x].coords;
	glm::vec3 B = vertices[faces[face].y].coords;
	glm::vec3 C = vertices[faces[face].z].coords;
	glm::vec3 n = glm::cross(B - A, C - A);
	n = glm::normalize(n);
	
	if (glm::dot(n, ray.direction) == 0.0f) {
		return false;
	}

	float t = (glm::dot(n, vertices[faces[face].x].coords) - glm::dot(n, ray.origin)) / glm::dot(n, ray.direction);

	if (ray.t <= t) {
		return false;
	}
	
	glm::vec3 p = ray.origin + t * ray.direction;
	if (std::abs(areaOfTriangle(A, B, C) - areaOfTriangle(A, B, p) - areaOfTriangle(A, p, C) - areaOfTriangle(p, B, C)) < 1e-5) {
		ray.t = t;
		return true;
	}
	return false;

}

bool Origami::intersectWithRay(Ray& ray)
{
	unsigned int bestFace = -1;
	bool ret = false;
	for (int i = 0; i < faces.size(); i++) {
		if (intersectWithFace(ray, i)) {
			ret = true;
			bestFace = i;
		}
	}
	if (ret) {
		ray.face = bestFace;
		glm::vec3 p = ray.origin + ray.t * ray.direction;
		float totalArea = areaOfTriangle(vertices[faces[bestFace].x].coords, vertices[faces[bestFace].y].coords, vertices[faces[bestFace].z].coords);
		ray.barycentricCoords = glm::vec3(
			areaOfTriangle(p, vertices[faces[bestFace].y].coords, vertices[faces[bestFace].z].coords),
			areaOfTriangle(vertices[faces[bestFace].x].coords, p, vertices[faces[bestFace].z].coords),
			areaOfTriangle(vertices[faces[bestFace].x].coords, vertices[faces[bestFace].y].coords, p)
		) / totalArea;
	}
	return ret;
}

glm::vec3 Origami::getSelectedPoint(Settings& settings)
{
	if (settings.selectedPointFace == -1) {
		return glm::vec3(0);
	} else {
		return 
			settings.selectedPointBarycentricCoords.x * vertices[faces[settings.selectedPointFace].x].coords +
			settings.selectedPointBarycentricCoords.y * vertices[faces[settings.selectedPointFace].y].coords +
			settings.selectedPointBarycentricCoords.z * vertices[faces[settings.selectedPointFace].z].coords;
	}
}
