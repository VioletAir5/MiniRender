#include "renderer/backends/opengl/passes/OpenGLDirectionalShadowPass.h"

#include "renderer/RenderFrame.h"
#include "renderer/backends/opengl/OpenGLMeshCache.h"
#include "renderer/backends/opengl/OpenGLRenderResources.h"
#include "renderer/backends/opengl/OpenGLShaderCache.h"
#include "renderer/backends/opengl/OpenGLShaderProgram.h"
#include "renderer/backends/opengl/passes/OpenGLPassContext.h"
#include "renderer/pipeline/BuiltInRenderPipeline.h"

#include <glad/glad.h>
#include <glm/common.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>

#include <cmath>
#include <string>

namespace renderlab {
namespace {

const RenderLightData* findShadowLight(const RenderFrame& frame, int& index) {
    for (std::size_t candidate = 0; candidate < frame.lights.size(); ++candidate) {
        const RenderLightData& light = frame.lights[candidate];
        if (light.type == LightType::Directional && light.castsShadow &&
            light.shadowTechnique != ShadowTechnique::None) {
            index = static_cast<int>(candidate);
            return &light;
        }
    }
    index = -1;
    return nullptr;
}

glm::mat4 shadowMatrix(const RenderFrame& frame, const RenderLightData& light) {
    const float distance = glm::max(light.shadowDistance, 1.0F);
    const float halfExtent = distance * 0.5F;
    const glm::vec3 direction = glm::normalize(light.direction);
    const glm::vec3 center = frame.cameraPosition;
    const glm::vec3 eye = center - direction * halfExtent;
    const glm::vec3 up = std::abs(glm::dot(direction, glm::vec3{0.0F, 1.0F, 0.0F})) > 0.95F
                             ? glm::vec3{1.0F, 0.0F, 0.0F}
                             : glm::vec3{0.0F, 1.0F, 0.0F};
    return glm::ortho(-halfExtent, halfExtent, -halfExtent, halfExtent, 0.1F, distance * 2.0F) *
           glm::lookAt(eye, center, up);
}

} // namespace

OpenGLDirectionalShadowPass::OpenGLDirectionalShadowPass(OpenGLPassContext& context) noexcept
    : context_(context) {}

bool OpenGLDirectionalShadowPass::initialize() {
    std::string error;
    return context_.shaderCache.resolve(context_.directionalShadowShader, error) != nullptr;
}

void OpenGLDirectionalShadowPass::execute(const RenderPassExecutionContext& execution) {
    const RenderFrame& frame = execution.frame;
    int lightIndex = -1;
    const RenderLightData* light = findShadowLight(frame, lightIndex);
    if (light == nullptr) {
        return;
    }

    std::string error;
    OpenGLShaderProgram* shader =
        context_.shaderCache.resolve(context_.directionalShadowShader, error);
    if (shader == nullptr || !context_.renderResources.bindRenderTargets(
                                 {}, render_resource_names::DirectionalShadowDepth, error)) {
        return;
    }

    context_.directionalShadowMatrix = shadowMatrix(frame, *light);
    context_.directionalShadowLightIndex = lightIndex;
    context_.directionalShadowTechnique = light->shadowTechnique;
    context_.directionalShadowBias = light->shadowBias;

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);
    glClear(GL_DEPTH_BUFFER_BIT);

    shader->bind();
    shader->setMatrix("uLightViewProjection", context_.directionalShadowMatrix);
    for (const RenderItem& item : frame.shadowCasters) {
        const CachedOpenGLMesh* mesh =
            context_.meshCache.resolve(item.meshAsset, context_.frameNumber);
        if (mesh == nullptr) {
            continue;
        }
        shader->setMatrix("uModel", item.model);
        for (const CachedOpenGLPrimitive& primitive : mesh->primitives) {
            primitive.mesh.draw();
        }
    }

    glCullFace(GL_BACK);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    context_.renderResources.bindExternalFramebuffer();
}

} // namespace renderlab
