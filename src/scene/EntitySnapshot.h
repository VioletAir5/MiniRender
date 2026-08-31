#pragma once

#include "scene/Components.h"

#include <optional>
#include <string>
#include <vector>

namespace renderlab {

class SceneDocument;

// 保存一个实体子树的可恢复数据，不包含任何界面或渲染后端状态。
struct EntitySnapshot {
    EntityId id{NullEntity};
    EntityId parent{NullEntity};
    std::string name{"Entity"};
    TransformComponent transform;
    std::optional<MeshRendererComponent> meshRenderer;
    std::optional<CameraComponent> camera;
    std::optional<LightComponent> light;
    std::vector<EntitySnapshot> children;
};

// 递归捕获实体及其全部后代；实体不存在时返回空值。
[[nodiscard]] std::optional<EntitySnapshot> captureEntitySnapshot(const SceneDocument& scene,
                                                                  EntityId entity);

// 恢复实体子树。preserveIds 用于撤销删除；复制和首次创建应传 false。
[[nodiscard]] EntityId restoreEntitySnapshot(SceneDocument& scene,
                                             const EntitySnapshot& snapshot,
                                             bool preserveIds);

} // namespace renderlab
