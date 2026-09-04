#include "renderer/backends/opengl/passes/OpenGLSceneDraw.h"

#include "assets/AssetRegistry.h"
#include "renderer/RenderFrame.h"
#include "renderer/backends/opengl/OpenGLMeshCache.h"
#include "renderer/backends/opengl/OpenGLRenderResources.h"
#include "renderer/backends/opengl/OpenGLShaderCache.h"
#include "renderer/backends/opengl/OpenGLShaderProgram.h"
#include "renderer/backends/opengl/OpenGLTexture.h"
#include "renderer/backends/opengl/OpenGLTextureCache.h"
#include "renderer/backends/opengl/passes/OpenGLPassContext.h"
#include "renderer/pipeline/BuiltInRenderPipeline.h"

#include <glad/glad.h>

#include <algorithm>
#include <optional>
#include <string>

namespace renderlab {
namespace {

// 优先使用实体覆盖材质，其次使用 primitive 从模型导入的默认材质。
const MaterialAsset* resolveMaterial(const AssetRegistry& registry, const MaterialHandle preferred,
                                     const MaterialHandle fallback) {
    const MaterialAsset* material = registry.tryGetMaterial(preferred);
    return material != nullptr ? material : registry.tryGetMaterial(fallback);
}

int lightTypeIndex(const LightType type) {
    switch (type) {
    case LightType::Directional:
        return 0;
    case LightType::Point:
        return 1;
    case LightType::Spot:
        return 2;
    }
    return 0;
}

int shadowTechniqueIndex(const ShadowTechnique technique) {
    switch (technique) {
    case ShadowTechnique::None:
        return 0;
    case ShadowTechnique::Hard:
        return 1;
    case ShadowTechnique::Pcf:
        return 2;
    }
    return 0;
}

std::string indexedUniform(const char* name, const std::size_t index) {
    return std::string{name} + "[" + std::to_string(index) + "]";
}

void configureLighting(OpenGLPassContext& context, OpenGLShaderProgram& shader,
                       const RenderFrame& frame) {
    const std::size_t lightCount = std::min(frame.lights.size(), MaxForwardLights);
    shader.setInteger("uLightCount", static_cast<int>(lightCount));
    for (std::size_t index = 0; index < lightCount; ++index) {
        const RenderLightData& light = frame.lights[index];
        shader.setInteger(indexedUniform("uLightTypes", index).c_str(), lightTypeIndex(light.type));
        shader.setVector3(indexedUniform("uLightPositions", index).c_str(), light.position);
        shader.setVector3(indexedUniform("uLightDirections", index).c_str(), light.direction);
        shader.setVector3(indexedUniform("uLightColors", index).c_str(), light.color);
        shader.setFloat(indexedUniform("uLightIntensities", index).c_str(), light.intensity);
        shader.setFloat(indexedUniform("uLightRanges", index).c_str(), light.range);
        shader.setFloat(indexedUniform("uLightInnerConeCosines", index).c_str(),
                        light.innerConeCosine);
        shader.setFloat(indexedUniform("uLightOuterConeCosines", index).c_str(),
                        light.outerConeCosine);
    }

    shader.setMatrix("uLightViewProjection", context.directionalShadowMatrix);
    shader.setInteger("uShadowLightIndex", context.directionalShadowLightIndex);
    shader.setInteger("uShadowTechnique", shadowTechniqueIndex(context.directionalShadowTechnique));
    shader.setFloat("uShadowBias", context.directionalShadowBias);
    shader.setInteger("uDirectionalShadowMap", 5);
    glActiveTexture(GL_TEXTURE5);
    glBindTexture(GL_TEXTURE_2D,
                  context.renderResources.texture(render_resource_names::DirectionalShadowDepth));
    glActiveTexture(GL_TEXTURE0);
}

} // namespace

void drawOpenGLSceneItem(OpenGLPassContext& context, const RenderFrame& frame,
                         const RenderItem& item, const glm::mat4& model,
                         const glm::vec4* overrideColor) {
    const CachedOpenGLMesh* cached = context.meshCache.resolve(item.meshAsset, context.frameNumber);
    if (cached == nullptr) {
        return;
    }

    for (const CachedOpenGLPrimitive& primitive : cached->primitives) {
        const MaterialAsset* material =
            resolveMaterial(context.registry, item.materialAsset, primitive.defaultMaterial);
        const bool useMaterialState = overrideColor == nullptr && material != nullptr;
        const ShaderHandle requestedShader = useMaterialState && material->shader.valid()
                                                 ? material->shader
                                                 : context.fallbackSurfaceShader;
        std::string shaderError;
        OpenGLShaderProgram* shader = context.shaderCache.resolve(requestedShader, shaderError);
        if (shader == nullptr && requestedShader != context.fallbackSurfaceShader) {
            shader = context.shaderCache.resolve(context.fallbackSurfaceShader, shaderError);
        }
        if (shader == nullptr) {
            continue;
        }
        shader->bind();
        shader->setVector3("uCameraPosition", frame.cameraPosition);
        shader->setMatrix("uView", frame.view);
        shader->setMatrix("uProjection", frame.projection);
        shader->setMatrix("uModel", model);
        configureLighting(context, *shader, frame);
        shader->setInteger("uBaseColorTexture", 0);
        shader->setInteger("uMetallicRoughnessTexture", 1);
        shader->setInteger("uNormalTexture", 2);
        const glm::vec4 baseColor = overrideColor != nullptr ? *overrideColor
                                    : useMaterialState       ? material->baseColorFactor
                                                             : glm::vec4{1.0F};

        const bool blend = useMaterialState && material->alphaMode == AlphaMode::Blend;
        const bool doubleSided = useMaterialState && material->doubleSided;
        blend ? glEnable(GL_BLEND) : glDisable(GL_BLEND);
        doubleSided ? glDisable(GL_CULL_FACE) : glEnable(GL_CULL_FACE);

        shader->setFloat("uMetallic", useMaterialState ? material->metallicFactor : 0.0F);
        shader->setFloat("uRoughness", useMaterialState ? material->roughnessFactor : 1.0F);
        shader->setFloat("uNormalScale", useMaterialState ? material->normalScale : 1.0F);
        shader->setVector3("uEmissive",
                           useMaterialState ? material->emissiveFactor : glm::vec3{0.0F});
        shader->setInteger("uUnlit", !useMaterialState || material->unlit ? 1 : 0);

        // 缓存扩容可能移动 GPU 包装对象，因此 resolve 成功后立即绑定。
        const auto resolveTextureBinding =
            [&context, useMaterialState](const std::optional<TextureBinding>& candidate,
                                         const std::uint32_t unit) -> const TextureBinding* {
            if (!useMaterialState || !candidate.has_value() || candidate->texCoordSet != 0U) {
                return nullptr;
            }
            const OpenGLTexture* texture =
                context.textureCache.resolve(candidate->texture, context.frameNumber);
            if (texture == nullptr) {
                return nullptr;
            }
            texture->bind(unit);
            return &*candidate;
        };

        const TextureBinding* baseColorBinding =
            material != nullptr ? resolveTextureBinding(material->baseColorTexture, 0) : nullptr;
        shader->setVector2("uUvOffset", baseColorBinding != nullptr ? baseColorBinding->offset
                                                                    : glm::vec2{0.0F});
        shader->setVector2("uUvScale",
                           baseColorBinding != nullptr ? baseColorBinding->scale : glm::vec2{1.0F});
        shader->setFloat("uUvRotation",
                         baseColorBinding != nullptr ? baseColorBinding->rotationRadians : 0.0F);

        const TextureBinding* metallicRoughnessBinding =
            material != nullptr ? resolveTextureBinding(material->metallicRoughnessTexture, 1)
                                : nullptr;
        shader->setVector2("uMetallicRoughnessUvOffset", metallicRoughnessBinding != nullptr
                                                             ? metallicRoughnessBinding->offset
                                                             : glm::vec2{0.0F});
        shader->setVector2("uMetallicRoughnessUvScale", metallicRoughnessBinding != nullptr
                                                            ? metallicRoughnessBinding->scale
                                                            : glm::vec2{1.0F});
        shader->setFloat(
            "uMetallicRoughnessUvRotation",
            metallicRoughnessBinding != nullptr ? metallicRoughnessBinding->rotationRadians : 0.0F);

        const TextureBinding* normalBinding =
            material != nullptr ? resolveTextureBinding(material->normalTexture, 2) : nullptr;
        shader->setVector2("uNormalUvOffset",
                           normalBinding != nullptr ? normalBinding->offset : glm::vec2{0.0F});
        shader->setVector2("uNormalUvScale",
                           normalBinding != nullptr ? normalBinding->scale : glm::vec2{1.0F});
        shader->setFloat("uNormalUvRotation",
                         normalBinding != nullptr ? normalBinding->rotationRadians : 0.0F);

        const AlphaMode alphaMode = useMaterialState ? material->alphaMode : AlphaMode::Opaque;
        shader->setInteger("uAlphaMode", alphaMode == AlphaMode::Mask    ? 1
                                         : alphaMode == AlphaMode::Blend ? 2
                                                                         : 0);
        shader->setFloat("uAlphaCutoff", useMaterialState ? material->alphaCutoff : 0.5F);
        shader->setVector4("uBaseColor", baseColor);
        shader->setInteger("uHasBaseColorTexture", baseColorBinding != nullptr ? 1 : 0);
        shader->setInteger("uHasMetallicRoughnessTexture",
                           metallicRoughnessBinding != nullptr ? 1 : 0);
        shader->setInteger("uHasNormalTexture", normalBinding != nullptr ? 1 : 0);
        primitive.mesh.draw();
    }
}

} // namespace renderlab
