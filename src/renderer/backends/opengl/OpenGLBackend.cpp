#include "renderer/backends/opengl/OpenGLBackend.h"

#include "assets/AssetRegistry.h"
#include "renderer/RenderFrame.h"
#include "renderer/backends/opengl/passes/OpenGLPassRegistry.h"

#include <spdlog/spdlog.h>

#include <string>
#include <utility>

namespace renderlab {

OpenGLBackend::OpenGLBackend(const AssetRegistry& registry, std::filesystem::path shaderRoot,
                             RenderPipelineDescriptor pipelineDescriptor)
    : registry_(registry), shaderLibrary_(std::move(shaderRoot)), meshCache_(registry),
      textureCache_(registry), passContext_{registry_, shader_, meshCache_, textureCache_},
      pipelineDescriptor_(std::move(pipelineDescriptor)) {
    pbrShader_ = shaderLibrary_.registerShader("renderlab.shader.pbr_forward",
                                               ShaderAsset{.name = "PBR Forward",
                                                           .vertexSource = "pbr_forward.vert",
                                                           .fragmentSource = "pbr_forward.frag"});

    passFactoryReady_ = registerBuiltInOpenGLPasses(passFactory_, passContext_);
}

bool OpenGLBackend::initialize() {
    if (!passFactoryReady_) {
        spdlog::error("OpenGL pass registration failed");
        return false;
    }
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
        initialized_ = shader_.initialize(pbrSource->vertexSource, pbrSource->fragmentSource);
    }

    if (initialized_) {
        std::string pipelineError;
        if (!pipeline_.build(pipelineDescriptor_, passFactory_, pipelineError) ||
            !pipeline_.initialize(pipelineError)) {
            spdlog::error("OpenGL render pipeline initialization failed: {}", pipelineError);
            initialized_ = false;
        }
    }
    if (!initialized_) {
        spdlog::error("OpenGL backend initialization failed");
        pipeline_.shutdown();
        shader_.shutdown();
    } else if (viewportWidth_ > 0 && viewportHeight_ > 0) {
        pipeline_.resize(viewportWidth_, viewportHeight_);
    }
    return initialized_;
}

void OpenGLBackend::shutdown() {
    pipeline_.shutdown();
    textureCache_.clear();
    meshCache_.clear();
    shader_.shutdown();
    frameNumber_ = 0;
    initialized_ = false;
}

void OpenGLBackend::resize(const int width, const int height) {
    viewportWidth_ = width;
    viewportHeight_ = height;
    if (initialized_) {
        glViewport(0, 0, width, height);
        pipeline_.resize(width, height);
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

    passContext_.frameNumber = frameNumber_;
    passContext_.outlinedItem = nullptr;
    pipeline_.execute(RenderPassExecutionContext{
        .frame = frame,
        .frameNumber = frameNumber_,
        .viewportWidth = viewportWidth_,
        .viewportHeight = viewportHeight_,
    });

    shader_.release();
    meshCache_.collectGarbage(frameNumber_);
    textureCache_.collectGarbage(frameNumber_);
}

} // namespace renderlab
