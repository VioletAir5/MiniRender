#include "renderer/backends/opengl/OpenGLBackend.h"

#include "assets/AssetRegistry.h"
#include "../../RenderFrame.h"

#include <glm/ext/matrix_transform.hpp>
#include <spdlog/spdlog.h>

#include <algorithm>

namespace renderlab {
namespace {

// 优先使用实体材质，其次使用 primitive 默认材质。
const MaterialAsset* resolveMaterial(const AssetRegistry& registry,
                                     const MaterialHandle preferred,
                                     const MaterialHandle fallback) {
    const MaterialAsset* material = registry.tryGetMaterial(preferred);
    if (material == nullptr) {
        material = registry.tryGetMaterial(fallback);
    }
    return material;
}

} // namespace

OpenGLBackend::OpenGLBackend(const AssetRegistry& registry) noexcept
    : registry_(registry), meshCache_(registry), textureCache_(registry) {}

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
    glClearStencil(0);
    glClearColor(0.055F, 0.065F, 0.085F, 1.0F);

    initialized_ = shader_.initialize();
    if (initialized_ && !gridRenderer_.initialize()) {
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        // 网格是可选编辑器覆盖层，失败时不能阻止场景本身继续渲染。
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
    // glClear 会遵守模板写掩码，因此每帧清理前必须恢复全部位可写。
    glStencilMask(0xFF);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    // 即使当前没有相机也推进帧号，使长期未使用的 GPU 资源能够回收。
    ++frameNumber_;
    if (!frame.hasCamera) {
        meshCache_.collectGarbage(frameNumber_);
        textureCache_.collectGarbage(frameNumber_);
        return;
    }
    shader_.bind();
    shader_.setMatrix("uView", frame.view);
    shader_.setMatrix("uProjection", frame.projection);
    // baseColor 纹理统一占用单元 0；每个 primitive 只需切换实际纹理对象。
    shader_.setInteger("uBaseColorTexture", 0);

    const SelectionOutline* outline = frame.selectionOutline.has_value()
                                          ? &*frame.selectionOutline
                                          : nullptr;
    const RenderItem* outlinedItem = nullptr;

    if (outline != nullptr) {
        // 正常场景阶段只让选中实体把值 1 写入模板缓冲。
        glEnable(GL_STENCIL_TEST);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
    } else {
        glDisable(GL_STENCIL_TEST);
    }

    for (const RenderItem& item : frame.items) {
        const bool isOutlined =
            outline != nullptr && item.entity == outline->entity;
        if (outline != nullptr) {
            glStencilFunc(GL_ALWAYS, 1, 0xFF);
            glStencilMask(isOutlined ? 0xFF : 0x00);
        }

        drawItem(item, item.model);
        if (isOutlined) {
            outlinedItem = &item;
        }
    }

    if (outline != nullptr && outlinedItem != nullptr) {
        // 第二遍只绘制模板值不为 1 的放大模型，留下环绕原模型的窄边。
        glStencilFunc(GL_NOTEQUAL, 1, 0xFF);
        glStencilMask(0x00);
        glDepthMask(GL_FALSE);

        const float outlineScale = std::max(outline->scale, 1.0F);
        const glm::mat4 outlineModel = glm::scale(
            outlinedItem->model, glm::vec3{outlineScale});
        drawItem(*outlinedItem, outlineModel, &outline->color);

        glDepthMask(GL_TRUE);
    }

    // 编辑器网格不参与实体模板标记，绘制前恢复通用状态。
    glStencilMask(0xFF);
    glDisable(GL_STENCIL_TEST);
    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    // 网格复用同一 Shader，必须清除最后一个场景材质留下的纹理开关。
    shader_.setInteger("uHasBaseColorTexture", 0);
    shader_.setInteger("uAlphaMode", 0);

    // 编辑器网格不进入场景资产链路，作为独立覆盖层在场景之后绘制。
    gridRenderer_.render(shader_, frame.view, frame.projection);

    shader_.release();
    meshCache_.collectGarbage(frameNumber_);
    textureCache_.collectGarbage(frameNumber_);
}

void OpenGLBackend::drawItem(const RenderItem& item, const glm::mat4& model,
                             const glm::vec4* overrideColor) {
    // CPU 资产只在首次使用或 revision 变化时上传到 GPU。
    const CachedOpenGLMesh* cached =
        meshCache_.resolve(item.meshAsset, frameNumber_);
    if (cached == nullptr) {
        return;
    }

    shader_.setMatrix("uModel", model);
    for (const CachedOpenGLPrimitive& primitive : cached->primitives) {
        const MaterialAsset* material = resolveMaterial(
            registry_, item.materialAsset, primitive.defaultMaterial);
        const glm::vec4 baseColor = overrideColor != nullptr
                                        ? *overrideColor
                                        : material != nullptr
                                              ? material->baseColorFactor
                                              : glm::vec4{1.0F};
        const bool useMaterialState =
            overrideColor == nullptr && material != nullptr;
        const bool blend = useMaterialState && material->alphaMode == AlphaMode::Blend;
        const bool doubleSided = useMaterialState && material->doubleSided;
        blend ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
        doubleSided ? glDisable(GL_CULL_FACE) : glEnable(GL_CULL_FACE);

        const OpenGLTexture* texture = nullptr;
        if (overrideColor == nullptr && material != nullptr &&
            material->baseColorTexture.has_value() &&
            material->baseColorTexture->texCoordSet == 0U) {
            // 当前 Vertex 只有 UV0；无效纹理或上传失败会自然回退为纯色。
            texture = textureCache_.resolve(
                material->baseColorTexture->texture, frameNumber_);
        }
        const TextureBinding* binding =
            texture != nullptr ? &*material->baseColorTexture : nullptr;
        shader_.setVector2("uUvOffset",
                           binding != nullptr ? binding->offset : glm::vec2{0.0F});
        shader_.setVector2("uUvScale",
                           binding != nullptr ? binding->scale : glm::vec2{1.0F});
        shader_.setFloat("uUvRotation",
                         binding != nullptr ? binding->rotationRadians : 0.0F);
        const AlphaMode alphaMode = useMaterialState
                                        ? material->alphaMode : AlphaMode::Opaque;
        shader_.setInteger("uAlphaMode",
                           alphaMode == AlphaMode::Mask ? 1
                           : alphaMode == AlphaMode::Blend ? 2 : 0);
        shader_.setFloat("uAlphaCutoff",
                         useMaterialState ? material->alphaCutoff : 0.5F);

        shader_.setVector4("uBaseColor", baseColor);
        shader_.setInteger("uHasBaseColorTexture", texture != nullptr ? 1 : 0);
        if (texture != nullptr) {
            texture->bind(0);
        }
        primitive.mesh.draw();
    }
}

} // namespace renderlab
