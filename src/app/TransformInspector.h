#pragma once

#include "scene/Components.h"
#include "scene/EntityId.h"

#include <QString>
#include <QWidget>

#include <array>

class QDoubleSpinBox;
class QGroupBox;
class QLabel;
class QSlider;
class QUndoStack;

namespace renderlab {

class SceneDocument;

// 编辑当前实体的局部 Transform，并把修改通过 SceneDocument 的写接口提交。
class TransformInspector final : public QWidget {
    Q_OBJECT

  public:
    explicit TransformInspector(QWidget* parent = nullptr);

    // 撤销栈不归 Inspector 所有；未设置时仍可直接预览编辑。
    void setUndoStack(QUndoStack* undoStack);
    // 切换当前实体并刷新全部输入框；无效实体会禁用 Transform 编辑区域。
    void setEntity(SceneDocument* scene, EntityId entity);
    // 在执行菜单撤销/重做前提交仍在进行的数字输入。
    void commitPendingEdit();

  signals:
    // Transform 成功写入场景后发出，供视口刷新及后续脏标记系统使用。
    void transformEdited(EntityId entity);

  private:
    // 从场景重新加载摘要及局部 Transform，刷新期间不会产生编辑信号。
    void refresh();
    bool beginTransformEdit(const QString& text);
    void endTransformEdit();
    void applyPosition();
    void applyRotation();
    void applyScale();

    SceneDocument* scene_{nullptr};
    QUndoStack* undoStack_{nullptr};
    EntityId entity_{NullEntity};

    SceneDocument* editScene_{nullptr};
    EntityId editEntity_{NullEntity};
    TransformComponent editBefore_;
    QString editText_;
    bool editActive_{false};

    QLabel* summaryLabel_{nullptr};
    QGroupBox* transformGroup_{nullptr};
    std::array<QDoubleSpinBox*, 3> positionFields_{};
    std::array<QDoubleSpinBox*, 3> rotationFields_{};
    std::array<QDoubleSpinBox*, 3> scaleFields_{};
    std::array<QSlider*, 3> positionSliders_{};
    std::array<QSlider*, 3> rotationSliders_{};
    std::array<QSlider*, 3> scaleSliders_{};
};

} // namespace renderlab
