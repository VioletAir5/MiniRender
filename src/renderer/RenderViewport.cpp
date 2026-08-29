#include "renderer/RenderViewport.h"

#include "renderer/surfaces/IRenderSurface.h"
#include "renderer/surfaces/OpenGLRenderSurface.h"

#include "scene/SceneDocument.h"

#include <QEvent>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QWheelEvent>

#include <utility>

namespace renderlab {

RenderViewport::RenderViewport(const AssetRegistry& registry, QWidget* parent)
    : QWidget(parent),
      surface_(std::make_unique<OpenGLRenderSurface>(registry, this)) {
    setMinimumSize(640, 360);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    // 输入事件发生在内部 surface 控件上，由外层视口统一解释为相机操作。
    surface_->widget().installEventFilter(this);
    layout->addWidget(&surface_->widget());
}

RenderViewport::~RenderViewport() = default;

void RenderViewport::setScene(const SceneDocument* scene) {
    scene_ = scene;
    requestRender();
}

void RenderViewport::requestRender() {
    QWidget& surfaceWidget = surface_->widget();
    RenderFrame frame;
    if (scene_ != nullptr) {
        const RenderView view =
            editorCamera_.renderView(surfaceWidget.width(), surfaceWidget.height());
        frame = sceneRenderer_.buildFrame(*scene_, view);
    }

    surface_->setFrame(std::move(frame));
    surface_->requestRender();
}

bool RenderViewport::eventFilter(QObject* watched, QEvent* event) {
    QWidget& surfaceWidget = surface_->widget();
    if (watched != &surfaceWidget) {
        return QWidget::eventFilter(watched, event);
    }

    if (event->type() == QEvent::Resize) {
        requestRender();
        return QWidget::eventFilter(watched, event);
    }

    if (event->type() == QEvent::MouseButtonPress) {
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::MiddleButton) {
            lastMousePosition_ = mouseEvent->position().toPoint();
            surfaceWidget.setCursor(Qt::ClosedHandCursor);
            event->accept();
            return true;
        }
    }

    if (event->type() == QEvent::MouseMove) {
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->buttons().testFlag(Qt::MiddleButton)) {
            const QPoint currentPosition = mouseEvent->position().toPoint();
            const QPoint delta = currentPosition - lastMousePosition_;
            lastMousePosition_ = currentPosition;

            if (mouseEvent->modifiers().testFlag(Qt::ShiftModifier)) {
                editorCamera_.pan(static_cast<float>(delta.x()),
                                  static_cast<float>(delta.y()),
                                  surfaceWidget.height());
            } else {
                editorCamera_.orbit(static_cast<float>(delta.x()),
                                    static_cast<float>(delta.y()));
            }

            requestRender();
            event->accept();
            return true;
        }
    }

    if (event->type() == QEvent::MouseButtonRelease) {
        auto* mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::MiddleButton) {
            surfaceWidget.unsetCursor();
            event->accept();
            return true;
        }
    }

    if (event->type() == QEvent::Wheel) {
        auto* wheelEvent = static_cast<QWheelEvent*>(event);
        // Qt 标准滚轮的一步通常是 120 个 angleDelta 单位。
        const int wheelDelta = wheelEvent->angleDelta().y();
        if (wheelDelta != 0) {
            editorCamera_.zoom(static_cast<float>(wheelDelta) / 120.0F);
            requestRender();
            event->accept();
            return true;
        }
    }

    return QWidget::eventFilter(watched, event);
}

} // namespace renderlab

