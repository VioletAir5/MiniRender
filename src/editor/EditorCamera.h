#pragma once

#include "renderer/RenderView.h"

#include <glm/vec3.hpp>

namespace renderlab {

class EditorCamera {
public:
    EditorCamera() = default;

    [[nodiscard]] RenderView renderView(int viewportWidth, int viewportHeight) const;

    void orbit(float deltaX, float deltaY);
    void pan(float deltaX, float deltaY, int viewportHeight);
    void zoom(float wheelSteps);
    void focus(const glm::vec3& center, float radius = 1.0F);
    void reset();

    [[nodiscard]] glm::vec3 position() const;
    [[nodiscard]] const glm::vec3& target() const noexcept;
    [[nodiscard]] float distance() const noexcept;
    [[nodiscard]] float yawDegrees() const noexcept;
    [[nodiscard]] float pitchDegrees() const noexcept;

private:
    [[nodiscard]] glm::vec3 forward() const;
    [[nodiscard]] glm::vec3 right() const;
    [[nodiscard]] glm::vec3 up() const;

    glm::vec3 target_{0.0F, 0.0F, 0.0F};
    float yawDegrees_{0.0F};
    float pitchDegrees_{0.0F};
    float distance_{5.0F};
    float verticalFovDegrees_{60.0F};
    float nearPlane_{0.05F};
    float farPlane_{1000.0F};
};

} // namespace renderlab
