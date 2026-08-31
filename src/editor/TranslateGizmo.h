#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <array>

namespace renderlab {

enum class GizmoAxis { None, X, Y, Z };

// 一帧内用于绘制、命中测试和拖动换算的屏幕空间数据。
struct TranslateGizmoGeometry {
    bool visible{false};
    glm::vec2 origin{};
    std::array<glm::vec2, 3> endpoints{};
    float worldScale{1.0F};
};

class TranslateGizmo final {
  public:
    [[nodiscard]] static TranslateGizmoGeometry project(
        const glm::vec3& worldOrigin, const glm::mat4& view, const glm::mat4& projection,
        int viewportWidth, int viewportHeight);
    [[nodiscard]] static GizmoAxis hitTest(const TranslateGizmoGeometry& geometry,
                                           const glm::vec2& point,
                                           float tolerancePixels = 9.0F);
    [[nodiscard]] static glm::vec3 dragDelta(const TranslateGizmoGeometry& geometry,
                                             GizmoAxis axis,
                                             const glm::vec2& pixelDelta);
};

} // namespace renderlab
