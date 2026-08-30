#pragma once

#include "scene/Components.h"

#include <glm/mat4x4.hpp>

namespace renderlab {

class SceneDocument;

// 按场景统一约定构造实体局部变换矩阵。
[[nodiscard]] glm::mat4 localTransformMatrix(const TransformComponent& transform);

// 递归累乘父节点变换，返回实体的世界空间矩阵。
[[nodiscard]] glm::mat4 worldTransformMatrix(const SceneDocument& scene,
                                             EntityId entity);

} // namespace renderlab
