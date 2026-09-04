#pragma once

#include <cstdint>

namespace renderlab {

class AssetRegistry;
class OpenGLMeshCache;
class OpenGLShaderProgram;
class OpenGLTextureCache;
struct RenderItem;

// 一帧内由多个 OpenGL Pass 共享的非拥有服务集合。
struct OpenGLPassContext {
    const AssetRegistry& registry;
    OpenGLShaderProgram& shader;
    OpenGLMeshCache& meshCache;
    OpenGLTextureCache& textureCache;
    std::uint64_t frameNumber{0};
    // 当前帧由 Forward Pass 找到的轮廓目标；每帧执行 Pipeline 前清空。
    const RenderItem* outlinedItem{nullptr};
};

} // namespace renderlab
