#include "renderer/backends/opengl/OpenGLBackend.h"

#include "scene/Components.h"

#include <spdlog/spdlog.h>

namespace renderlab {

bool OpenGLBackend::initialize() {
    if (gladLoadGL() == 0) {
        spdlog::error("GLAD failed to load OpenGL functions");
        return false;
    }
    if (GLAD_GL_VERSION_3_3 == 0) {
        spdlog::error("RenderLab requires an OpenGL 3.3 context");
        return false;
    }

    spdlog::info("Loaded OpenGL {}.{}", GLVersion.major, GLVersion.minor);

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.055F, 0.065F, 0.085F, 1.0F);

    const bool shaderReady = shader_.initialize();
    const bool meshReady = cubeMesh_.createCube();
    initialized_ = shaderReady && meshReady;

    if (!initialized_) {
        spdlog::error("OpenGL backend initialization failed");
    }
    return initialized_;
}

void OpenGLBackend::shutdown() {
    cubeMesh_.destroy();
    shader_.shutdown();
    initialized_ = false;
}

void OpenGLBackend::resize(const int width, const int height) {
    if (!initialized_) {
        return;
    }
    glViewport(0, 0, width, height);
}

void OpenGLBackend::render(const RenderFrame& frame) {
    if (!initialized_) {
        return;
    }
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!frame.hasCamera) {
        return;
    }

    shader_.bind();
    shader_.setMatrix("uView", frame.view);
    shader_.setMatrix("uProjection", frame.projection);

    for (const RenderItem& item : frame.items) {
        if (item.meshAsset != BuiltinCubeMeshAsset) {
            continue;
        }

        shader_.setMatrix("uModel", item.model);
        cubeMesh_.draw();
    }

    shader_.release();
}

} // namespace renderlab
