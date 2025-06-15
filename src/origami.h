#pragma once

#include <filesystem>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_int3.hpp>
#include <framework/shader.h>
#include <framework/ray.h>
#include "settings.h"
//#include <glm/fwd.hpp>

#define BOUNDARY_EDGE 0u
#define MOUNTAIN_EDGE 1u
#define VALLEY_EDGE 2u
#define FACET_EDGE 3u

#define RENDERMODE_PLAIN 0
#define RENDERMODE_VELOCITY 1
#define RENDERMODE_FORCE 2

class Origami {
public:
	Origami();

	static Origami loadFromFile(std::filesystem::path filePath);

	/// <summary>
	/// Trianglulates the origami by turning all 
	/// </summary>
	void triangulate(std::vector<unsigned int> verts);

	//void draw(const Shader& face_shader, const Shader& edge_shader, glm::mat4 mvpMatrix, int renderMode, float magnitudeCutoff);
	void draw(const Shader& face_shader, const Shader& edge_shader, glm::mat4 mvpMatrix, Settings& settings);

	void updateVertexBuffers();

	void step();
	void calculateOptimalTimeStep();

	std::vector<glm::vec3> axialConstraints();
	std::vector<glm::vec3> creaseConstraints();
	std::vector<glm::vec3> faceConstraints();
	std::vector<glm::vec3> dampingForce();

	std::vector<glm::vec3> getTotalForce();
	std::vector<glm::vec3> getVelocities();

	std::vector<glm::vec3> getVertices();

	void setDefaultSettings();
	void free();

	bool intersectWithRay(Ray& ray);

	glm::vec3 getSelectedPoint(Settings& settings);

	// parameters
	float EA = 20.0f;
	float k_fold = 0.7f;
	float k_facet = 0.7f;
	float k_face = 0.2f;
	float damping_ratio = 0.45f;
	float deltaT = 0.01; // recalculated when loading an origami
	float target_angle_percent = 0.0;

	bool enable_axial_constraints = true;
	bool enable_crease_constraints = true;
	bool enable_face_constraints = true;
	bool enable_damping_force = true;

	std::string name;


//private:

	class VertexData {
	public:
		glm::vec3 coords;
		glm::vec3 force;
		glm::vec3 velocity;

		VertexData(glm::vec3 coords, glm::vec3 force, glm::vec3 velocity) {
			this->coords = coords;
			this->force = force;
			this->velocity = velocity;
		}
	};

	std::vector<VertexData> vertices;

	/// <summary>
	/// Each edge (x, y, z) represents: 
	/// - x - index of the first vertex
	/// - y - index of the second vertex
	/// - z - 0 if unfolded, +1 if mountain fold, -1 if valley fold
	/// </summary>
	std::vector<glm::uvec3> edges;
	
	std::vector<glm::uvec3> faces;

	/// <summary>
	/// For edge i, edge_to_faces[i] contains the two indeces of the faces on each side of the edge.
	/// If this is a boundary edge, then edge_to_faces[i].x == edge_to_faces[i].y
	/// </summary>
	std::vector<glm::uvec2> edge_to_faces;
	/// <summary>
	/// Nominal length of edge i.
	/// </summary>
	std::vector<float> nominal_length;
	/// <summary>
	/// Nominal angles for all 3 points of a face. Value x corresponds to the angle at corner x.
	/// </summary>
	std::vector<glm::vec3> nominal_angles;

	std::vector<glm::vec3> normals;

	void normalizeVertices();
	
	void prepareGpuMesh();

	/// <summary>
	/// Calculates the cotangent by the indecies of 3 points in the mesh.
	/// </summary>
	/// <param name="p1"></param>
	/// <param name="p2"></param>
	/// <param name="p3"></param>
	/// <returns></returns>
	float cot(unsigned int p1, unsigned int p2, unsigned int p3);

	/// <summary>
	/// Calculates the angles at each of the three corners of a face.
	/// </summary>
	/// <param name="face"></param>
	/// <returns></returns>
	glm::vec3 angles(glm::uvec3 face);

	std::vector<VertexData> formatVertices();
	void prepareEdgeShaderData(std::vector<glm::vec4>& vertexData, std::vector<glm::uvec3>& faceData);
	void updateNormals();

	bool intersectWithFace(Ray& ray, unsigned int face);


	GLuint m_vao_faces;
	GLuint m_vbo_faces;
	GLuint m_ibo_faces;

	GLuint m_vao_edges;
	GLuint m_vbo_edges;
	GLuint m_ibo_edges;

	bool m_force_cache_used = false;
	std::vector<glm::vec3> m_total_force_cache;
};

unsigned int opposite_vertex(glm::uvec3 face, glm::uvec3 edge);