#include "renderer/RenderViewport.h"

#include <QSurfaceFormat>

namespace renderlab {

RenderViewport::RenderViewport(QWidget* parent) : QOpenGLWidget(parent) {
    setMinimumSize(640, 360);
    setFocusPolicy(Qt::StrongFocus);
}

void RenderViewport::initializeGL() {
    initializeOpenGLFunctions();
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.055F, 0.065F, 0.085F, 1.0F);
}

void RenderViewport::resizeGL(const int width, const int height) {
    glViewport(0, 0, width, height);
}

void RenderViewport::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

} // namespace renderlab

