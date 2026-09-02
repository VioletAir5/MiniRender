#include "assets/MeshTangents.h"

#include <glm/geometric.hpp>

#include <cmath>
#include <vector>

namespace renderlab {
namespace {

constexpr float Epsilon = 0.000001F;

// 在 UV 退化时构造一个与法线正交的稳定方向，避免向 Shader 传入零向量。
glm::vec3 fallbackTangent(const glm::vec3& normal) noexcept {
    const glm::vec3 axis = std::abs(normal.y) < 0.999F
                               ? glm::vec3{0.0F, 1.0F, 0.0F}
                               : glm::vec3{1.0F, 0.0F, 0.0F};
    return glm::normalize(glm::cross(axis, normal));
}

} // namespace

bool generateTangents(MeshPrimitive& primitive) noexcept {
    std::vector<glm::vec3> tangentSums(primitive.vertices.size(), glm::vec3{0.0F});
    std::vector<glm::vec3> bitangentSums(primitive.vertices.size(), glm::vec3{0.0F});
    bool generatedAny = false;

    for (std::size_t index = 0; index + 2U < primitive.indices.size(); index += 3U) {
        const std::uint32_t first = primitive.indices[index];
        const std::uint32_t second = primitive.indices[index + 1U];
        const std::uint32_t third = primitive.indices[index + 2U];
        if (first >= primitive.vertices.size() ||
            second >= primitive.vertices.size() ||
            third >= primitive.vertices.size()) {
            continue;
        }

        const Vertex& a = primitive.vertices[first];
        const Vertex& b = primitive.vertices[second];
        const Vertex& c = primitive.vertices[third];
        const glm::vec3 edge1 = b.position - a.position;
        const glm::vec3 edge2 = c.position - a.position;
        const glm::vec2 uvEdge1 = b.texCoord - a.texCoord;
        const glm::vec2 uvEdge2 = c.texCoord - a.texCoord;
        const float determinant = uvEdge1.x * uvEdge2.y -
                                  uvEdge1.y * uvEdge2.x;
        if (std::abs(determinant) <= Epsilon) {
            continue;
        }

        const float reciprocal = 1.0F / determinant;
        const glm::vec3 tangent =
            (edge1 * uvEdge2.y - edge2 * uvEdge1.y) * reciprocal;
        const glm::vec3 bitangent =
            (edge2 * uvEdge1.x - edge1 * uvEdge2.x) * reciprocal;
        for (const std::uint32_t vertexIndex : {first, second, third}) {
            tangentSums[vertexIndex] += tangent;
            bitangentSums[vertexIndex] += bitangent;
        }
        generatedAny = true;
    }

    for (std::size_t index = 0; index < primitive.vertices.size(); ++index) {
        Vertex& vertex = primitive.vertices[index];
        glm::vec3 normal = vertex.normal;
        if (glm::dot(normal, normal) <= Epsilon) {
            normal = {0.0F, 1.0F, 0.0F};
        } else {
            normal = glm::normalize(normal);
        }

        // Gram–Schmidt 正交化，移除累计切线沿法线方向的误差。
        glm::vec3 tangent = tangentSums[index] -
                            normal * glm::dot(normal, tangentSums[index]);
        if (glm::dot(tangent, tangent) <= Epsilon) {
            tangent = fallbackTangent(normal);
        } else {
            tangent = glm::normalize(tangent);
        }

        const float handedness =
            glm::dot(glm::cross(normal, tangent), bitangentSums[index]) < 0.0F
                ? -1.0F : 1.0F;
        vertex.tangent = glm::vec4{tangent, handedness};
    }
    return generatedAny;
}

} // namespace renderlab
