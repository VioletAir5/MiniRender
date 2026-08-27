#pragma once

#include "renderer/surfaces/IRenderSurface.h"

#include <glad/glad.h>
#include <QOpenGLWidget>

#include <memory>

namespace renderlab {

class IRenderBackend;

class OpenGLRenderSurface final : public QOpenGLWidget, public IRenderSurface {
public:
    explicit OpenGLRenderSurface(QWidget* parent = nullptr);
    ~OpenGLRenderSurface() override;

    [[nodiscard]] QWidget& widget() noexcept override;
    void setFrame(RenderFrame frame) override;
    void requestRender() override;

protected:
    void initializeGL() override;
    void resizeGL(int width, int height) override;
    void paintGL() override;

private:
    std::unique_ptr<IRenderBackend> backend_;
    RenderFrame frame_;
    bool backendReady_{false};
};

} // namespace renderlab
