#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace renderlab {

struct RenderView {
    glm::mat4 view{1.0F};
    glm::mat4 projection{1.0F};
    glm::vec3 cameraPosition{0.0F, 0.0F, 0.0F};
    bool valid{false};
};

} // namespace renderlab
