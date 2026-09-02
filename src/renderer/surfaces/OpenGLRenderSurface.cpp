#include "renderer/surfaces/OpenGLRenderSurface.h"

#include "renderer/backends/opengl/OpenGLBackend.h"
#include "renderer/rhi/IRenderBackend.h"

#include <QCoreApplication>
#include <filesystem>
#include <utility>

namespace renderlab {

OpenGLRenderSurface::OpenGLRenderSurface(
    const AssetRegistry& registry,
    QWidget* parent)
    : QOpenGLWidget(parent),
      backend_(std::make_unique<OpenGLBackend>(
          registry,
          std::filesystem::path{
              QCoreApplication::applicationDirPath().toStdWString()} / "shaders")) {
    setFocusPolicy(Qt::StrongFocus);
}

OpenGLRenderSurface::~OpenGLRenderSurface() {
    // QOpenGLWidget 析构前主动激活上下文，保证后端能安全删除 GPU 资源。
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

    // QOpenGLWidget 绘制到 Qt 管理的 FBO，而不是传统编号为零的默认 FBO。
    glBindFramebuffer(GL_FRAMEBUFFER, defaultFramebufferObject());

    backend_->render(frame_);
}

} // namespace renderlab
