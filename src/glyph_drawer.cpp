#include "glyph_drawer.h"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

GlyphDrawer::GlyphDrawer(Settings& settings) : m_settings(settings)
{
	glGenVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);

	glGenBuffers(1, &m_vbo);
	glGenBuffers(1, &m_ibo);

	m_vertex_size = 0;
}

void GlyphDrawer::loadGlyphs(Origami& origami, std::vector<glm::vec3> positions, std::vector<glm::vec3> directions, float scale)
{
	assert(positions.size() == directions.size());
	splitFaces(origami, positions, directions);
	std::vector<glm::vec3> vertices;
	std::vector<glm::uvec3> faces;
	glm::vec3 selectedPoint = origami.getSelectedPoint(m_settings);
	// create glyphs
	for (int i = 0; i < positions.size(); i++) {
		if (glm::length(directions[i]) < 1e-5) {
			continue;
		}

		// check if the position is in the range of the selected point
		if (m_settings.useSelectedPoint && glm::distance(positions[i], selectedPoint) > m_settings.selectedPointRadius) {
			continue;
		}

		glm::vec3 scaled_dir = directions[i] * scale;
		scaled_dir *= std::log(glm::length(scaled_dir) + 1.0f) / glm::length(scaled_dir);
		glm::vec3 perp1 = glm::normalize(glm::cross(scaled_dir, scaled_dir.z == 0.0f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(1.0f, 0.0f, 0.0f)));
		glm::vec3 perp2 = glm::normalize(glm::cross(scaled_dir, perp1));

		int idx = vertices.size();
		vertices.push_back(positions[i] + scaled_dir);
		vertices.push_back(positions[i] + perp1 * 0.001f);
		vertices.push_back(positions[i] - perp1 * 0.001f);
		vertices.push_back(positions[i] + perp2 * 0.001f);
		vertices.push_back(positions[i] - perp2 * 0.001f);
		vertices.push_back(positions[i] + scaled_dir * 0.85f + perp1 * 0.01f);
		vertices.push_back(positions[i] + scaled_dir * 0.85f - perp1 * 0.01f);
		vertices.push_back(positions[i] + scaled_dir * 0.85f + perp2 * 0.01f);
		vertices.push_back(positions[i] + scaled_dir * 0.85f - perp2 * 0.01f);
		faces.push_back(glm::uvec3(idx, idx + 1, idx + 2));
		faces.push_back(glm::uvec3(idx, idx + 3, idx + 4));
		faces.push_back(glm::uvec3(idx, idx + 5, idx + 6));
		faces.push_back(glm::uvec3(idx, idx + 7, idx + 8));
	}

	glBindVertexArray(m_vao);

	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(decltype(vertices)::value_type)), vertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(faces.size() * sizeof(decltype(faces)::value_type)), faces.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

	m_vertex_size = 3 * faces.size();
}

void GlyphDrawer::loadGlyphsSetLength(Origami& origami, std::vector<glm::vec3> positions, std::vector<glm::vec3> directions, float length)
{
	for (int i = 0; i < directions.size(); i++) {
		directions[i] = glm::normalize(directions[i]) * length;
	}
	loadGlyphs(origami, positions, directions, 1.0f);
}

void GlyphDrawer::draw(const Shader& shader, glm::mat4 mvpMatrix, glm::vec3 color) {
	shader.bind();
	glUniformMatrix4fv(shader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
	glUniform3f(shader.getUniformLocation("kd"), color.r, color.g, color.b);
	glBindVertexArray(m_vao);
	glDrawElements(GL_TRIANGLES, m_vertex_size, GL_UNSIGNED_INT, nullptr);
}

void GlyphDrawer::free() {
	glDeleteVertexArrays(1, &m_vao);
	glDeleteBuffers(1, &m_vbo);
	glDeleteBuffers(1, &m_ibo);
}

void GlyphDrawer::splitFaces(Origami& origami, std::vector<glm::vec3>& positions, std::vector<glm::vec3>& directions)
{
	for (glm::uvec3 face : origami.faces) {
		float step = 1.0f / float(m_settings.faceSplitting);
		for (int i = 0; i < m_settings.faceSplitting + 1; i++) {
			for (int j = 0; j < i + 1; j++) {
				glm::vec3 bary(1.0f - float(i) * step, float(j) * step, float(i) * step - float(j) * step);
				if (bary.x == 1.0f || bary.y == 1.0f || bary.z == 1.0f) {
					continue;
				}
				positions.push_back(positions[face.x] * bary.x + positions[face.y] * bary.y + positions[face.z] * bary.z);
				directions.push_back(directions[face.x] * bary.x + directions[face.y] * bary.y + directions[face.z] * bary.z);
			}
		}
	}
}
