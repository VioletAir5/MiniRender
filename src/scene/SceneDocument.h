#pragma once

#include "scene/Components.h"
#include "scene/Entity.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace renderlab {

// 拥有场景实体、层级关系及其组件，是编辑器场景数据的唯一写入入口。
class SceneDocument {
  public:
    SceneDocument() = default;

    // 创建带默认 TransformComponent 的实体；parent 非空时必须已存在。
    EntityId createEntity(std::string name, EntityId parent = NullEntity);
    // 递归销毁实体、全部后代及其组件；实体不存在时返回 false。
    bool destroyEntity(EntityId entity);
    // 判断实体标识当前是否属于该场景。
    [[nodiscard]] bool contains(EntityId entity) const;

    // 查找实体元数据；不存在时返回 nullptr。
    [[nodiscard]] EntityMetadata* tryGetEntity(EntityId entity);
    [[nodiscard]] const EntityMetadata* tryGetEntity(EntityId entity) const;

    // 查找实体的变换组件；不存在时返回 nullptr。
    [[nodiscard]] TransformComponent* tryGetTransform(EntityId entity);
    [[nodiscard]] const TransformComponent* tryGetTransform(EntityId entity) const;

    // 为已有实体创建或返回网格渲染组件；实体不存在时抛出异常。
    MeshRendererComponent& addMeshRenderer(EntityId entity);
    // 为已有实体创建或返回相机组件；实体不存在时抛出异常。
    CameraComponent& addCamera(EntityId entity);
    // 为已有实体创建或返回光源组件；实体不存在时抛出异常。
    LightComponent& addLight(EntityId entity);

    // 查找对应的可选组件；实体或组件不存在时返回 nullptr。
    [[nodiscard]] MeshRendererComponent* tryGetMeshRenderer(EntityId entity);
    [[nodiscard]] const MeshRendererComponent* tryGetMeshRenderer(EntityId entity) const;
    [[nodiscard]] CameraComponent* tryGetCamera(EntityId entity);
    [[nodiscard]] const CameraComponent* tryGetCamera(EntityId entity) const;
    [[nodiscard]] LightComponent* tryGetLight(EntityId entity);
    [[nodiscard]] const LightComponent* tryGetLight(EntityId entity) const;

    // 修改父节点并同步维护父子两侧；非法实体或形成环时返回 false。
    bool setParent(EntityId entity, EntityId newParent);

    // 返回当前全部根实体，顺序不构成稳定接口。
    [[nodiscard]] std::vector<EntityId> rootEntities() const;
    // 只读访问全部实体，主要供场景遍历和编辑器面板使用。
    [[nodiscard]] const std::unordered_map<EntityId, EntityMetadata>& entities() const noexcept;

    // 原子替换完整局部变换；实体不存在时返回 false。
    bool setTransform(EntityId entity, const TransformComponent& transform);
    bool setPosition(EntityId entity, const glm::vec3& position);
    bool setRotation(EntityId entity, const glm::vec3& rotation);
    bool setScale(EntityId entity, const glm::vec3& scale);

  private:
    // 沿候选父节点向上检查，判断重新挂接是否会形成层级环。
    [[nodiscard]] bool wouldCreateCycle(EntityId entity, EntityId newParent) const;
    // 从旧父节点的 children 中移除实体，但不修改实体自身的 parent。
    void removeFromParent(EntityId entity);
    // 验证实体存在；失败时抛出 std::out_of_range。
    void requireEntity(EntityId entity) const;

    // 单调递增的实体编号生成器，零值被保留为 NullEntity。
    EntityId nextEntityId_{1};

    std::unordered_map<EntityId, EntityMetadata> entities_;
    std::unordered_map<EntityId, TransformComponent> transforms_;
    std::unordered_map<EntityId, MeshRendererComponent> meshRenderers_;
    std::unordered_map<EntityId, CameraComponent> cameras_;
    std::unordered_map<EntityId, LightComponent> lights_;
};

} // namespace renderlab
