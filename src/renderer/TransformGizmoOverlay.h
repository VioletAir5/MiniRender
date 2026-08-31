#pragma once

#include "editor/TranslateGizmo.h"

#include <QWidget>

namespace renderlab {

// 透明覆盖层只绘制 Gizmo；输入仍由 RenderViewport 的事件过滤器统一处理。
class TransformGizmoOverlay final : public QWidget {
  public:
    explicit TransformGizmoOverlay(QWidget* parent = nullptr);
    void setGeometryData(const TranslateGizmoGeometry& geometry,
                         GizmoAxis activeAxis = GizmoAxis::None,
                         GizmoMode mode = GizmoMode::Translate);

  protected:
    void paintEvent(QPaintEvent* event) override;

  private:
    TranslateGizmoGeometry geometry_;
    GizmoAxis activeAxis_{GizmoAxis::None};
    GizmoMode mode_{GizmoMode::Translate};
};

} // namespace renderlab
