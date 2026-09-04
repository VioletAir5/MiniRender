#include "renderer/SceneRenderer.h"

#include "scene/SceneDocument.h"
#include "scene/TransformUtils.h"

#include <glm/common.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include <algorithm>
#include <cmath>

namespace renderlab {
namespace {

// 优先返回 primary 相机；若没有标记，则使用第一个可用相机。
EntityId findPrimaryCamera(const SceneDocument& scene) {
    EntityId fallback = NullEntity;
    for (const auto& [entity, metadata] : scene.entities()) {
        (void)metadata;
        const CameraComponent* camera = scene.tryGetCamera(entity);
        if (camera == nullptr) {
            continue;
        }
        if (fallback == NullEntity) {
            fallback = entity;
        }
        if (camera->primary) {
            return entity;
        }
    }
    return fallback;
}

} // namespace

RenderFrame SceneRenderer::buildFrame(const SceneDocument& scene, const int viewportWidth,
                                      const int viewportHeight) const {
    RenderFrame frame;
    if (viewportWidth <= 0 || viewportHeight <= 0) {
        return frame;
    }

    const EntityId cameraEntity = findPrimaryCamera(scene);
    const CameraComponent* camera = scene.tryGetCamera(cameraEntity);
    if (camera == nullptr) {
        return frame;
    }

    const glm::mat4 cameraWorld = worldTransformMatrix(scene, cameraEntity);
    // 零缩放会使世界矩阵不可逆，此时不能计算观察矩阵。
    if (std::abs(glm::determinant(cameraWorld)) < 0.000001F) {
        return frame;
    }

    // 防御性修正非法裁剪面，避免生成未定义的透视矩阵。
    const float nearPlane = std::max(camera->nearPlane, 0.001F);
    const float farPlane = std::max(camera->farPlane, nearPlane + 0.001F);
    const float aspect = static_cast<float>(viewportWidth) / static_cast<float>(viewportHeight);

    RenderView view;
    view.view = glm::inverse(cameraWorld);
    view.projection =
        glm::perspective(glm::radians(camera->verticalFovDegrees), aspect, nearPlane, farPlane);
    view.cameraPosition = glm::vec3{cameraWorld[3]};
    view.valid = true;
    return buildFrame(scene, view);
}

RenderFrame SceneRenderer::buildFrame(const SceneDocument& scene, const RenderView& view) const {
    RenderFrame frame;
    if (!view.valid) {
        return frame;
    }
    frame.cameraPosition = view.cameraPosition;

    frame.lights.reserve(std::min(scene.entities().size(), MaxForwardLights));
    for (const auto& [entity, metadata] : scene.entities()) {
        (void)metadata;
        const LightComponent* light = scene.tryGetLight(entity);
        if (light == nullptr) {
            continue;
        }
        const glm::mat4 lightWorld = worldTransformMatrix(scene, entity);
        glm::vec3 direction = glm::vec3{lightWorld * glm::vec4{0.0F, 0.0F, -1.0F, 0.0F}};
        if (glm::length(direction) <= 0.000001F) {
            direction = {0.0F, -1.0F, 0.0F};
        } else {
            direction = glm::normalize(direction);
        }
        const float outerDegrees = std::clamp(light->outerConeDegrees, 0.1F, 89.9F);
        const float innerDegrees = std::clamp(light->innerConeDegrees, 0.0F, outerDegrees);
        frame.lights.push_back(RenderLightData{
            .entity = entity,
            .type = light->type,
            .position = glm::vec3{lightWorld[3]},
            .direction = direction,
            .color = glm::max(light->color, glm::vec3{0.0F}),
            .intensity = std::max(light->intensity, 0.0F),
            .range = std::max(light->range, 0.001F),
            .innerConeCosine = std::cos(glm::radians(innerDegrees)),
            .outerConeCosine = std::cos(glm::radians(outerDegrees)),
            .castsShadow = light->castShadow,
            .shadowTechnique = light->shadowTechnique,
            .shadowBias = std::max(light->shadowBias, 0.0F),
            .shadowDistance = std::max(light->shadowDistance, 1.0F),
        });
    }
    std::sort(frame.lights.begin(), frame.lights.end(),
              [](const RenderLightData& left, const RenderLightData& right) {
                  const bool leftPrimaryShadow = left.type == LightType::Directional &&
                                                 left.castsShadow &&
                                                 left.shadowTechnique != ShadowTechnique::None;
                  const bool rightPrimaryShadow = right.type == LightType::Directional &&
                                                  right.castsShadow &&
                                                  right.shadowTechnique != ShadowTechnique::None;
                  if (leftPrimaryShadow != rightPrimaryShadow) {
                      return leftPrimaryShadow;
                  }
                  return left.entity < right.entity;
              });
    if (frame.lights.size() > MaxForwardLights) {
        frame.lights.resize(MaxForwardLights);
    }

    frame.view = view.view;
    frame.projection = view.projection;
    frame.hasCamera = true;
    frame.items.reserve(scene.entities().size());

    for (const auto& [entity, metadata] : scene.entities()) {
        (void)metadata;
        const MeshRendererComponent* meshRenderer = scene.tryGetMeshRenderer(entity);
        if (meshRenderer == nullptr || !meshRenderer->visible || !meshRenderer->meshAsset.valid()) {
            continue;
        }

        RenderItem item{
            entity,
            meshRenderer->meshAsset,
            meshRenderer->materialAsset,
            worldTransformMatrix(scene, entity),
        };
        frame.items.push_back(item);
        if (meshRenderer->castShadow) {
            frame.shadowCasters.push_back(std::move(item));
        }
    }

    return frame;
}

} // namespace renderlab
