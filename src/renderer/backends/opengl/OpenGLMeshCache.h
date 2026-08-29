#pragma once

#include "renderer/backends/opengl/OpenGLMesh.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace renderlab {

struct CachedOpenGLMesh {
    std::uint32_t generation{0};
    std::uint32_t revision{0};
    OpenGLMesh mesh;
};

class OpenGLMeshCache {
private:
    std::vector<std::optional<CachedOpenGLMesh>> entries_;
};

} // namespace renderlab
