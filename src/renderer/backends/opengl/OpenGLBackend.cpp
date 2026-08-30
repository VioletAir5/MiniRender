#include "renderer/backends/opengl/OpenGLBackend.h"

#include "assets/AssetRegistry.h"

#include <spdlog/spdlog.h>

namespace renderlab {
namespace {

// 优先使用实体材质，其次使用 primitive 默认材质，最终回退为白色。
glm::vec4 resolveBaseColor(const AssetRegistry& registry,
                           const MaterialHandle preferred,
                           const MaterialHandle fallback) {
    const MaterialAsset* material = registry.tryGetMaterial(preferred);
    if (material == nullptr) {
        material = registry.tryGetMaterial(fallback);
    }
    return material == nullptr ? glm::vec4{1.0F} : material->baseColorFactor;
}

} // namespace

OpenGLBackend::OpenGLBackend(const AssetRegistry& registry) noexcept
    : registry_(registry), meshCache_(registry) {}

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

    initialized_ = shader_.initialize();

    if (!initialized_) {
        spdlog::error("OpenGL backend initialization failed");
        shader_.shutdown();
    }
    return initialized_;
}

void OpenGLBackend::shutdown() {
    meshCache_.clear();
    shader_.shutdown();
    frameNumber_ = 0;
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

    // 即使当前没有相机也推进帧号，使长期未使用的 GPU 资源能够回收。
    ++frameNumber_;
    if (!frame.hasCamera) {
        meshCache_.collectGarbage(frameNumber_);
        return;
    }
    shader_.bind();
    shader_.setMatrix("uView", frame.view);
    shader_.setMatrix("uProjection", frame.projection);

    for (const RenderItem& item : frame.items) {
        // CPU 资产只在首次使用或 revision 变化时上传到 GPU。
        const CachedOpenGLMesh* cached =
            meshCache_.resolve(item.meshAsset, frameNumber_);
        if (cached == nullptr) {
            continue;
        }

        shader_.setMatrix("uModel", item.model);
        for (const CachedOpenGLPrimitive& primitive : cached->primitives) {
            const glm::vec4 baseColor = resolveBaseColor(
                registry_, item.materialAsset, primitive.defaultMaterial);
            shader_.setVector4("uBaseColor", baseColor);
            primitive.mesh.draw();
        }
    }

    shader_.release();
    meshCache_.collectGarbage(frameNumber_);
}

} // namespace renderlab
