#pragma once

#include "renderer/SceneRenderer.h"
#include "renderer/surfaces/IRenderSurface.h"

#include <glad/glad.h>
#include <QOpenGLWidget>

#include <memory>

namespace renderlab {

class IRenderBackend;
class SceneDocument;

class OpenGLRenderSurface final : public QOpenGLWidget, public IRenderSurface {
public:
    explicit OpenGLRenderSurface(QWidget* parent = nullptr);
    ~OpenGLRenderSurface() override;

    [[nodiscard]] QWidget& widget() noexcept override;
    void setScene(const SceneDocument* scene) noexcept override;
    void requestRender() override;

protected:
    void initializeGL() override;
    void resizeGL(int width, int height) override;
    void paintGL() override;

private:
    const SceneDocument* scene_{nullptr};
    SceneRenderer sceneRenderer_;
    std::unique_ptr<IRenderBackend> backend_;
    bool backendReady_{false};
};

} // namespace renderlab
