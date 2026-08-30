#include "renderer/RenderViewport.h"

#include "editor/ScenePicker.h"

#include "renderer/surfaces/IRenderSurface.h"
#include "renderer/surfaces/OpenGLRenderSurface.h"

#include "scene/SceneDocument.h"
#include "scene/TransformUtils.h"

#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <glm/mat4x4.hpp>
#include <optional>

#include <utility>

namespace renderlab {
namespace {

// 右键上下拖动时，把像素位移换算为 EditorCamera::zoom 的滚轮步数。
constexpr float kDragZoomStepsPerPixel = 0.02F;

} // namespace

RenderViewport::RenderViewport(const AssetRegistry& registry, QWidget* parent)
    : QWidget(parent),
      registry_(registry),
      surface_(std::make_unique<OpenGLRenderSurface>(registry, this)) {
    setMinimumSize(640, 360);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    QWidget& surfaceWidget = surface_->widget();
    // 输入事件发生在内部 surface 控件上，由外层视口统一解释为相机操作。
    surfaceWidget.installEventFilter(this);
    surfaceWidget.setFocusPolicy(Qt::StrongFocus);
    layout->addWidget(&surfaceWidget);
}

RenderViewport::~RenderViewport() {
    cancelNavigation();
}

void RenderViewport::setScene(const SceneDocument* scene) {
    scene_ = scene;
    if (scene_ == nullptr || !scene_->contains(selectedEntity_)) {
        selectedEntity_ = NullEntity;
    }
    requestRender();
}

void RenderViewport::setSelectedEntity(const EntityId entity) {
    selectedEntity_ =
        scene_ != nullptr && scene_->contains(entity) ? entity : NullEntity;
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

    switch (event->type()) {
    case QEvent::Resize:
        requestRender();
        break;

    case QEvent::MouseButtonPress:
        if (beginNavigation(*static_cast<QMouseEvent*>(event))) {
            event->accept();
            return true;
        }
        if (auto& mouseEvent = *static_cast<QMouseEvent*>(event);
            mouseEvent.button() == Qt::LeftButton) {
            surfaceWidget.setFocus(Qt::MouseFocusReason);
            emit selectionRequested(
                pickEntity(mouseEvent.position().toPoint()));
            event->accept();
            return true;
        }
        break;

    case QEvent::MouseMove:
        if (updateNavigation(*static_cast<QMouseEvent*>(event))) {
            event->accept();
            return true;
        }
        break;

    case QEvent::MouseButtonRelease:
        if (endNavigation(*static_cast<QMouseEvent*>(event))) {
            event->accept();
            return true;
        }
        break;

    case QEvent::Wheel: {
        auto& wheelEvent = *static_cast<QWheelEvent*>(event);
        // Qt 标准滚轮的一步通常是 120 个 angleDelta 单位。
        const int wheelDelta = wheelEvent.angleDelta().y();
        if (wheelDelta != 0) {
            editorCamera_.zoom(static_cast<float>(wheelDelta) / 120.0F);
            requestRender();
            event->accept();
            return true;
        }
        break;
    }

    case QEvent::KeyPress: {
        auto& keyEvent = *static_cast<QKeyEvent*>(event);
        if (keyEvent.key() == Qt::Key_F) {
            focusSelection();
            event->accept();
            return true;
        }
        if (keyEvent.key() == Qt::Key_Escape &&
            navigationMode_ != NavigationMode::None) {
            cancelNavigation();
            event->accept();
            return true;
        }
        break;
    }

    case QEvent::FocusOut:
    case QEvent::WindowDeactivate:
    case QEvent::Hide:
    case QEvent::UngrabMouse:
        cancelNavigation();
        break;

    default:
        break;
    }

    return QWidget::eventFilter(watched, event);
}

RenderViewport::NavigationMode RenderViewport::navigationModeFor(
    const QMouseEvent& event) noexcept {
    if (event.button() == Qt::MiddleButton) {
        return event.modifiers().testFlag(Qt::ShiftModifier)
                   ? NavigationMode::Pan
                   : NavigationMode::Orbit;
    }

    if (event.modifiers().testFlag(Qt::AltModifier)) {
        if (event.button() == Qt::LeftButton) {
            return NavigationMode::Orbit;
        }
        if (event.button() == Qt::RightButton) {
            return NavigationMode::Zoom;
        }
    }

    return NavigationMode::None;
}

bool RenderViewport::beginNavigation(QMouseEvent& event) {
    const NavigationMode mode = navigationModeFor(event);
    if (mode == NavigationMode::None) {
        return false;
    }

    cancelNavigation();

    QWidget& surfaceWidget = surface_->widget();
    navigationMode_ = mode;
    navigationButton_ = event.button();
    lastMousePosition_ = event.position().toPoint();

    surfaceWidget.setFocus(Qt::MouseFocusReason);
    surfaceWidget.grabMouse();

    switch (navigationMode_) {
    case NavigationMode::Orbit:
        surfaceWidget.setCursor(Qt::ClosedHandCursor);
        break;
    case NavigationMode::Pan:
        surfaceWidget.setCursor(Qt::SizeAllCursor);
        break;
    case NavigationMode::Zoom:
        surfaceWidget.setCursor(Qt::SizeVerCursor);
        break;
    case NavigationMode::None:
        break;
    }

    return true;
}

bool RenderViewport::updateNavigation(QMouseEvent& event) {
    if (navigationMode_ == NavigationMode::None) {
        return false;
    }

    // 防止窗口系统漏发 Release 后状态永久卡住。
    if (!event.buttons().testFlag(navigationButton_)) {
        cancelNavigation();
        return false;
    }

    const QPoint currentPosition = event.position().toPoint();
    const QPoint delta = currentPosition - lastMousePosition_;
    lastMousePosition_ = currentPosition;

    if (delta.isNull()) {
        return true;
    }

    switch (navigationMode_) {
    case NavigationMode::Orbit:
        editorCamera_.orbit(static_cast<float>(delta.x()),
                            static_cast<float>(delta.y()));
        break;
    case NavigationMode::Pan:
        editorCamera_.pan(static_cast<float>(delta.x()),
                          static_cast<float>(delta.y()),
                          surface_->widget().height());
        break;
    case NavigationMode::Zoom:
        editorCamera_.zoom(
            -static_cast<float>(delta.y()) * kDragZoomStepsPerPixel);
        break;
    case NavigationMode::None:
        return false;
    }

    requestRender();
    return true;
}

bool RenderViewport::endNavigation(QMouseEvent& event) {
    if (navigationMode_ == NavigationMode::None ||
        event.button() != navigationButton_) {
        return false;
    }

    cancelNavigation();
    return true;
}

void RenderViewport::cancelNavigation() {
    navigationMode_ = NavigationMode::None;
    navigationButton_ = Qt::NoButton;

    if (surface_ == nullptr) {
        return;
    }

    QWidget& surfaceWidget = surface_->widget();
    surfaceWidget.unsetCursor();

    // 只释放由当前表面持有的捕获，避免干扰其他 Qt 控件。
    if (QWidget::mouseGrabber() == &surfaceWidget) {
        surfaceWidget.releaseMouse();
    }
}

EntityId RenderViewport::pickEntity(const QPoint& viewportPosition) const {
    if (scene_ == nullptr) {
        return NullEntity;
    }

    // 使用与当前画面完全相同的相机和场景快照，避免可见结果与拾取不一致。
    const QWidget& surfaceWidget = surface_->widget();
    const RenderView view =
        editorCamera_.renderView(surfaceWidget.width(), surfaceWidget.height());
    const RenderFrame frame = sceneRenderer_.buildFrame(*scene_, view);
    const std::optional<ScenePickResult> result = ScenePicker::pick(
        frame, registry_, static_cast<float>(viewportPosition.x()),
        static_cast<float>(viewportPosition.y()), surfaceWidget.width(),
        surfaceWidget.height());
    return result.has_value() ? result->entity : NullEntity;
}

void RenderViewport::focusSelection() {
    if (scene_ != nullptr && scene_->contains(selectedEntity_)) {
        const QWidget& surfaceWidget = surface_->widget();
        const RenderView view = editorCamera_.renderView(
            surfaceWidget.width(), surfaceWidget.height());
        const RenderFrame frame = sceneRenderer_.buildFrame(*scene_, view);
        // 网格实体按实际世界包围球聚焦，使不同尺寸模型都能完整进入视野。
        const std::optional<WorldBounds> bounds =
            ScenePicker::worldBounds(frame, registry_, selectedEntity_);
        if (bounds.has_value()) {
            editorCamera_.focus(bounds->center, bounds->radius);
            requestRender();
            return;
        }

        // 无网格的相机、灯光或空实体仍可按其世界位置聚焦。
        const glm::mat4 world =
            worldTransformMatrix(*scene_, selectedEntity_);
        editorCamera_.focus(glm::vec3{world[3]}, 1.0F);
        requestRender();
        return;
    }

    editorCamera_.focus({0.0F, 0.0F, 0.0F}, 1.0F);
    requestRender();
}

} // namespace renderlab