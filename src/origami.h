#include <filesystem>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_int3.hpp>
#include <framework/shader.h>
//#include <glm/fwd.hpp>

#define BOUNDARY_EDGE 0u
#define MOUNTAIN_EDGE 1u
#define VALLEY_EDGE 2u
#define FACET_EDGE 3u


class Origami {
public:
	static Origami load_from_file(std::filesystem::path filePath);

	/// <summary>
	/// Trianglulates the origami by turning all 
	/// </summary>
	void triangulate(std::vector<unsigned int> verts);

	void draw(const Shader& face_shader, const Shader& edge_shader, glm::mat4 mvpMatrix);

private:
	std::vector<glm::vec3> vertices;

	/// <summary>
	/// Each edge (x, y, z) represents: 
	/// - x - index of the first vertex
	/// - y - index of the second vertex
	/// - z - 0 if unfolded, +1 if mountain fold, -1 if valley fold
	/// </summary>
	std::vector<glm::uvec3> edges;
	
	std::vector<glm::uvec3> faces;

	Origami();

	void prepareGpuMesh();

	GLuint m_vao_faces;
	GLuint m_vbo_faces;
	GLuint m_ibo_faces;

	GLuint m_vao_edges;
	GLuint m_vbo_edges;
	GLuint m_ibo_edges;
};