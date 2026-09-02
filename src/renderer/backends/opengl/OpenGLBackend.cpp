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

    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glClearStencil(0);
    glClearColor(0.055F, 0.065F, 0.085F, 1.0F);

    initialized_ = shader_.initialize();
    if (initialized_ && !gridRenderer_.initialize()) {
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
    shader_.setVector3("uCameraPosition", frame.cameraPosition);
    shader_.setInteger("uHasDirectionalLight", frame.directionalLight.valid ? 1 : 0);
    shader_.setVector3("uLightDirection", frame.directionalLight.direction);
    shader_.setVector3("uLightColor", frame.directionalLight.color);
    shader_.setFloat("uLightIntensity", frame.directionalLight.intensity);

    shader_.setMatrix("uView", frame.view);
    shader_.setMatrix("uProjection", frame.projection);
    // baseColor 纹理统一占用单元 0；每个 primitive 只需切换实际纹理对象。
    shader_.setInteger("uBaseColorTexture", 0);
    // glTF Metallic-Roughness 纹理使用单元 1，其中 G/B 分别存储粗糙度和金属度。
    shader_.setInteger("uMetallicRoughnessTexture", 1);
    // 法线纹理使用单元 2，并在 Shader 中通过 TBN 从切线空间转换到世界空间。
    shader_.setInteger("uNormalTexture", 2);

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
    shader_.setInteger("uHasMetallicRoughnessTexture", 0);
    shader_.setInteger("uHasNormalTexture", 0);
    shader_.setInteger("uAlphaMode", 0);

    shader_.setInteger("uUnlit", 1);
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
        shader_.setFloat("uMetallic",
                         useMaterialState ? material->metallicFactor : 0.0F);
        shader_.setFloat("uRoughness",
                         useMaterialState ? material->roughnessFactor : 1.0F);
        shader_.setFloat("uNormalScale",
                         useMaterialState ? material->normalScale : 1.0F);
        shader_.setVector3("uEmissive",
                           useMaterialState ? material->emissiveFactor
                                            : glm::vec3{0.0F});
        shader_.setInteger("uUnlit",
                           !useMaterialState || material->unlit ? 1 : 0);
        doubleSided ? glDisable(GL_CULL_FACE) : glEnable(GL_CULL_FACE);

        // resolve 可能扩容缓存并移动对象，所以纹理必须在下一次 resolve 前完成绑定。
        const auto resolveTextureBinding =
            [this, useMaterialState](
                const std::optional<TextureBinding>& candidate,
                const std::uint32_t unit) -> const TextureBinding* {
            if (!useMaterialState || !candidate.has_value() ||
                candidate->texCoordSet != 0U) {
                return nullptr;
            }
            const OpenGLTexture* texture = textureCache_.resolve(
                candidate->texture, frameNumber_);
            if (texture == nullptr) {
                return nullptr;
            }
            texture->bind(unit);
            return &*candidate;
        };

        const TextureBinding* baseColorBinding = material != nullptr
            ? resolveTextureBinding(material->baseColorTexture, 0)
            : nullptr;
        shader_.setVector2("uUvOffset",
                           baseColorBinding != nullptr
                               ? baseColorBinding->offset : glm::vec2{0.0F});
        shader_.setVector2("uUvScale",
                           baseColorBinding != nullptr
                               ? baseColorBinding->scale : glm::vec2{1.0F});
        shader_.setFloat("uUvRotation",
                         baseColorBinding != nullptr
                             ? baseColorBinding->rotationRadians : 0.0F);

        const TextureBinding* metallicRoughnessBinding = material != nullptr
            ? resolveTextureBinding(material->metallicRoughnessTexture, 1)
            : nullptr;
        shader_.setVector2(
            "uMetallicRoughnessUvOffset",
            metallicRoughnessBinding != nullptr
                ? metallicRoughnessBinding->offset : glm::vec2{0.0F});
        shader_.setVector2(
            "uMetallicRoughnessUvScale",
            metallicRoughnessBinding != nullptr
                ? metallicRoughnessBinding->scale : glm::vec2{1.0F});
        shader_.setFloat(
            "uMetallicRoughnessUvRotation",
            metallicRoughnessBinding != nullptr
                ? metallicRoughnessBinding->rotationRadians : 0.0F);

        const TextureBinding* normalBinding = material != nullptr
            ? resolveTextureBinding(material->normalTexture, 2)
            : nullptr;
        shader_.setVector2(
            "uNormalUvOffset",
            normalBinding != nullptr
                ? normalBinding->offset : glm::vec2{0.0F});
        shader_.setVector2(
            "uNormalUvScale",
            normalBinding != nullptr
                ? normalBinding->scale : glm::vec2{1.0F});
        shader_.setFloat(
            "uNormalUvRotation",
            normalBinding != nullptr
                ? normalBinding->rotationRadians : 0.0F);
        const AlphaMode alphaMode = useMaterialState
                                        ? material->alphaMode : AlphaMode::Opaque;
        shader_.setInteger("uAlphaMode",
                           alphaMode == AlphaMode::Mask ? 1
                           : alphaMode == AlphaMode::Blend ? 2 : 0);
        shader_.setFloat("uAlphaCutoff",
                         useMaterialState ? material->alphaCutoff : 0.5F);

        shader_.setVector4("uBaseColor", baseColor);
        shader_.setInteger("uHasBaseColorTexture",
                           baseColorBinding != nullptr ? 1 : 0);
        shader_.setInteger("uHasMetallicRoughnessTexture",
                           metallicRoughnessBinding != nullptr ? 1 : 0);
        shader_.setInteger("uHasNormalTexture", normalBinding != nullptr ? 1 : 0);
        primitive.mesh.draw();
    }
}

} // namespace renderlab
