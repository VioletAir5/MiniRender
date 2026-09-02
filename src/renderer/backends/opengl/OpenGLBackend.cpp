#include "renderer/backends/opengl/OpenGLBackend.h"

#include "assets/AssetRegistry.h"
#include "renderer/RenderFrame.h"
#include "renderer/backends/opengl/passes/OpenGLPassContext.h"

#include <spdlog/spdlog.h>

#include <string>
#include <utility>

namespace renderlab {

OpenGLBackend::OpenGLBackend(const AssetRegistry& registry,
                             std::filesystem::path shaderRoot)
    : registry_(registry),
      shaderLibrary_(std::move(shaderRoot)),
      meshCache_(registry),
      textureCache_(registry) {
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

    if (initialized_ && !gridPass_.initialize()) {
        // 编辑器网格属于可选 Pass，初始化失败不阻止场景表面继续绘制。
        spdlog::warn("OpenGL editor grid pass initialization failed");
    }
    if (!initialized_) {
        spdlog::error("OpenGL backend initialization failed");
        shader_.shutdown();
    }
    return initialized_;
}

void OpenGLBackend::shutdown() {
    gridPass_.shutdown();
    textureCache_.clear();
    meshCache_.clear();
    shader_.shutdown();
    frameNumber_ = 0;
    initialized_ = false;
}

void OpenGLBackend::resize(const int width, const int height) {
    if (initialized_) {
        glViewport(0, 0, width, height);
    }
}

void OpenGLBackend::render(const RenderFrame& frame) {
    if (!initialized_) {
        return;
    }

    // glClear 会遵守模板写掩码，因此每帧清理前恢复全部位可写。
    glStencilMask(0xFF);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    ++frameNumber_;

    if (!frame.hasCamera) {
        meshCache_.collectGarbage(frameNumber_);
        textureCache_.collectGarbage(frameNumber_);
        return;
    }

    shader_.bind();
    OpenGLPassContext context{
        .registry = registry_,
        .shader = shader_,
        .meshCache = meshCache_,
        .textureCache = textureCache_,
        .frameNumber = frameNumber_,
    };

    const RenderItem* outlinedItem = forwardPass_.render(frame, context);
    outlinePass_.render(frame, outlinedItem, context);
    gridPass_.render(frame, context);

    shader_.release();
    meshCache_.collectGarbage(frameNumber_);
    textureCache_.collectGarbage(frameNumber_);
}

} // namespace renderlab
