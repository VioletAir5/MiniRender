#include "renderer/backends/opengl/OpenGLCommandList.h"

#include "assets/AssetRegistry.h"
#include "renderer/RenderFrame.h"
#include "renderer/backends/opengl/OpenGLGridRenderer.h"
#include "renderer/backends/opengl/OpenGLMeshCache.h"
#include "renderer/backends/opengl/OpenGLShaderProgram.h"
#include "renderer/backends/opengl/OpenGLTexture.h"
#include "renderer/backends/opengl/OpenGLTextureCache.h"

#include <glad/glad.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include <optional>

namespace renderlab {
namespace {

const MaterialAsset* resolveMaterial(const AssetRegistry& registry,
                                     const MaterialHandle preferred,
                                     const MaterialHandle fallback) {
    const MaterialAsset* material = registry.tryGetMaterial(preferred);
    return material != nullptr ? material : registry.tryGetMaterial(fallback);
}

GLenum toOpenGL(const CompareOperation operation) {
    switch (operation) {
    case CompareOperation::Always:
        return GL_ALWAYS;
    case CompareOperation::NotEqual:
        return GL_NOTEQUAL;
    }
    return GL_ALWAYS;
}

GLenum toOpenGL(const StencilOperation operation) {
    switch (operation) {
    case StencilOperation::Keep:
        return GL_KEEP;
    case StencilOperation::Replace:
        return GL_REPLACE;
    }
    return GL_KEEP;
}

} // namespace

OpenGLCommandList::OpenGLCommandList(
    const AssetRegistry& registry,
    OpenGLShaderProgram& shader,
    OpenGLMeshCache& meshCache,
    OpenGLTextureCache& textureCache,
    OpenGLGridRenderer& gridRenderer)
    : registry_(registry),
      shader_(shader),
      meshCache_(meshCache),
      textureCache_(textureCache),
      gridRenderer_(gridRenderer) {}

void OpenGLCommandList::setFrameNumber(const std::uint64_t frameNumber) noexcept {
    frameNumber_ = frameNumber;
}

void OpenGLCommandList::setStencilState(const StencilState& state) {
    glStencilMask(state.writeMask);
    if (!state.enabled) {
        glDisable(GL_STENCIL_TEST);
        return;
    }

    glEnable(GL_STENCIL_TEST);
    glStencilFunc(toOpenGL(state.comparison),
                  static_cast<GLint>(state.reference), state.readMask);
    glStencilOp(toOpenGL(state.stencilFail),
                toOpenGL(state.depthFail),
                toOpenGL(state.pass));
}

void OpenGLCommandList::setDepthWriteEnabled(const bool enabled) {
    glDepthMask(enabled ? GL_TRUE : GL_FALSE);
}

void OpenGLCommandList::setBlendEnabled(const bool enabled) {
    enabled ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
}

void OpenGLCommandList::setCullEnabled(const bool enabled) {
    enabled ? glEnable(GL_CULL_FACE) : glDisable(GL_CULL_FACE);
}

void OpenGLCommandList::drawMesh(
    const RenderItem& item, const MeshDrawParameters& parameters) {
    const CachedOpenGLMesh* cached =
        meshCache_.resolve(item.meshAsset, frameNumber_);
    if (cached == nullptr) {
        return;
    }

    shader_.setMatrix("uModel", parameters.model);
    for (const CachedOpenGLPrimitive& primitive : cached->primitives) {
        const MaterialAsset* material = resolveMaterial(
            registry_, item.materialAsset, primitive.defaultMaterial);
        const bool useMaterialState =
            !parameters.overrideColor.has_value() && material != nullptr;
        const glm::vec4 baseColor = parameters.overrideColor.has_value()
                                        ? *parameters.overrideColor
                                        : useMaterialState
                                              ? material->baseColorFactor
                                              : glm::vec4{1.0F};

        const bool blend =
            useMaterialState && material->alphaMode == AlphaMode::Blend;
        const bool doubleSided = useMaterialState && material->doubleSided;
        setBlendEnabled(blend);
        setCullEnabled(!doubleSided);

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

        const auto resolveTextureBinding =
            [this, useMaterialState](
                const std::optional<TextureBinding>& candidate,
                const std::uint32_t unit) -> const TextureBinding* {
            if (!useMaterialState || !candidate.has_value() ||
                candidate->texCoordSet != 0U) {
                return nullptr;
            }
            const OpenGLTexture* texture =
                textureCache_.resolve(candidate->texture, frameNumber_);
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
        shader_.setVector2("uNormalUvOffset",
                           normalBinding != nullptr
                               ? normalBinding->offset : glm::vec2{0.0F});
        shader_.setVector2("uNormalUvScale",
                           normalBinding != nullptr
                               ? normalBinding->scale : glm::vec2{1.0F});
        shader_.setFloat("uNormalUvRotation",
                         normalBinding != nullptr
                             ? normalBinding->rotationRadians : 0.0F);

        const AlphaMode alphaMode = useMaterialState
                                        ? material->alphaMode
                                        : AlphaMode::Opaque;
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
        shader_.setInteger("uHasNormalTexture",
                           normalBinding != nullptr ? 1 : 0);
        primitive.mesh.draw();
    }
}

void OpenGLCommandList::drawEditorGrid(
    const glm::mat4& view, const glm::mat4& projection) {
    shader_.setInteger("uHasBaseColorTexture", 0);
    shader_.setInteger("uHasMetallicRoughnessTexture", 0);
    shader_.setInteger("uHasNormalTexture", 0);
    shader_.setInteger("uAlphaMode", 0);
    shader_.setInteger("uUnlit", 1);
    gridRenderer_.render(shader_, view, projection);
}

} // namespace renderlab
