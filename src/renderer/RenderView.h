#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace renderlab {

// 描述一次渲染所需的观察数据，不依赖 OpenGL、Vulkan 等具体 API。
struct RenderView {
    glm::mat4 view{1.0F};
    glm::mat4 projection{1.0F};
    glm::vec3 cameraPosition{0.0F, 0.0F, 0.0F};
    bool valid{false};
};

} // namespace renderlab
