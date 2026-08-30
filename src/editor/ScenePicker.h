#pragma once

#include "scene/EntityId.h"

#include <glm/vec3.hpp>

#include <optional>

namespace renderlab {

class AssetRegistry;
struct RenderFrame;

// 射线命中场景三角形后的最近实体及世界空间信息。
struct ScenePickResult {
    EntityId entity{NullEntity};
    float distance{0.0F};
    glm::vec3 worldPosition{0.0F};
};

// 用于编辑器聚焦操作的世界空间包围球。
struct WorldBounds {
    glm::vec3 center{0.0F};
    float radius{0.0F};
};

// API 无关的 CPU 场景拾取器；只读取 RenderFrame 和 CPU 网格资产。
class ScenePicker {
public:
    // 从视口像素生成射线，并返回距离相机最近的三角形命中。
    [[nodiscard]] static std::optional<ScenePickResult>
    pick(const RenderFrame& frame, const AssetRegistry& registry,
         float viewportX, float viewportY, int viewportWidth,
         int viewportHeight);

    // 汇总实体所有网格顶点，计算其世界空间包围球。
    [[nodiscard]] static std::optional<WorldBounds>
    worldBounds(const RenderFrame& frame, const AssetRegistry& registry,
                EntityId entity);
};

} // namespace renderlab
