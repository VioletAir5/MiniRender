#pragma once

#include <cstdint>

namespace renderlab {

class AssetRegistry;
class OpenGLMeshCache;
class OpenGLShaderProgram;
class OpenGLTextureCache;

// 一帧内由多个 OpenGL Pass 共享的非拥有服务集合。
struct OpenGLPassContext {
    const AssetRegistry& registry;
    OpenGLShaderProgram& shader;
    OpenGLMeshCache& meshCache;
    OpenGLTextureCache& textureCache;
    std::uint64_t frameNumber{0};
};

} // namespace renderlab
