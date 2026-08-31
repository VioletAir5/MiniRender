#pragma once

#include "scene/Components.h"
#include "scene/EntityId.h"

#include <QUndoCommand>

#include <QString>

namespace renderlab {

class SceneDocument;

// 保存一次完整的局部 Transform 修改，供 Inspector 和未来的视口 Gizmo 共同使用。
class TransformCommand final : public QUndoCommand {
  public:
    TransformCommand(SceneDocument& scene, EntityId entity, const TransformComponent& before,
                     const TransformComponent& after, QString text, QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;

  private:
    void apply(const TransformComponent& transform);

    SceneDocument* scene_{nullptr};
    EntityId entity_{NullEntity};
    TransformComponent before_;
    TransformComponent after_;
};

} // namespace renderlab
