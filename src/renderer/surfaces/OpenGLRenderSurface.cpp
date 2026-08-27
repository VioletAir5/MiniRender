#include "renderer/surfaces/OpenGLRenderSurface.h"

#include "renderer/backends/opengl/OpenGLBackend.h"
#include "renderer/rhi/IRenderBackend.h"

#include <utility>

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

void OpenGLRenderSurface::setFrame(RenderFrame frame) {
    frame_ = std::move(frame);
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

    backend_->render(frame_);
}

} // namespace renderlab
