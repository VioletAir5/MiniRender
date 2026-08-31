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

    // 完整快照通过单一入口写回，避免验证规则变化后产生部分应用。
    scene_->setTransform(entity_, transform);
}

} // namespace renderlab
