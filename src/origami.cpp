#pragma once

#include "origami.h"
#include <fstream>
#include "../external_code/third_party/json/single_include/nlohmann/json.hpp"
#include <iostream>
#include <glm/gtc/type_ptr.hpp>

using json = nlohmann::json;

Origami::Origami() {

}

Origami Origami::load_from_file(std::filesystem::path filePath) {
	std::ifstream f(filePath);		
	json data = json::parse(f);	

	Origami origami = Origami();

	for (json vert : data["vertices_coords"]) {		
		origami.vertices.push_back(glm::vec3(vert[0], vert[1], vert[2]) * 10.0f);
	}

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
	
	for (json face : data["faces_vertices"]) {
		std::vector<unsigned int> face_verts;
		for (unsigned int vert_index : face) {
			face_verts.push_back(vert_index);
		}
		origami.triangulate(face_verts);
	}

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
		outside_point = glm::max(outside_point, this->vertices[v]);
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
		float area = areaOfTriangle(this->vertices[v0], this->vertices[v1], this->vertices[v2]);
		for (int j = 0; j < verts.size() - 3; j++) {
			unsigned int vx = verts[(i + 3 + j) % verts.size()];
			float area_around_point = areaOfTriangle(this->vertices[vx], this->vertices[v1], this->vertices[v2]) + areaOfTriangle(this->vertices[v0], this->vertices[vx], this->vertices[v2]) + areaOfTriangle(this->vertices[v0], this->vertices[v1], this->vertices[vx]);
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

void Origami::draw(const Shader& face_shader, const Shader& edge_shader, glm::mat4 mvpMatrix) {

	// draw faces
	face_shader.bind();
	glUniformMatrix4fv(face_shader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
	glUniform3f(face_shader.getUniformLocation("kd"), 1.f, 0.f, 0.f);
	glBindVertexArray(m_vao_faces);
	glDrawElements(GL_TRIANGLES, 3*this->faces.size(), GL_UNSIGNED_INT, nullptr);

	//draw edges
	edge_shader.bind();
	glUniformMatrix4fv(edge_shader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
	glUniform3f(edge_shader.getUniformLocation("kd"), 0.f, 1.f, 0.f);
	glBindVertexArray(m_vao_edges);
	glDrawElements(GL_LINES, 2*this->edges.size(), GL_UNSIGNED_INT, nullptr);
}

void Origami::prepareGpuMesh() {

	// Create VAO and bind it so subsequent creations of VBO and IBO are bound to this VAO
	glGenVertexArrays(1, &m_vao_faces);
	glBindVertexArray(m_vao_faces);

	// Create vertex buffer object (VBO)
	glGenBuffers(1, &m_vbo_faces);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo_faces);
	glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(this->vertices.size() * sizeof(decltype(this->vertices)::value_type)), this->vertices.data(), GL_STATIC_DRAW);

	// Create index buffer object (IBO)
	glGenBuffers(1, &m_ibo_faces);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo_faces);

	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(newfaces.size() * sizeof(decltype(newfaces)::value_type)), newfaces.data(), GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(this->faces.size() * sizeof(decltype(this->faces)::value_type)), this->faces.data(), GL_STATIC_DRAW);

	// We tell OpenGL what each vertex looks like and how they are mapped to the shader (location = ...).
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);





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
	glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(this->vertices.size() * sizeof(decltype(this->vertices)::value_type)), this->vertices.data(), GL_STATIC_DRAW);

	// Create index buffer object (IBO)
	glGenBuffers(1, &m_ibo_edges);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo_edges);

	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(newfaces.size() * sizeof(decltype(newfaces)::value_type)), newfaces.data(), GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(edges_gpu.size() * sizeof(decltype(edges_gpu)::value_type)), edges_gpu.data(), GL_STATIC_DRAW);

	// We tell OpenGL what each vertex looks like and how they are mapped to the shader (location = ...).
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);
}