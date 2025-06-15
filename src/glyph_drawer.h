#pragma once
#include <vector>
#include <glm/ext/vector_float3.hpp>
#include <framework/shader.h>
#include "settings.h"
#include "origami.h"

class GlyphDrawer {

public:
	GlyphDrawer(Settings& settings);

	void loadGlyphs(Origami& origami, std::vector<glm::vec3> positions, std::vector<glm::vec3> directions, float scale);
	void loadGlyphsSetLength(Origami& origami, std::vector<glm::vec3> positions, std::vector<glm::vec3> directions, float length);
	void draw(const Shader &shader, glm::mat4 mvpMatrix, glm::vec3 color);
	void free();

private:

	void splitFaces(Origami& origami, std::vector<glm::vec3>& positions, std::vector<glm::vec3>& directions);

	GLuint m_vao;
	GLuint m_vbo;
	GLuint m_ibo;
	unsigned int m_vertex_size;
	Settings& m_settings;
};