#include "renderer/backends/opengl/OpenGLBackend.h"

#include "assets/AssetRegistry.h"
#include "renderer/RenderFrame.h"

#include <spdlog/spdlog.h>

#include <string>
#include <utility>

namespace renderlab {

OpenGLBackend::OpenGLBackend(const AssetRegistry& registry,
                             std::filesystem::path shaderRoot)
    : registry_(registry),
      shaderLibrary_(std::move(shaderRoot)),
      meshCache_(registry),
      textureCache_(registry),
      commandList_(registry_, shader_, meshCache_, textureCache_, gridRenderer_) {
    pbrShader_ = shaderLibrary_.registerShader(
        "renderlab.shader.pbr_forward",
        ShaderAsset{.name = "PBR Forward",
                    .vertexSource = "pbr_forward.vert",
                    .fragmentSource = "pbr_forward.frag"});
}

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
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glClearStencil(0);
    glClearColor(0.055F, 0.065F, 0.085F, 1.0F);

    std::string shaderError;
    const auto pbrSource = shaderLibrary_.load(pbrShader_, shaderError);
    if (!pbrSource.has_value()) {
        spdlog::error("Unable to load PBR shader asset: {}", shaderError);
        initialized_ = false;
    } else {
        initialized_ = shader_.initialize(
            pbrSource->vertexSource, pbrSource->fragmentSource);
    }

    if (initialized_ && !gridRenderer_.initialize()) {
        // 编辑器网格属于可选资源，初始化失败不阻止场景表面继续绘制。
        spdlog::warn("OpenGL editor grid initialization failed");
    }
    if (!initialized_) {
        spdlog::error("OpenGL backend initialization failed");
        shader_.shutdown();
    }
    return initialized_;
}

void OpenGLBackend::shutdown() {
    gridRenderer_.shutdown();
    textureCache_.clear();
    meshCache_.clear();
    shader_.shutdown();
    frameNumber_ = 0;
    renderingScene_ = false;
    initialized_ = false;
}

void OpenGLBackend::resize(const int width, const int height) {
    if (initialized_) {
        glViewport(0, 0, width, height);
    }
}

void OpenGLBackend::beginFrame(const RenderFrame& frame) {
    if (!initialized_) {
        return;
    }

    // glClear 会遵守模板写掩码，因此每帧清理前恢复全部位可写。
    glStencilMask(0xFF);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    ++frameNumber_;
    commandList_.setFrameNumber(frameNumber_);
    renderingScene_ = false;

    if (!frame.hasCamera) {
        return;
    }

    shader_.bind();
    shader_.setVector3("uCameraPosition", frame.cameraPosition);
    shader_.setInteger("uHasDirectionalLight",
                       frame.directionalLight.valid ? 1 : 0);
    shader_.setVector3("uLightDirection", frame.directionalLight.direction);
    shader_.setVector3("uLightColor", frame.directionalLight.color);
    shader_.setFloat("uLightIntensity", frame.directionalLight.intensity);
    shader_.setMatrix("uView", frame.view);
    shader_.setMatrix("uProjection", frame.projection);
    shader_.setInteger("uBaseColorTexture", 0);
    shader_.setInteger("uMetallicRoughnessTexture", 1);
    shader_.setInteger("uNormalTexture", 2);
    renderingScene_ = true;
}

IRenderCommandList& OpenGLBackend::commandList() {
    return commandList_;
}

void OpenGLBackend::endFrame() {
    if (!initialized_) {
        return;
    }
    if (renderingScene_) {
        shader_.release();
        renderingScene_ = false;
    }
    meshCache_.collectGarbage(frameNumber_);
    textureCache_.collectGarbage(frameNumber_);
}

} // namespace renderlab
