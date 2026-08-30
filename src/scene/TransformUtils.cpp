#include "scene/TransformUtils.h"

#include "scene/SceneDocument.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/trigonometric.hpp>

namespace renderlab {

glm::mat4 localTransformMatrix(const TransformComponent& transform) {
    // 固定使用 T * Rz * Ry * Rx * S，保证渲染、拾取和编辑工具解释一致。
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

glm::mat4 worldTransformMatrix(const SceneDocument& scene,
                               const EntityId entity) {
    glm::mat4 world{1.0F};
    const EntityMetadata* metadata = scene.tryGetEntity(entity);
    if (metadata != nullptr && metadata->parent != NullEntity) {
        // SceneDocument 禁止父子环，因此递归会在根实体处稳定终止。
        world = worldTransformMatrix(scene, metadata->parent);
    }

    const TransformComponent* transform = scene.tryGetTransform(entity);
    return transform == nullptr ? world : world * localTransformMatrix(*transform);
}

} // namespace renderlab
