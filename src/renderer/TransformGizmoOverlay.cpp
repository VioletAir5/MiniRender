#include "renderer/TransformGizmoOverlay.h"

#include <QPainter>
#include <QPainterPath>

#include <array>

namespace renderlab {

TransformGizmoOverlay::TransformGizmoOverlay(QWidget* parent) : QWidget(parent) {
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TranslucentBackground);
}

void TransformGizmoOverlay::setGeometryData(const TranslateGizmoGeometry& geometry,
                                            const GizmoAxis activeAxis) {
    geometry_ = geometry;
    activeAxis_ = activeAxis;
    update();
}

void TransformGizmoOverlay::paintEvent(QPaintEvent*) {
    if (!geometry_.visible) return;
    static const std::array<QColor, 3> colors{
        QColor{224, 80, 90}, QColor{80, 200, 100}, QColor{80, 140, 235}};
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    const QPointF origin{geometry_.origin.x, geometry_.origin.y};
    for (std::size_t index = 0; index < geometry_.endpoints.size(); ++index) {
        const GizmoAxis axis = static_cast<GizmoAxis>(static_cast<int>(index) + 1);
        QColor color = colors[index];
        if (activeAxis_ == axis) color = color.lighter(160);
        QPen pen{color, activeAxis_ == axis ? 5.0 : 3.0, Qt::SolidLine, Qt::RoundCap};
        painter.setPen(pen);
        const QPointF endpoint{geometry_.endpoints[index].x, geometry_.endpoints[index].y};
        painter.drawLine(origin, endpoint);
        painter.setBrush(color);
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(endpoint, 5.0, 5.0);
    }
}

} // namespace renderlab
