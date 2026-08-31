#pragma once

#include "scene/EntityId.h"

#include <QWidget>

#include <array>

class QDoubleSpinBox;
class QGroupBox;
class QLabel;
class QSlider;

namespace renderlab {

class SceneDocument;

// 编辑当前实体的局部 Transform，并把修改通过 SceneDocument 的写接口提交。
class TransformInspector final : public QWidget {
    Q_OBJECT

  public:
    explicit TransformInspector(QWidget* parent = nullptr);

    // 切换当前实体并刷新全部输入框；无效实体会禁用 Transform 编辑区域。
    void setEntity(SceneDocument* scene, EntityId entity);

  signals:
    // Transform 成功写入场景后发出，供视口刷新及后续脏标记系统使用。
    void transformEdited(EntityId entity);

  private:
    // 从场景重新加载摘要及局部 Transform，刷新期间不会产生编辑信号。
    void refresh();
    void applyPosition();
    void applyRotation();
    void applyScale();

    SceneDocument* scene_{nullptr};
    EntityId entity_{NullEntity};
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