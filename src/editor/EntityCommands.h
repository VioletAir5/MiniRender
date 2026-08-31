#pragma once

#include "scene/EntitySnapshot.h"

#include <QUndoCommand>

#include <QString>

#include <string>

namespace renderlab {

class SceneDocument;

// 创建一个实体子树；首次执行分配新 ID，之后重做恢复相同 ID。
class CreateEntityCommand final : public QUndoCommand {
  public:
    CreateEntityCommand(SceneDocument& scene, EntitySnapshot snapshot, QString text,
                        QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    [[nodiscard]] EntityId entity() const noexcept;

  private:
    SceneDocument* scene_{nullptr};
    EntitySnapshot snapshot_;
    EntityId entity_{NullEntity};
    bool initialized_{false};
};

// 删除一个实体子树，并保留完整快照供撤销恢复。
class DeleteEntityCommand final : public QUndoCommand {
  public:
    DeleteEntityCommand(SceneDocument& scene, EntityId entity, QString text,
                        QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    [[nodiscard]] EntityId entity() const noexcept;

  private:
    SceneDocument* scene_{nullptr};
    EntitySnapshot snapshot_;
};

// 复制实体子树，首次执行生成一组新 ID，撤销后重做保持这些 ID。
class DuplicateEntityCommand final : public QUndoCommand {
  public:
    DuplicateEntityCommand(SceneDocument& scene, EntityId source, QString text,
                           QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;
    [[nodiscard]] EntityId entity() const noexcept;

  private:
    SceneDocument* scene_{nullptr};
    EntitySnapshot snapshot_;
    EntityId entity_{NullEntity};
    bool initialized_{false};
};

// 保存一次实体名称修改。
class RenameEntityCommand final : public QUndoCommand {
  public:
    RenameEntityCommand(SceneDocument& scene, EntityId entity, std::string before,
                        std::string after, QString text, QUndoCommand* parent = nullptr);

    void undo() override;
    void redo() override;

  private:
    SceneDocument* scene_{nullptr};
    EntityId entity_{NullEntity};
    std::string before_;
    std::string after_;
};

} // namespace renderlab
