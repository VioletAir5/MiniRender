#include "renderer/surfaces/OpenGLRenderSurface.h"

#include "renderer/backends/opengl/OpenGLBackend.h"
#include "renderer/rhi/IRenderBackend.h"
#include "scene/SceneDocument.h"

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
        frame = sceneRenderer_.buildFrame(*scene_, width(), height());
    }
    backend_->render(frame);
}

} // namespace renderlab
