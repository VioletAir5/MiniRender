#include "editor/EditorCamera.h"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

#include <algorithm>
#include <cmath>

namespace renderlab {
namespace {

constexpr float OrbitSensitivity = 0.25F;
constexpr float ZoomSensitivity = 0.2F;
constexpr float MinimumPitchDegrees = -89.0F;
constexpr float MaximumPitchDegrees = 89.0F;
constexpr float MinimumDistance = 0.05F;
constexpr float MaximumDistance = 1000.0F;

} // namespace

RenderView EditorCamera::renderView(const int viewportWidth,
                                    const int viewportHeight) const {
    RenderView result;
    if (viewportWidth <= 0 || viewportHeight <= 0) {
        return result;
    }

    const float aspect = static_cast<float>(viewportWidth) /
                         static_cast<float>(viewportHeight);
    result.cameraPosition = position();
    result.view = glm::lookAt(result.cameraPosition, target_, up());
    result.projection = glm::perspective(glm::radians(verticalFovDegrees_),
                                         aspect, nearPlane_, farPlane_);
    result.valid = true;
    return result;
}

void EditorCamera::orbit(const float deltaX, const float deltaY) {
    yawDegrees_ -= deltaX * OrbitSensitivity;
    pitchDegrees_ = std::clamp(pitchDegrees_ - deltaY * OrbitSensitivity,
                               MinimumPitchDegrees, MaximumPitchDegrees);

    yawDegrees_ = std::fmod(yawDegrees_, 360.0F);
}

void EditorCamera::pan(const float deltaX, const float deltaY,
                       const int viewportHeight) {
    if (viewportHeight <= 0) {
        return;
    }

    const float visibleHeight =
        2.0F * distance_ * std::tan(glm::radians(verticalFovDegrees_ * 0.5F));
    const float worldUnitsPerPixel =
        visibleHeight / static_cast<float>(viewportHeight);

    target_ += (-right() * deltaX + up() * deltaY) * worldUnitsPerPixel;
}

void EditorCamera::zoom(const float wheelSteps) {
    distance_ *= std::exp(-wheelSteps * ZoomSensitivity);
    distance_ = std::clamp(distance_, MinimumDistance, MaximumDistance);
}

void EditorCamera::focus(const glm::vec3& center, const float radius) {
    target_ = center;

    const float safeRadius = std::max(radius, MinimumDistance);
    const float halfFovRadians = glm::radians(verticalFovDegrees_ * 0.5F);
    distance_ = std::clamp((safeRadius / std::tan(halfFovRadians)) * 1.25F,
                           MinimumDistance, MaximumDistance);
}

void EditorCamera::reset() {
    target_ = {0.0F, 0.0F, 0.0F};
    yawDegrees_ = 0.0F;
    pitchDegrees_ = 0.0F;
    distance_ = 5.0F;
}

glm::vec3 EditorCamera::position() const {
    const float yaw = glm::radians(yawDegrees_);
    const float pitch = glm::radians(pitchDegrees_);
    const float horizontalDistance = distance_ * std::cos(pitch);

    return target_ + glm::vec3{
        horizontalDistance * std::sin(yaw),
        distance_ * std::sin(pitch),
        horizontalDistance * std::cos(yaw),
    };
}

const glm::vec3& EditorCamera::target() const noexcept {
    return target_;
}

float EditorCamera::distance() const noexcept {
    return distance_;
}

float EditorCamera::yawDegrees() const noexcept {
    return yawDegrees_;
}

float EditorCamera::pitchDegrees() const noexcept {
    return pitchDegrees_;
}

glm::vec3 EditorCamera::forward() const {
    return glm::normalize(target_ - position());
}

glm::vec3 EditorCamera::right() const {
    return glm::normalize(glm::cross(forward(), glm::vec3{0.0F, 1.0F, 0.0F}));
}

glm::vec3 EditorCamera::up() const {
    return glm::normalize(glm::cross(right(), forward()));
}

} // namespace renderlab
