#pragma once

#include "assets/AssetHandle.h"

#include <glm/vec4.hpp>

#include <cstdint>
#include <optional>
#include <string>

namespace renderlab {

enum class AlphaMode { Opaque, Mask, Blend };
struct TextureBinding {
    TextureHandle texture;
    std::uint32_t texCoordSet{0};
};
// 描述与图形 API 无关的基础材质数据。
struct MaterialAsset {
    std::string name{"Material"};

    glm::vec4 baseColorFactor{1.0F};
    std::optional<TextureBinding> baseColorTexture;

    float metallicFactor{1.0F};
    float roughnessFactor{1.0F};

    AlphaMode alphaMode{AlphaMode::Opaque};
    float alphaCutoff{0.5F};
    bool doubleSided{false};
};

} // namespace renderlab
