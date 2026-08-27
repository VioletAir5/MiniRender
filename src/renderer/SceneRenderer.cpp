#include "renderer/SceneRenderer.h"

#include "scene/SceneDocument.h"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include <algorithm>
#include <cmath>

namespace renderlab {
namespace {

glm::mat4 localMatrix(const TransformComponent& transform) {
    glm::mat4 matrix{1.0F};
    matrix = glm::translate(matrix, transform.position);
    matrix = glm::rotate(matrix, glm::radians(transform.rotationDegrees.z),
                         glm::vec3{0.0F, 0.0F, 1.0F});
    matrix = glm::rotate(matrix, glm::radians(transform.rotationDegrees.y),
                         glm::vec3{0.0F, 1.0F, 0.0F});
    matrix = glm::rotate(matrix, glm::radians(transform.rotationDegrees.x),
                         glm::vec3{1.0F, 0.0F, 0.0F});
    return glm::scale(matrix, transform.scale);
}

glm::mat4 worldMatrix(const SceneDocument& scene, const EntityId entity) {
    glm::mat4 world{1.0F};
    const EntityMetadata* metadata = scene.tryGetEntity(entity);
    if (metadata != nullptr && metadata->parent != NullEntity) {
        world = worldMatrix(scene, metadata->parent);
    }

    const TransformComponent* transform = scene.tryGetTransform(entity);
    return transform == nullptr ? world : world * localMatrix(*transform);
}

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

RenderFrame SceneRenderer::buildFrame(const SceneDocument& scene,
                                      const int viewportWidth,
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

    const glm::mat4 cameraWorld = worldMatrix(scene, cameraEntity);
    if (std::abs(glm::determinant(cameraWorld)) < 0.000001F) {
        return frame;
    }

    const float nearPlane = std::max(camera->nearPlane, 0.001F);
    const float farPlane = std::max(camera->farPlane, nearPlane + 0.001F);
    const float aspect = static_cast<float>(viewportWidth) /
                         static_cast<float>(viewportHeight);

    RenderView view;
    view.view = glm::inverse(cameraWorld);
    view.projection = glm::perspective(glm::radians(camera->verticalFovDegrees),
                                       aspect, nearPlane, farPlane);
    view.cameraPosition = glm::vec3{cameraWorld[3]};
    view.valid = true;
    return buildFrame(scene, view);
}

RenderFrame SceneRenderer::buildFrame(const SceneDocument& scene,
                                      const RenderView& view) const {
    RenderFrame frame;
    if (!view.valid) {
        return frame;
    }

    frame.view = view.view;
    frame.projection = view.projection;
    frame.hasCamera = true;
    frame.items.reserve(scene.entities().size());

    for (const auto& [entity, metadata] : scene.entities()) {
        (void)metadata;
        const MeshRendererComponent* meshRenderer = scene.tryGetMeshRenderer(entity);
        if (meshRenderer == nullptr || !meshRenderer->visible ||
            meshRenderer->meshAsset == MissingAsset) {
            continue;
        }

        frame.items.push_back(RenderItem{
            entity,
            meshRenderer->meshAsset,
            meshRenderer->materialAsset,
            worldMatrix(scene, entity),
        });
    }

    return frame;
}

} // namespace renderlab
