#include "editor/TransformCommand.h"

#include "scene/SceneDocument.h"

#include <utility>

namespace renderlab {

TransformCommand::TransformCommand(SceneDocument& scene, const EntityId entity,
                                   const TransformComponent& before,
                                   const TransformComponent& after, QString text,
                                   QUndoCommand* parent)
    : QUndoCommand(std::move(text), parent), scene_(&scene), entity_(entity), before_(before),
      after_(after) {}

void TransformCommand::undo() {
    apply(before_);
}

void TransformCommand::redo() {
    apply(after_);
}

void TransformCommand::apply(const TransformComponent& transform) {
    if (scene_ == nullptr || !scene_->contains(entity_)) {
        return;
    }

    scene_->setPosition(entity_, transform.position);
    scene_->setRotation(entity_, transform.rotationDegrees);
    scene_->setScale(entity_, transform.scale);
}

} // namespace renderlab
