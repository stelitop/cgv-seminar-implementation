#include <filesystem>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_int3.hpp>
//#include <glm/fwd.hpp>

class Origami {
private:
	std::vector<glm::vec3> vertices;

	/// <summary>
	/// Each edge (x, y, z) represents: 
	/// - x - index of the first vertex
	/// - y - index of the second vertex
	/// - z - 0 if unfolded, +1 if mountain fold, -1 if valley fold
	/// </summary>
	std::vector<glm::ivec3> edges;
	
	std::vector<std::vector<unsigned int>> faces;

	Origami();

public:
	static Origami load_from_file(std::filesystem::path filePath);
};