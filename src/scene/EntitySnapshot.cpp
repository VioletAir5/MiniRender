#include "scene/EntitySnapshot.h"

#include "scene/SceneDocument.h"

#include <unordered_set>
#include <utility>

namespace renderlab {
namespace {

std::optional<EntitySnapshot> capture(const SceneDocument& scene, const EntityId entity) {
    const EntityMetadata* metadata = scene.tryGetEntity(entity);
    const TransformComponent* transform = scene.tryGetTransform(entity);
    if (metadata == nullptr || transform == nullptr) {
        return std::nullopt;
    }

    EntitySnapshot snapshot{
        .id = metadata->id,
        .parent = metadata->parent,
        .name = metadata->name,
        .transform = *transform,
    };
    if (const auto* component = scene.tryGetMeshRenderer(entity); component != nullptr) {
        snapshot.meshRenderer = *component;
    }
    if (const auto* component = scene.tryGetCamera(entity); component != nullptr) {
        snapshot.camera = *component;
    }
    if (const auto* component = scene.tryGetLight(entity); component != nullptr) {
        snapshot.light = *component;
    }

    snapshot.children.reserve(metadata->children.size());
    for (const EntityId child : metadata->children) {
        auto childSnapshot = capture(scene, child);
        if (!childSnapshot.has_value()) {
            return std::nullopt;
        }
        snapshot.children.push_back(std::move(*childSnapshot));
    }
    return snapshot;
}

bool validatePreservedIds(const SceneDocument& scene, const EntitySnapshot& snapshot,
                          std::unordered_set<EntityId>& ids) {
    if (snapshot.id == NullEntity || scene.contains(snapshot.id) || !ids.insert(snapshot.id).second) {
        return false;
    }
    for (const EntitySnapshot& child : snapshot.children) {
        if (!validatePreservedIds(scene, child, ids)) {
            return false;
        }
    }
    return true;
}

EntityId restore(SceneDocument& scene, const EntitySnapshot& snapshot, const EntityId parent,
                 const bool preserveIds) {
    const EntityId entity = preserveIds
                                ? scene.restoreEntity(snapshot.id, snapshot.name, parent)
                                : scene.createEntity(snapshot.name, parent);
    if (entity == NullEntity) {
        return NullEntity;
    }

    (void)scene.setTransform(entity, snapshot.transform);
    if (snapshot.meshRenderer.has_value()) {
        scene.addMeshRenderer(entity) = *snapshot.meshRenderer;
    }
    if (snapshot.camera.has_value()) {
        scene.addCamera(entity) = *snapshot.camera;
    }
    if (snapshot.light.has_value()) {
        scene.addLight(entity) = *snapshot.light;
    }

    for (const EntitySnapshot& child : snapshot.children) {
        if (restore(scene, child, entity, preserveIds) == NullEntity) {
            // 前置验证保证正常情况下不会部分失败；保留防御性清理避免残缺子树。
            scene.destroyEntity(entity);
            return NullEntity;
        }
    }
    return entity;
}

} // namespace

std::optional<EntitySnapshot> captureEntitySnapshot(const SceneDocument& scene,
                                                    const EntityId entity) {
    return capture(scene, entity);
}

EntityId restoreEntitySnapshot(SceneDocument& scene, const EntitySnapshot& snapshot,
                               const bool preserveIds) {
    if (snapshot.parent != NullEntity && !scene.contains(snapshot.parent)) {
        return NullEntity;
    }
    if (preserveIds) {
        std::unordered_set<EntityId> ids;
        if (!validatePreservedIds(scene, snapshot, ids)) {
            return NullEntity;
        }
    }
    return restore(scene, snapshot, snapshot.parent, preserveIds);
}

} // namespace renderlab
