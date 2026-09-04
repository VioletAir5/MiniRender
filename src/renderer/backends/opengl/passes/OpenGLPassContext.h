#pragma once

#include "assets/AssetHandle.h"
#include "scene/LightTypes.h"

#include <cstdint>
#include <glm/mat4x4.hpp>

namespace renderlab {

class AssetRegistry;
class OpenGLMeshCache;
class OpenGLRenderResources;
class OpenGLShaderCache;
class OpenGLTextureCache;
struct RenderItem;

// 一帧内由多个 OpenGL Pass 共享的非拥有服务集合。
struct OpenGLPassContext {
    const AssetRegistry& registry;
    OpenGLShaderCache& shaderCache;
    ShaderHandle fallbackSurfaceShader;
    ShaderHandle directionalShadowShader;
    OpenGLMeshCache& meshCache;
    OpenGLTextureCache& textureCache;
    OpenGLRenderResources& renderResources;
    std::uint64_t frameNumber{0};
    // 当前帧由 Forward Pass 找到的轮廓目标；每帧执行 Pipeline 前清空。
    const RenderItem* outlinedItem{nullptr};
    glm::mat4 directionalShadowMatrix{1.0F};
    int directionalShadowLightIndex{-1};
    ShadowTechnique directionalShadowTechnique{ShadowTechnique::None};
    float directionalShadowBias{0.0015F};
};

} // namespace renderlab
