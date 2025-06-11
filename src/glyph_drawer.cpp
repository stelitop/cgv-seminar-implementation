#include "glyph_drawer.h"
#include <glm/gtc/type_ptr.hpp>
#include <iostream>

GlyphDrawer::GlyphDrawer()
{
	glGenVertexArrays(1, &m_vao);
	glBindVertexArray(m_vao);

	glGenBuffers(1, &m_vbo);
	glGenBuffers(1, &m_ibo);

	m_vertex_size = 0;
}

void GlyphDrawer::loadGlyphs(std::vector<glm::vec3> positions, std::vector<glm::vec3> directions, float scale)
{
	assert(positions.size() == directions.size());
	std::vector<glm::vec3> vertices;
	std::vector<glm::uvec3> faces;
	// create glyphs
	for (int i = 0; i < positions.size(); i++) {
		glm::vec3 scaled_dir = directions[i] * scale;
		scaled_dir *= std::log(glm::length(scaled_dir) + 1.0f) / glm::length(scaled_dir);
		glm::vec3 perp1 = glm::normalize(glm::cross(scaled_dir, scaled_dir.z == 0.0f ? glm::vec3(0.0f, 0.0f, 1.0f) : glm::vec3(1.0f, 0.0f, 0.0f)));
		glm::vec3 perp2 = glm::normalize(glm::cross(scaled_dir, perp1));
		vertices.push_back(positions[i] + scaled_dir);
		vertices.push_back(positions[i] + perp1 * 0.01f);
		vertices.push_back(positions[i] - perp1 * 0.01f);
		vertices.push_back(positions[i] + perp2 * 0.01f);
		vertices.push_back(positions[i] - perp2 * 0.01f);
		faces.push_back(glm::uvec3(5 * i, 5 * i + 1, 5 * i + 2));
		faces.push_back(glm::uvec3(5 * i, 5 * i + 3, 5 * i + 4));
	}

	glBindVertexArray(m_vao);

	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(decltype(vertices)::value_type)), vertices.data(), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ibo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(faces.size() * sizeof(decltype(faces)::value_type)), faces.data(), GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), (void*)0);

	m_vertex_size = 6 * positions.size();
}

void GlyphDrawer::draw(const Shader& shader, glm::mat4 mvpMatrix) {
	shader.bind();
	glUniformMatrix4fv(shader.getUniformLocation("mvpMatrix"), 1, GL_FALSE, glm::value_ptr(mvpMatrix));
	glUniform3f(shader.getUniformLocation("kd"), 0.f, 0.f, 1.f);
	glBindVertexArray(m_vao);
	glDrawElements(GL_TRIANGLES, m_vertex_size, GL_UNSIGNED_INT, nullptr);
}

void GlyphDrawer::free() {
	glDeleteVertexArrays(1, &m_vao);
	glDeleteBuffers(1, &m_vbo);
	glDeleteBuffers(1, &m_ibo);
}
