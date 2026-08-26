#include "renderer/backends/opengl/OpenGLBackend.h"

#include "scene/Components.h"

#include <QDebug>

namespace renderlab {

bool OpenGLBackend::initialize() {
    initializeOpenGLFunctions();
    glEnable(GL_DEPTH_TEST);
    glClearColor(0.055F, 0.065F, 0.085F, 1.0F);

    const bool shaderReady = shader_.initialize();
    const bool meshReady = cubeMesh_.createCube(*this);
    initialized_ = shaderReady && meshReady;

    if (!initialized_) {
        qWarning() << "OpenGL backend initialization failed";
    }
    return initialized_;
}

void OpenGLBackend::shutdown() {
    cubeMesh_.destroy();
    shader_.shutdown();
    initialized_ = false;
}

void OpenGLBackend::resize(const int width, const int height) {
    glViewport(0, 0, width, height);
}

void OpenGLBackend::render(const RenderFrame& frame) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!initialized_ || !frame.hasCamera) {
        return;
    }

    shader_.bind();
    shader_.setMatrix(*this, "uView", frame.view);
    shader_.setMatrix(*this, "uProjection", frame.projection);

    for (const RenderItem& item : frame.items) {
        if (item.meshAsset != BuiltinCubeMeshAsset) {
            continue;
        }

        shader_.setMatrix(*this, "uModel", item.model);
        cubeMesh_.draw(*this);
    }

    shader_.release();
}

} // namespace renderlab
