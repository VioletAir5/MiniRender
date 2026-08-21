#pragma once

#include <QOpenGLFunctions>
#include <QOpenGLWidget>

namespace renderlab {

class RenderViewport final : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    explicit RenderViewport(QWidget* parent = nullptr);

protected:
    void initializeGL() override;
    void resizeGL(int width, int height) override;
    void paintGL() override;
};

} // namespace renderlab

