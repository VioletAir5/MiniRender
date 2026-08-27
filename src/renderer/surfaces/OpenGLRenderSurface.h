#pragma once

#include "editor/EditorCamera.h"
#include "renderer/SceneRenderer.h"
#include "renderer/surfaces/IRenderSurface.h"

#include <glad/glad.h>
#include <QOpenGLWidget>
#include <QPoint>

#include <memory>

class QMouseEvent;
class QWheelEvent;

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
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    const SceneDocument* scene_{nullptr};
    SceneRenderer sceneRenderer_;
    std::unique_ptr<IRenderBackend> backend_;
    EditorCamera editorCamera_;
    QPoint lastMousePosition_;
    bool backendReady_{false};
};

} // namespace renderlab
