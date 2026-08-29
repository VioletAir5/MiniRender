#pragma once

#include "renderer/RenderView.h"

#include <glm/vec3.hpp>

namespace renderlab {

// 围绕目标点工作的编辑器轨道相机，独立于场景中的 CameraComponent。
class EditorCamera {
public:
    EditorCamera() = default;

    // 按视口尺寸生成 API 无关的观察和投影矩阵。
    [[nodiscard]] RenderView renderView(int viewportWidth, int viewportHeight) const;

    // 根据像素增量环绕目标旋转，并限制俯仰角避免翻转。
    void orbit(float deltaX, float deltaY);
    // 在相机局部平面平移目标点，移动速度随观察距离缩放。
    void pan(float deltaX, float deltaY, int viewportHeight);
    // 按滚轮步数指数缩放观察距离，并限制最小和最大值。
    void zoom(float wheelSteps);
    // 聚焦给定包围球，使目标居中并调整到合适距离。
    void focus(const glm::vec3& center, float radius = 1.0F);
    // 恢复默认目标、朝向和观察距离。
    void reset();

    // 返回由球坐标参数计算出的世界空间相机位置。
    [[nodiscard]] glm::vec3 position() const;
    // 返回当前轨道中心、距离及欧拉角状态。
    [[nodiscard]] const glm::vec3& target() const noexcept;
    [[nodiscard]] float distance() const noexcept;
    [[nodiscard]] float yawDegrees() const noexcept;
    [[nodiscard]] float pitchDegrees() const noexcept;

private:
    // 计算相机局部正交基，供观察矩阵和交互操作复用。
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
