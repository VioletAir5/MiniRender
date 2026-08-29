#include "assets/PrimitiveFactory.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <glm/geometric.hpp>

namespace {

void requireValidIndices(const renderlab::MeshPrimitive& primitive) {
    for (const std::uint32_t index : primitive.indices) {
        REQUIRE(index < primitive.vertices.size());
    }
}

void requireOutwardTriangleNormals(
    const renderlab::MeshPrimitive& primitive,
    const float minimumAlignment = 0.999F) {

    for (std::size_t index = 0; index < primitive.indices.size(); index += 3) {
        const renderlab::Vertex& a =
            primitive.vertices[primitive.indices[index]];
        const renderlab::Vertex& b =
            primitive.vertices[primitive.indices[index + 1]];
        const renderlab::Vertex& c =
            primitive.vertices[primitive.indices[index + 2]];

        const glm::vec3 crossProduct =
            glm::cross(b.position - a.position, c.position - a.position);
        if (glm::length(crossProduct) < 0.000001F) {
            continue;
        }

        const glm::vec3 triangleNormal = glm::normalize(crossProduct);
        REQUIRE(glm::dot(triangleNormal, a.normal) > minimumAlignment);
    }
}

} // namespace

TEST_CASE("plane primitive has valid upward-facing geometry") {
    const renderlab::MeshPrimitive plane =
        renderlab::primitive_factory::createPlane(-2.0F, -4.0F);

    REQUIRE(plane.vertices.size() == 4);
    REQUIRE(plane.indices.size() == 6);
    requireValidIndices(plane);
    requireOutwardTriangleNormals(plane);

    for (const renderlab::Vertex& vertex : plane.vertices) {
        REQUIRE(vertex.normal.x == Catch::Approx(0.0F));
        REQUIRE(vertex.normal.y == Catch::Approx(1.0F));
        REQUIRE(vertex.normal.z == Catch::Approx(0.0F));
    }
}

TEST_CASE("cube primitive has separate vertices for hard-edged faces") {
    const renderlab::MeshPrimitive cube =
        renderlab::primitive_factory::createCube(2.0F);

    REQUIRE(cube.vertices.size() == 24);
    REQUIRE(cube.indices.size() == 36);
    requireValidIndices(cube);
    requireOutwardTriangleNormals(cube);

    for (const renderlab::Vertex& vertex : cube.vertices) {
        REQUIRE(glm::length(vertex.normal) == Catch::Approx(1.0F));
    }
}

TEST_CASE("UV sphere clamps tessellation and produces valid indices") {
    constexpr std::uint32_t minimumSegments = 3;
    constexpr std::uint32_t minimumRings = 2;

    const renderlab::MeshPrimitive sphere =
        renderlab::primitive_factory::createUvSphere(2.0F, 1, 1);

    REQUIRE(sphere.vertices.size() ==
            (minimumRings + 1U) * (minimumSegments + 1U));
    REQUIRE(sphere.indices.size() ==
            minimumRings * minimumSegments * 6U);
    requireValidIndices(sphere);
    requireOutwardTriangleNormals(sphere, 0.0F);

    for (const renderlab::Vertex& vertex : sphere.vertices) {
        REQUIRE(glm::length(vertex.normal) == Catch::Approx(1.0F));
        REQUIRE(glm::length(vertex.position) == Catch::Approx(2.0F));
    }
}
