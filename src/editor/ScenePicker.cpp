#include "editor/ScenePicker.h"

#include "assets/AssetRegistry.h"
#include "renderer/RenderFrame.h"

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/vec4.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <vector>

namespace renderlab {
namespace {

// 浮点比较容差，用于排除不可逆矩阵、退化射线和平行三角形。
constexpr float IntersectionEpsilon = 0.000001F;

// CPU 拾取使用的世界空间射线，direction 始终保持单位长度。
struct Ray {
    glm::vec3 origin{0.0F};
    glm::vec3 direction{0.0F, 0.0F, -1.0F};
};

// 将左上角为原点的视口像素反投影为世界空间射线。
std::optional<Ray> makeRay(const RenderFrame& frame, const float viewportX,
                           const float viewportY, const int viewportWidth,
                           const int viewportHeight) {
    if (!frame.hasCamera || viewportWidth <= 0 || viewportHeight <= 0) {
        return std::nullopt;
    }

    const glm::mat4 viewProjection = frame.projection * frame.view;
    if (std::abs(glm::determinant(viewProjection)) < IntersectionEpsilon) {
        return std::nullopt;
    }

    // 屏幕坐标先转换到 OpenGL 的 [-1, 1] NDC，Y 轴方向需要翻转。
    const float normalizedX =
        (2.0F * viewportX) / static_cast<float>(viewportWidth) - 1.0F;
    const float normalizedY =
        1.0F - (2.0F * viewportY) / static_cast<float>(viewportHeight);
    const glm::mat4 inverseViewProjection = glm::inverse(viewProjection);

    // 反投影近、远裁剪面上的同一像素，再由两点构造射线方向。
    glm::vec4 nearPoint =
        inverseViewProjection * glm::vec4{normalizedX, normalizedY, -1.0F, 1.0F};
    glm::vec4 farPoint =
        inverseViewProjection * glm::vec4{normalizedX, normalizedY, 1.0F, 1.0F};
    if (std::abs(nearPoint.w) < IntersectionEpsilon ||
        std::abs(farPoint.w) < IntersectionEpsilon) {
        return std::nullopt;
    }

    nearPoint /= nearPoint.w;
    farPoint /= farPoint.w;
    const glm::vec3 segment = glm::vec3{farPoint - nearPoint};
    const float segmentLength = glm::length(segment);
    if (segmentLength < IntersectionEpsilon) {
        return std::nullopt;
    }

    return Ray{glm::vec3{nearPoint}, segment / segmentLength};
}

// 返回射线到三角形交点的非负距离；未命中或三角形退化时返回空值。
std::optional<float> intersectTriangle(const Ray& ray, const glm::vec3& first,
                                       const glm::vec3& second,
                                       const glm::vec3& third) {
    // Möller–Trumbore 算法同时支持三角形正反面，适合编辑器拾取。
    const glm::vec3 firstEdge = second - first;
    const glm::vec3 secondEdge = third - first;
    const glm::vec3 perpendicular = glm::cross(ray.direction, secondEdge);
    const float determinant = glm::dot(firstEdge, perpendicular);
    if (std::abs(determinant) < IntersectionEpsilon) {
        return std::nullopt;
    }

    const float inverseDeterminant = 1.0F / determinant;
    const glm::vec3 originOffset = ray.origin - first;
    const float firstCoordinate =
        glm::dot(originOffset, perpendicular) * inverseDeterminant;
    if (firstCoordinate < 0.0F || firstCoordinate > 1.0F) {
        return std::nullopt;
    }

    const glm::vec3 secondPerpendicular = glm::cross(originOffset, firstEdge);
    const float secondCoordinate =
        glm::dot(ray.direction, secondPerpendicular) * inverseDeterminant;
    if (secondCoordinate < 0.0F ||
        firstCoordinate + secondCoordinate > 1.0F) {
        return std::nullopt;
    }

    const float distance =
        glm::dot(secondEdge, secondPerpendicular) * inverseDeterminant;
    return distance >= 0.0F ? std::optional<float>{distance} : std::nullopt;
}

// 一次性把 Primitive 顶点变换到世界空间，避免每个三角形重复变换共享顶点。
std::vector<glm::vec3> worldVertices(const MeshPrimitive& primitive,
                                     const glm::mat4& model) {
    std::vector<glm::vec3> vertices;
    vertices.reserve(primitive.vertices.size());
    for (const Vertex& vertex : primitive.vertices) {
        vertices.emplace_back(model * glm::vec4{vertex.position, 1.0F});
    }
    return vertices;
}

// 统一遍历索引和非索引三角形，并跳过越界索引保护编辑器稳定性。
template<typename Visitor>
void visitTriangles(const MeshPrimitive& primitive,
                    const std::vector<glm::vec3>& vertices,
                    Visitor&& visitor) {
    if (!primitive.indices.empty()) {
        for (std::size_t index = 0; index + 2 < primitive.indices.size();
             index += 3) {
            const std::uint32_t first = primitive.indices[index];
            const std::uint32_t second = primitive.indices[index + 1];
            const std::uint32_t third = primitive.indices[index + 2];
            if (first >= vertices.size() || second >= vertices.size() ||
                third >= vertices.size()) {
                continue;
            }
            visitor(vertices[first], vertices[second], vertices[third]);
        }
        return;
    }

    for (std::size_t index = 0; index + 2 < vertices.size(); index += 3) {
        visitor(vertices[index], vertices[index + 1], vertices[index + 2]);
    }
}

} // namespace

std::optional<ScenePickResult> ScenePicker::pick(
    const RenderFrame& frame, const AssetRegistry& registry,
    const float viewportX, const float viewportY, const int viewportWidth,
    const int viewportHeight) {
    const std::optional<Ray> ray = makeRay(
        frame, viewportX, viewportY, viewportWidth, viewportHeight);
    if (!ray.has_value()) {
        return std::nullopt;
    }

    // 初始距离设为无穷大，遍历顺序因此不会影响最终选中的实体。
    ScenePickResult nearest;
    nearest.distance = std::numeric_limits<float>::max();

    for (const RenderItem& item : frame.items) {
        const MeshAsset* mesh = registry.tryGetMesh(item.meshAsset);
        if (mesh == nullptr) {
            continue;
        }

        for (const MeshPrimitive& primitive : mesh->primitives) {
            const std::vector<glm::vec3> vertices =
                worldVertices(primitive, item.model);
            visitTriangles(primitive, vertices,
                           [&](const glm::vec3& first,
                               const glm::vec3& second,
                               const glm::vec3& third) {
                               const std::optional<float> distance =
                                   intersectTriangle(*ray, first, second, third);
                               if (distance.has_value() &&
                                   *distance < nearest.distance) {
                                   nearest.entity = item.entity;
                                   nearest.distance = *distance;
                                   nearest.worldPosition =
                                       ray->origin + ray->direction * *distance;
                               }
                           });
        }
    }

    return nearest.entity == NullEntity
               ? std::nullopt
               : std::optional<ScenePickResult>{nearest};
}

std::optional<WorldBounds> ScenePicker::worldBounds(
    const RenderFrame& frame, const AssetRegistry& registry,
    const EntityId entity) {
    // 先计算世界空间 AABB，再以其中心包围所有顶点得到保守包围球。
    glm::vec3 minimum{std::numeric_limits<float>::max()};
    glm::vec3 maximum{std::numeric_limits<float>::lowest()};
    std::vector<glm::vec3> vertices;

    for (const RenderItem& item : frame.items) {
        if (item.entity != entity) {
            continue;
        }

        const MeshAsset* mesh = registry.tryGetMesh(item.meshAsset);
        if (mesh == nullptr) {
            continue;
        }

        for (const MeshPrimitive& primitive : mesh->primitives) {
            std::vector<glm::vec3> primitiveVertices =
                worldVertices(primitive, item.model);
            for (const glm::vec3& vertex : primitiveVertices) {
                minimum = glm::min(minimum, vertex);
                maximum = glm::max(maximum, vertex);
            }
            vertices.insert(vertices.end(), primitiveVertices.begin(),
                            primitiveVertices.end());
        }
    }

    if (vertices.empty()) {
        return std::nullopt;
    }

    WorldBounds bounds;
    bounds.center = (minimum + maximum) * 0.5F;
    for (const glm::vec3& vertex : vertices) {
        bounds.radius =
            std::max(bounds.radius, glm::distance(bounds.center, vertex));
    }
    return bounds;
}

} // namespace renderlab
