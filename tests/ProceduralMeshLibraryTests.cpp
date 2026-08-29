#include "assets/AssetRegistry.h"
#include "assets/ProceduralMeshLibrary.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <glm/geometric.hpp>

TEST_CASE("procedural mesh library reuses unit primitives") {
    renderlab::AssetRegistry registry;
    renderlab::ProceduralMeshLibrary meshes{registry};

    const renderlab::MeshHandle firstCube = meshes.unitCube();
    const renderlab::MeshHandle secondCube = meshes.unitCube();
    const renderlab::MeshHandle firstPlane = meshes.unitPlane();
    const renderlab::MeshHandle secondPlane = meshes.unitPlane();

    REQUIRE(firstCube.valid());
    REQUIRE(firstCube == secondCube);
    REQUIRE(firstPlane.valid());
    REQUIRE(firstPlane == secondPlane);
    REQUIRE(firstCube != firstPlane);

    REQUIRE(registry.tryGetMesh(firstCube)->primitives.front().vertices.size() ==
            24);
    REQUIRE(registry.tryGetMesh(firstPlane)->primitives.front().vertices.size() ==
            4);
}

TEST_CASE("UV spheres are cached by normalized topology") {
    renderlab::AssetRegistry registry;
    renderlab::ProceduralMeshLibrary meshes{registry};

    const renderlab::MeshHandle minimumSphere = meshes.uvSphere(1, 1);
    const renderlab::MeshHandle normalizedSphere = meshes.uvSphere(3, 2);
    const renderlab::MeshHandle detailedSphere = meshes.uvSphere(8, 4);

    REQUIRE(minimumSphere.valid());
    REQUIRE(minimumSphere == normalizedSphere);
    REQUIRE(detailedSphere.valid());
    REQUIRE(detailedSphere != minimumSphere);

    const renderlab::MeshAsset* asset = registry.tryGetMesh(detailedSphere);
    REQUIRE(asset != nullptr);
    REQUIRE(asset->primitives.size() == 1);

    const renderlab::MeshPrimitive& primitive = asset->primitives.front();
    REQUIRE(primitive.vertices.size() == (4U + 1U) * (8U + 1U));
    REQUIRE(primitive.indices.size() == 4U * 8U * 6U);
    for (const renderlab::Vertex& vertex : primitive.vertices) {
        REQUIRE(glm::length(vertex.position) == Catch::Approx(1.0F));
    }
}

TEST_CASE("procedural mesh library recreates a destroyed cached mesh") {
    renderlab::AssetRegistry registry;
    renderlab::ProceduralMeshLibrary meshes{registry};

    const renderlab::MeshHandle original = meshes.uvSphere(16, 8);
    REQUIRE(registry.destroyMesh(original));
    REQUIRE(registry.tryGetMesh(original) == nullptr);

    const renderlab::MeshHandle replacement = meshes.uvSphere(16, 8);
    REQUIRE(replacement.valid());
    REQUIRE(replacement.index == original.index);
    REQUIRE(replacement.generation != original.generation);
    REQUIRE(registry.tryGetMesh(replacement) != nullptr);
}
