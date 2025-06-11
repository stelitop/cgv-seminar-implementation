#include <vector>
#include <glm/ext/vector_float3.hpp>
#include <framework/shader.h>

class GlyphDrawer {

public:
	GlyphDrawer();

	void loadGlyphs(std::vector<glm::vec3> positions, std::vector<glm::vec3> directions, float scale);
	void draw(const Shader &shader, glm::mat4 mvpMatrix);
	void free();

private:
	GLuint m_vao;
	GLuint m_vbo;
	GLuint m_ibo;
	unsigned int m_vertex_size;
};