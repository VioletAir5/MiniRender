#pragma once

#include "assets/AssetHandle.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace renderlab {

// CPU 侧通用顶点格式，属性位置由具体图形后端在上传时解释。
struct Vertex {
    glm::vec3 position{0.0F};
    glm::vec3 normal{0.0F, 1.0F, 0.0F};
    glm::vec2 texCoord{0.0F};
    glm::vec4 color{1.0F};
};

// 可独立绘制的一组顶点和三角形索引，并可指定默认材质。
struct MeshPrimitive {
    std::vector<Vertex> vertices;
    std::vector<std::uint32_t> indices;
    MaterialHandle defaultMaterial;
};

// CPU 侧网格资产；一个导入模型可由多个 Primitive 组成。
struct MeshAsset {
    std::string name;
    std::vector<MeshPrimitive> primitives;
};

} // namespace renderlab
