#include "scene/SceneDocument.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace renderlab {

EntityId SceneDocument::createEntity(std::string name, const EntityId parent) {
    if (parent != NullEntity && !contains(parent)) {
        throw std::invalid_argument("Cannot create an entity under an unknown parent");
    }

    const EntityId id = nextEntityId_++;
    entities_.emplace(id, EntityMetadata{id, std::move(name), NullEntity, {}});
    transforms_.emplace(id, TransformComponent{});

    if (parent != NullEntity) {
        setParent(id, parent);
    }

    return id;
}

EntityId SceneDocument::restoreEntity(const EntityId entity, std::string name,
                                      const EntityId parent) {
    if (entity == NullEntity || contains(entity) ||
        (parent != NullEntity && !contains(parent))) {
        return NullEntity;
    }

    entities_.emplace(entity, EntityMetadata{entity, std::move(name), NullEntity, {}});
    transforms_.emplace(entity, TransformComponent{});
    nextEntityId_ = std::max(nextEntityId_, entity + 1);

    if (parent != NullEntity) {
        setParent(entity, parent);
    }
    return entity;
}

bool SceneDocument::destroyEntity(const EntityId entity) {
    const auto iterator = entities_.find(entity);
    if (iterator == entities_.end()) {
        return false;
    }

    // 复制子列表后再递归，避免 erase 导致正在遍历的容器失效。
    const std::vector<EntityId> children = iterator->second.children;
    for (const EntityId child : children) {
        destroyEntity(child);
    }

    removeFromParent(entity);
    transforms_.erase(entity);
    meshRenderers_.erase(entity);
    cameras_.erase(entity);
    lights_.erase(entity);
    entities_.erase(entity);
    return true;
}

bool SceneDocument::setName(const EntityId entity, std::string name) {
    EntityMetadata* metadata = tryGetEntity(entity);
    if (metadata == nullptr) {
        return false;
    }
    metadata->name = std::move(name);
    return true;
}

bool SceneDocument::contains(const EntityId entity) const {
    return entity != NullEntity && entities_.contains(entity);
}

EntityMetadata* SceneDocument::tryGetEntity(const EntityId entity) {
    const auto iterator = entities_.find(entity);
    return iterator == entities_.end() ? nullptr : &iterator->second;
}

const EntityMetadata* SceneDocument::tryGetEntity(const EntityId entity) const {
    const auto iterator = entities_.find(entity);
    return iterator == entities_.end() ? nullptr : &iterator->second;
}

TransformComponent* SceneDocument::tryGetTransform(const EntityId entity) {
    const auto iterator = transforms_.find(entity);
    return iterator == transforms_.end() ? nullptr : &iterator->second;
}

const TransformComponent* SceneDocument::tryGetTransform(const EntityId entity) const {
    const auto iterator = transforms_.find(entity);
    return iterator == transforms_.end() ? nullptr : &iterator->second;
}

MeshRendererComponent& SceneDocument::addMeshRenderer(const EntityId entity) {
    requireEntity(entity);
    return meshRenderers_.try_emplace(entity).first->second;
}

CameraComponent& SceneDocument::addCamera(const EntityId entity) {
    requireEntity(entity);
    return cameras_.try_emplace(entity).first->second;
}

LightComponent& SceneDocument::addLight(const EntityId entity) {
    requireEntity(entity);
    return lights_.try_emplace(entity).first->second;
}

MeshRendererComponent* SceneDocument::tryGetMeshRenderer(const EntityId entity) {
    const auto iterator = meshRenderers_.find(entity);
    return iterator == meshRenderers_.end() ? nullptr : &iterator->second;
}

const MeshRendererComponent* SceneDocument::tryGetMeshRenderer(const EntityId entity) const {
    const auto iterator = meshRenderers_.find(entity);
    return iterator == meshRenderers_.end() ? nullptr : &iterator->second;
}

CameraComponent* SceneDocument::tryGetCamera(const EntityId entity) {
    const auto iterator = cameras_.find(entity);
    return iterator == cameras_.end() ? nullptr : &iterator->second;
}

const CameraComponent* SceneDocument::tryGetCamera(const EntityId entity) const {
    const auto iterator = cameras_.find(entity);
    return iterator == cameras_.end() ? nullptr : &iterator->second;
}

LightComponent* SceneDocument::tryGetLight(const EntityId entity) {
    const auto iterator = lights_.find(entity);
    return iterator == lights_.end() ? nullptr : &iterator->second;
}

const LightComponent* SceneDocument::tryGetLight(const EntityId entity) const {
    const auto iterator = lights_.find(entity);
    return iterator == lights_.end() ? nullptr : &iterator->second;
}

bool SceneDocument::setParent(const EntityId entity, const EntityId newParent) {
    if (!contains(entity) || (newParent != NullEntity && !contains(newParent)) ||
        entity == newParent || wouldCreateCycle(entity, newParent)) {
        return false;
    }

    EntityMetadata& metadata = entities_.at(entity);
    if (metadata.parent == newParent) {
        return true;
    }

    // 先从旧父节点移除，再同时写入新关系，维持父子两侧一致。
    removeFromParent(entity);
    metadata.parent = newParent;

    if (newParent != NullEntity) {
        entities_.at(newParent).children.push_back(entity);
    }

    return true;
}

std::vector<EntityId> SceneDocument::rootEntities() const {
    std::vector<EntityId> roots;
    roots.reserve(entities_.size());

    for (const auto& [id, metadata] : entities_) {
        if (metadata.parent == NullEntity) {
            roots.push_back(id);
        }
    }

    std::ranges::sort(roots);
    return roots;
}

const std::unordered_map<EntityId, EntityMetadata>& SceneDocument::entities() const noexcept {
    return entities_;
}

bool SceneDocument::wouldCreateCycle(const EntityId entity, EntityId newParent) const {
    // 只要候选父节点的祖先链回到 entity，就会形成环。
    while (newParent != NullEntity) {
        if (newParent == entity) {
            return true;
        }

        const EntityMetadata* metadata = tryGetEntity(newParent);
        if (metadata == nullptr) {
            return false;
        }

        newParent = metadata->parent;
    }

    return false;
}

void SceneDocument::removeFromParent(const EntityId entity) {
    EntityMetadata* metadata = tryGetEntity(entity);
    if (metadata == nullptr || metadata->parent == NullEntity) {
        return;
    }

    EntityMetadata* parent = tryGetEntity(metadata->parent);
    if (parent != nullptr) {
        std::erase(parent->children, entity);
    }

    metadata->parent = NullEntity;
}

void SceneDocument::requireEntity(const EntityId entity) const {
    if (!contains(entity)) {
        throw std::out_of_range("Entity does not exist in this scene");
    }
}

bool SceneDocument::setTransform(const EntityId entity,
                                 const TransformComponent& transform) {
    TransformComponent* current = tryGetTransform(entity);
    if (current == nullptr) {
        return false;
    }
    *current = transform;
    return true;
}

bool SceneDocument::setPosition(const EntityId entity, const glm::vec3& position) {
    TransformComponent* transform = tryGetTransform(entity);
    if (transform == nullptr) {
        return false;
    }
    transform->position = position;
    return true;
}

bool SceneDocument::setRotation(const EntityId entity, const glm::vec3& rotation) {
    TransformComponent* transform = tryGetTransform(entity);
    if (transform == nullptr) {
        return false;
    }
    transform->rotationDegrees = rotation;
    return true;
}

bool SceneDocument::setScale(const EntityId entity, const glm::vec3& scale) {
    TransformComponent* transform = tryGetTransform(entity);
    if (transform == nullptr) {
        return false;
    }
    transform->scale = scale;
    return true;
}
} // namespace renderlab
