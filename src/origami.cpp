#pragma once

#include "origami.h"
#include <fstream>
#include "../external_code/third_party/json/single_include/nlohmann/json.hpp"
#include <iostream>

using json = nlohmann::json;

Origami::Origami() {

}

Origami Origami::load_from_file(std::filesystem::path filePath) {
	std::ifstream f(filePath);		
	json data = json::parse(f);	

	Origami origami = Origami();

	for (json vert : data["vertices_coords"]) {		
		origami.vertices.push_back(glm::vec3(vert[0], vert[1], vert[2]));
	}

	for (size_t i = 0; i < data["edges_vertices"].size(); i++) {
		int crease_type = 0;
		if (data["edges_assignment"][i] == "M") {
			crease_type = 1;
		}
		else if (data["edges_assignment"][i] == "V") {
			crease_type = -1;
		}
		origami.edges.push_back(glm::ivec3(data["edges_vertices"][i][0], data["edges_vertices"][i][1], crease_type));
	}

	for (json face : data["faces_vertices"]) {
		std::vector<unsigned int> face_verts;
		for (unsigned int vert_index : face) {
			face_verts.push_back(vert_index);
		}
		origami.faces.push_back(face);
	}

	return origami;
}

