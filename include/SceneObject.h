#pragma once
#include <string>
#include <map>
#include <glm/glm.hpp>

struct SceneObject {
    std::string  name;
    size_t       first_index;
    size_t       num_indices;
    unsigned int rendering_mode;
    unsigned int vertex_array_object_id;
    glm::vec3    bbox_min;
    glm::vec3    bbox_max;
};

struct AABB {
    glm::vec3 min;
    glm::vec3 max;
};

extern std::map<int, AABB> g_listaAABB;
extern std::map<std::string, SceneObject> g_VirtualScene;
