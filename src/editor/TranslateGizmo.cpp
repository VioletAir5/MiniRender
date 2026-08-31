#include "editor/TranslateGizmo.h"

#include <glm/geometric.hpp>
#include <glm/vec4.hpp>

#include <algorithm>
#include <cmath>
#include <limits>

namespace renderlab {
namespace {

constexpr std::array<glm::vec3, 3> Axes{
    glm::vec3{1.0F, 0.0F, 0.0F}, glm::vec3{0.0F, 1.0F, 0.0F},
    glm::vec3{0.0F, 0.0F, 1.0F}};

bool projectPoint(const glm::vec3& point, const glm::mat4& viewProjection,
                  const int width, const int height, glm::vec2& screen) {
    const glm::vec4 clip = viewProjection * glm::vec4{point, 1.0F};
    if (clip.w <= 0.0001F) return false;
    const glm::vec3 ndc = glm::vec3{clip} / clip.w;
    screen = {(ndc.x * 0.5F + 0.5F) * static_cast<float>(width),
              (0.5F - ndc.y * 0.5F) * static_cast<float>(height)};
    return ndc.z >= -1.0F && ndc.z <= 1.0F;
}

float distanceToSegment(const glm::vec2& point, const glm::vec2& start,
                        const glm::vec2& end) {
    const glm::vec2 segment = end - start;
    const float lengthSquared = glm::dot(segment, segment);
    if (lengthSquared <= 0.0001F) return std::numeric_limits<float>::max();
    const float t = std::clamp(glm::dot(point - start, segment) / lengthSquared, 0.15F, 1.15F);
    return glm::length(point - (start + segment * t));
}

} // namespace

TranslateGizmoGeometry TranslateGizmo::project(const glm::vec3& worldOrigin,
                                               const glm::mat4& view,
                                               const glm::mat4& projection,
                                               const int viewportWidth,
                                               const int viewportHeight,
                                               const std::array<glm::vec3, 3>& axes) {
    TranslateGizmoGeometry result;
    result.axes = axes;
    if (viewportWidth <= 0 || viewportHeight <= 0) return result;
    const glm::mat4 viewProjection = projection * view;
    const glm::vec4 viewPosition = view * glm::vec4{worldOrigin, 1.0F};
    result.worldScale = std::max(std::abs(viewPosition.z) * 0.16F, 0.25F);
    if (!projectPoint(worldOrigin, viewProjection, viewportWidth, viewportHeight, result.origin)) {
        return result;
    }
    for (std::size_t index = 0; index < axes.size(); ++index) {
        if (!projectPoint(worldOrigin + axes[index] * result.worldScale, viewProjection,
                          viewportWidth, viewportHeight, result.endpoints[index])) {
            return result;
        }
    }
    result.visible = true;
    return result;
}

GizmoAxis TranslateGizmo::hitTest(const TranslateGizmoGeometry& geometry,
                                  const glm::vec2& point,
                                  const float tolerancePixels) {
    if (!geometry.visible) return GizmoAxis::None;
    float closest = tolerancePixels;
    GizmoAxis result = GizmoAxis::None;
    for (std::size_t index = 0; index < geometry.endpoints.size(); ++index) {
        const float distance = distanceToSegment(point, geometry.origin, geometry.endpoints[index]);
        if (distance < closest) {
            closest = distance;
            result = static_cast<GizmoAxis>(static_cast<int>(index) + 1);
        }
    }
    return result;
}

float TranslateGizmo::dragAmount(const TranslateGizmoGeometry& geometry,
                                 const GizmoAxis axis,
                                 const glm::vec2& pixelDelta) {
    const int index = static_cast<int>(axis) - 1;
    if (!geometry.visible || index < 0 || index >= 3) return 0.0F;
    const glm::vec2 screenAxis = geometry.endpoints[static_cast<std::size_t>(index)] -
                                 geometry.origin;
    const float pixelLength = glm::length(screenAxis);
    if (pixelLength <= 0.001F) return 0.0F;
    return glm::dot(pixelDelta, screenAxis / pixelLength) /
           pixelLength * geometry.worldScale;
}

glm::vec3 TranslateGizmo::dragDelta(const TranslateGizmoGeometry& geometry,
                                    const GizmoAxis axis, const glm::vec2& pixelDelta) {
    const int index = static_cast<int>(axis) - 1;
    if (index < 0 || index >= 3) return {};
    return geometry.axes[static_cast<std::size_t>(index)] *
           dragAmount(geometry, axis, pixelDelta);
}

} // namespace renderlab
