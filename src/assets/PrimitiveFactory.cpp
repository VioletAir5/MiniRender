#include "assets/PrimitiveFactory.h"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace renderlab::primitive_factory {
namespace {

// 追加四个不共享的顶点，使立方体相邻面可拥有不同法线和 UV。
void appendQuad(MeshPrimitive& primitive,
                const glm::vec3& p0,
                const glm::vec3& p1,
                const glm::vec3& p2,
                const glm::vec3& p3,
                const glm::vec3& normal,
                const glm::vec4& color) {
    const auto base = static_cast<std::uint32_t>(primitive.vertices.size());
    primitive.vertices.push_back({p0, normal, {0.0F, 0.0F}, color});
    primitive.vertices.push_back({p1, normal, {1.0F, 0.0F}, color});
    primitive.vertices.push_back({p2, normal, {1.0F, 1.0F}, color});
    primitive.vertices.push_back({p3, normal, {0.0F, 1.0F}, color});
    primitive.indices.insert(primitive.indices.end(), {
                                                          base,
                                                          base + 1U,
                                                          base + 2U,
                                                          base,
                                                          base + 2U,
                                                          base + 3U,
                                                      });
}

// 程序化图元未指定材质时使用的默认顶点色。
constexpr glm::vec4 white{1.0F, 1.0F, 1.0F, 1.0F};

} // namespace

MeshPrimitive createPlane(const float width, const float depth) {
    const float halfWidth = std::abs(width) * 0.5F;
    const float halfDepth = std::abs(depth) * 0.5F;
    MeshPrimitive primitive;
    primitive.vertices = {
        {{-halfWidth, 0.0F, -halfDepth},
         {0.0F, 1.0F, 0.0F},
         {0.0F, 0.0F},
         {1.0F, 0.2F, 0.2F, 1.0F}},
        {{halfWidth, 0.0F, -halfDepth},
         {0.0F, 1.0F, 0.0F},
         {1.0F, 0.0F},
         {0.2F, 1.0F, 0.2F, 1.0F}},
        {{halfWidth, 0.0F, halfDepth},
         {0.0F, 1.0F, 0.0F},
         {1.0F, 1.0F},
         {0.2F, 0.2F, 1.0F, 1.0F}},
        {{-halfWidth, 0.0F, halfDepth},
         {0.0F, 1.0F, 0.0F},
         {0.0F, 1.0F},
         {1.0F, 1.0F, 0.2F, 1.0F}},
    };
    primitive.indices = {0, 2, 1, 0, 3, 2};
    return primitive;
}

MeshPrimitive createCube(const float size) {
    const float halfSize = std::abs(size) * 0.5F;
    MeshPrimitive primitive;
    primitive.vertices.reserve(24);
    primitive.indices.reserve(36);
    appendQuad(primitive,
               {-halfSize, -halfSize, halfSize},
               {halfSize, -halfSize, halfSize},
               {halfSize, halfSize, halfSize},
               {-halfSize, halfSize, halfSize},
               {0.0F, 0.0F, 1.0F}, white);
    appendQuad(primitive,
               {halfSize, -halfSize, -halfSize},
               {-halfSize, -halfSize, -halfSize},
               {-halfSize, halfSize, -halfSize},
               {halfSize, halfSize, -halfSize},
               {0.0F, 0.0F, -1.0F}, white);
    appendQuad(primitive,
               {halfSize, -halfSize, halfSize},
               {halfSize, -halfSize, -halfSize},
               {halfSize, halfSize, -halfSize},
               {halfSize, halfSize, halfSize},
               {1.0F, 0.0F, 0.0F}, white);
    appendQuad(primitive,
               {-halfSize, -halfSize, -halfSize},
               {-halfSize, -halfSize, halfSize},
               {-halfSize, halfSize, halfSize},
               {-halfSize, halfSize, -halfSize},
               {-1.0F, 0.0F, 0.0F}, white);
    appendQuad(primitive,
               {-halfSize, halfSize, halfSize},
               {halfSize, halfSize, halfSize},
               {halfSize, halfSize, -halfSize},
               {-halfSize, halfSize, -halfSize},
               {0.0F, 1.0F, 0.0F}, white);
    appendQuad(primitive,
               {-halfSize, -halfSize, -halfSize},
               {halfSize, -halfSize, -halfSize},
               {halfSize, -halfSize, halfSize},
               {-halfSize, -halfSize, halfSize},
               {0.0F, -1.0F, 0.0F}, white);
    return primitive;
}

MeshPrimitive createUvSphere(const float radius,
                             const std::uint32_t segments,
                             const std::uint32_t rings) {
    const float sphereRadius = std::abs(radius);
    // 限制最小拓扑，确保每一圈至少能组成三角形。
    const std::uint32_t segmentCount = std::max(segments, 3U);
    const std::uint32_t ringCount = std::max(rings, 2U);
    MeshPrimitive primitive;
    primitive.vertices.reserve(static_cast<std::size_t>(ringCount + 1U) *
                               (segmentCount + 1U));
    primitive.indices.reserve(static_cast<std::size_t>(ringCount) *
                              segmentCount * 6U);

    constexpr float pi = std::numbers::pi_v<float>;
    constexpr float twoPi = 2.0F * pi;
    for (std::uint32_t ring = 0; ring <= ringCount; ++ring) {
        const float v = static_cast<float>(ring) / static_cast<float>(ringCount);
        const float polarAngle = v * pi;
        const float sinPolar = std::sin(polarAngle);
        const float cosPolar = std::cos(polarAngle);
        // 首尾经线位置相同但 UV 不同，因此需要重复顶点消除纹理接缝。
        for (std::uint32_t segment = 0; segment <= segmentCount; ++segment) {
            const float u = static_cast<float>(segment) /
                            static_cast<float>(segmentCount);
            const float azimuth = u * twoPi;
            const glm::vec3 normal{
                sinPolar * std::cos(azimuth),
                cosPolar,
                sinPolar * std::sin(azimuth),
            };
            primitive.vertices.push_back({
                normal * sphereRadius, normal, {u, 1.0F - v}, white,
            });
        }
    }

    const std::uint32_t stride = segmentCount + 1U;
    for (std::uint32_t ring = 0; ring < ringCount; ++ring) {
        for (std::uint32_t segment = 0; segment < segmentCount; ++segment) {
            const std::uint32_t current = ring * stride + segment;
            const std::uint32_t next = current + stride;
            primitive.indices.insert(primitive.indices.end(), {
                                                                  current,
                                                                  current + 1U,
                                                                  next,
                                                                  current + 1U,
                                                                  next + 1U,
                                                                  next,
                                                              });
        }
    }
    return primitive;
}

} // namespace renderlab::primitive_factory
