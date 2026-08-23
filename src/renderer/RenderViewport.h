#pragma once

#include <QOpenGLFunctions>
#include <QOpenGLWidget>

namespace renderlab {

class SceneDocument;

class RenderViewport final : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    explicit RenderViewport(QWidget* parent = nullptr);

    void setScene(const SceneDocument* scene) noexcept;

protected:
    void initializeGL() override;
    void resizeGL(int width, int height) override;
    void paintGL() override;

private:
    const SceneDocument* scene_{nullptr};
};

} // namespace renderlab

