#pragma once

#include <glm/vec4.hpp>

#include <string>

namespace renderlab {

// 描述与图形 API 无关的基础材质数据。
struct MaterialAsset {
    std::string name{"Material"};
    glm::vec4 baseColorFactor{1.0F, 1.0F, 1.0F, 1.0F};
};

} // namespace renderlab
