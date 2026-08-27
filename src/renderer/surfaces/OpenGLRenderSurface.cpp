#include "renderer/surfaces/OpenGLRenderSurface.h"

#include "renderer/backends/opengl/OpenGLBackend.h"
#include "renderer/rhi/IRenderBackend.h"
#include "scene/SceneDocument.h"
#include <QMouseEvent>
#include <QWheelEvent>

namespace renderlab {

OpenGLRenderSurface::OpenGLRenderSurface(QWidget* parent)
    : QOpenGLWidget(parent),
      backend_(std::make_unique<OpenGLBackend>()) {
    setFocusPolicy(Qt::StrongFocus);
}

OpenGLRenderSurface::~OpenGLRenderSurface() {
    if (backend_ != nullptr && context() != nullptr) {
        makeCurrent();
        backend_->shutdown();
        doneCurrent();
    }
}

QWidget& OpenGLRenderSurface::widget() noexcept {
    return *this;
}

void OpenGLRenderSurface::setScene(const SceneDocument* scene) noexcept {
    scene_ = scene;
    update();
}

void OpenGLRenderSurface::requestRender() {
    update();
}

void OpenGLRenderSurface::initializeGL() {
    backendReady_ = backend_->initialize();
}

void OpenGLRenderSurface::resizeGL(const int width, const int height) {
    if (backendReady_) {
        backend_->resize(width, height);
    }
}

void OpenGLRenderSurface::paintGL() {
    if (!backendReady_) {
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());

    RenderFrame frame;
    if (scene_ != nullptr) {
        const RenderView view = editorCamera_.renderView(width(), height());
        frame = sceneRenderer_.buildFrame(*scene_, view);
    }
    backend_->render(frame);
}

void OpenGLRenderSurface::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton) {
        lastMousePosition_ = event->position().toPoint();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }

    QOpenGLWidget::mousePressEvent(event);
}

void OpenGLRenderSurface::mouseMoveEvent(QMouseEvent* event) {
    if (event->buttons().testFlag(Qt::MiddleButton)) {
        const QPoint currentPosition = event->position().toPoint();
        const QPoint delta = currentPosition - lastMousePosition_;
        lastMousePosition_ = currentPosition;

        if (event->modifiers().testFlag(Qt::ShiftModifier)) {
            editorCamera_.pan(static_cast<float>(delta.x()),
                              static_cast<float>(delta.y()),
                              height());
        } else {
            editorCamera_.orbit(static_cast<float>(delta.x()),
                                static_cast<float>(delta.y()));
        }

        update();
        event->accept();
        return;
    }

    QOpenGLWidget::mouseMoveEvent(event);
}

void OpenGLRenderSurface::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::MiddleButton) {
        unsetCursor();
        event->accept();
        return;
    }

    QOpenGLWidget::mouseReleaseEvent(event);
}

void OpenGLRenderSurface::wheelEvent(QWheelEvent* event) {
    const QPoint angleDelta = event->angleDelta();
    if (!angleDelta.isNull()) {
        editorCamera_.zoom(static_cast<float>(angleDelta.y()) / 120.0F);
        update();
        event->accept();
        return;
    }

    QOpenGLWidget::wheelEvent(event);
}

} // namespace renderlab
