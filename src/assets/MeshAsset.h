#pragma once

#include "assets/AssetHandle.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace renderlab {

struct Vertex {
    glm::vec3 position{0.0F};
    glm::vec3 normal{0.0F, 1.0F, 0.0F};
    glm::vec2 texCoord{0.0F};
    glm::vec4 color{1.0F};
};

struct MeshPrimitive {
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
    MaterialHandle defaultMaterial;
};

struct MeshAsset {
    std::string name;
    std::vector<MeshPrimitive> primitives;
};

} // namespace renderlab
