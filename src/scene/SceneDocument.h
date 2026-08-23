#pragma once

#include "scene/Components.h"
#include "scene/Entity.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace renderlab {

class SceneDocument {
public:
    SceneDocument() = default;

    EntityId createEntity(std::string name, EntityId parent = NullEntity);
    bool destroyEntity(EntityId entity);
    [[nodiscard]] bool contains(EntityId entity) const;

    [[nodiscard]] EntityMetadata* tryGetEntity(EntityId entity);
    [[nodiscard]] const EntityMetadata* tryGetEntity(EntityId entity) const;

    [[nodiscard]] TransformComponent* tryGetTransform(EntityId entity);
    [[nodiscard]] const TransformComponent* tryGetTransform(EntityId entity) const;

    MeshRendererComponent& addMeshRenderer(EntityId entity);
    CameraComponent& addCamera(EntityId entity);
    LightComponent& addLight(EntityId entity);

    [[nodiscard]] MeshRendererComponent* tryGetMeshRenderer(EntityId entity);
    [[nodiscard]] const MeshRendererComponent* tryGetMeshRenderer(EntityId entity) const;
    [[nodiscard]] CameraComponent* tryGetCamera(EntityId entity);
    [[nodiscard]] const CameraComponent* tryGetCamera(EntityId entity) const;
    [[nodiscard]] LightComponent* tryGetLight(EntityId entity);
    [[nodiscard]] const LightComponent* tryGetLight(EntityId entity) const;

    bool setParent(EntityId entity, EntityId newParent);

    [[nodiscard]] std::vector<EntityId> rootEntities() const;
    [[nodiscard]] const std::unordered_map<EntityId, EntityMetadata>& entities() const noexcept;

private:
    [[nodiscard]] bool wouldCreateCycle(EntityId entity, EntityId newParent) const;
    void removeFromParent(EntityId entity);
    void requireEntity(EntityId entity) const;

    EntityId nextEntityId_{1};

    std::unordered_map<EntityId, EntityMetadata> entities_;
    std::unordered_map<EntityId, TransformComponent> transforms_;
    std::unordered_map<EntityId, MeshRendererComponent> meshRenderers_;
    std::unordered_map<EntityId, CameraComponent> cameras_;
    std::unordered_map<EntityId, LightComponent> lights_;
};

} // namespace renderlab

