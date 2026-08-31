#include "editor/EntityCommands.h"

#include "scene/SceneDocument.h"

#include <stdexcept>
#include <utility>

namespace renderlab {

CreateEntityCommand::CreateEntityCommand(SceneDocument& scene, EntitySnapshot snapshot,
                                         QString text, QUndoCommand* parent)
    : QUndoCommand(std::move(text), parent), scene_(&scene), snapshot_(std::move(snapshot)) {}

void CreateEntityCommand::undo() {
    if (scene_ != nullptr && entity_ != NullEntity) {
        scene_->destroyEntity(entity_);
    }
}

void CreateEntityCommand::redo() {
    if (scene_ == nullptr) {
        return;
    }
    entity_ = restoreEntitySnapshot(*scene_, snapshot_, initialized_);
    if (!initialized_ && entity_ != NullEntity) {
        snapshot_ = *captureEntitySnapshot(*scene_, entity_);
        initialized_ = true;
    }
}

EntityId CreateEntityCommand::entity() const noexcept {
    return entity_;
}

DeleteEntityCommand::DeleteEntityCommand(SceneDocument& scene, const EntityId entity,
                                         QString text, QUndoCommand* parent)
    : QUndoCommand(std::move(text), parent), scene_(&scene) {
    const auto snapshot = captureEntitySnapshot(scene, entity);
    if (!snapshot.has_value()) {
        throw std::invalid_argument("Cannot delete an unknown entity");
    }
    snapshot_ = *snapshot;
}

void DeleteEntityCommand::undo() {
    if (scene_ != nullptr) {
        (void)restoreEntitySnapshot(*scene_, snapshot_, true);
    }
}

void DeleteEntityCommand::redo() {
    if (scene_ != nullptr) {
        scene_->destroyEntity(snapshot_.id);
    }
}

EntityId DeleteEntityCommand::entity() const noexcept {
    return snapshot_.id;
}

DuplicateEntityCommand::DuplicateEntityCommand(SceneDocument& scene, const EntityId source,
                                               QString text, QUndoCommand* parent)
    : QUndoCommand(std::move(text), parent), scene_(&scene) {
    const auto snapshot = captureEntitySnapshot(scene, source);
    if (!snapshot.has_value()) {
        throw std::invalid_argument("Cannot duplicate an unknown entity");
    }
    snapshot_ = *snapshot;
    snapshot_.name += " Copy";
}

void DuplicateEntityCommand::undo() {
    if (scene_ != nullptr && entity_ != NullEntity) {
        scene_->destroyEntity(entity_);
    }
}

void DuplicateEntityCommand::redo() {
    if (scene_ == nullptr) {
        return;
    }
    entity_ = restoreEntitySnapshot(*scene_, snapshot_, initialized_);
    if (!initialized_ && entity_ != NullEntity) {
        snapshot_ = *captureEntitySnapshot(*scene_, entity_);
        initialized_ = true;
    }
}

EntityId DuplicateEntityCommand::entity() const noexcept {
    return entity_;
}

RenameEntityCommand::RenameEntityCommand(SceneDocument& scene, const EntityId entity,
                                         std::string before, std::string after, QString text,
                                         QUndoCommand* parent)
    : QUndoCommand(std::move(text), parent), scene_(&scene), entity_(entity),
      before_(std::move(before)), after_(std::move(after)) {}

void RenameEntityCommand::undo() {
    if (scene_ != nullptr) {
        scene_->setName(entity_, before_);
    }
}

void RenameEntityCommand::redo() {
    if (scene_ != nullptr) {
        scene_->setName(entity_, after_);
    }
}

} // namespace renderlab
