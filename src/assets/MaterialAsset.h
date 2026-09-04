#pragma once

#include "assets/AssetHandle.h"

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cstdint>
#include <optional>
#include <string>

namespace renderlab {

enum class AlphaMode { Opaque, Mask, Blend };
struct TextureBinding {
    TextureHandle texture;
    glm::vec2 offset{0.0F};
    glm::vec2 scale{1.0F};
    float rotationRadians{0.0F};
    std::uint32_t texCoordSet{0};
};
// 描述与图形 API 无关的基础材质数据。
struct MaterialAsset {
    std::string name{"Material"};
    // 可为空；渲染后端为空或无效时使用默认表面 Shader。
    ShaderHandle shader;

    glm::vec4 baseColorFactor{1.0F};
    std::optional<TextureBinding> baseColorTexture;

    std::optional<TextureBinding> metallicRoughnessTexture;
    std::optional<TextureBinding> normalTexture;
    std::optional<TextureBinding> occlusionTexture;
    std::optional<TextureBinding> emissiveTexture;
    float metallicFactor{1.0F};
    float roughnessFactor{1.0F};

    AlphaMode alphaMode{AlphaMode::Opaque};
    float normalScale{1.0F};
    float occlusionStrength{1.0F};
    glm::vec3 emissiveFactor{0.0F};
    bool unlit{false};

    float alphaCutoff{0.5F};
    bool doubleSided{false};
};

} // namespace renderlab
