#include "serialization/SceneSerializer.h"

#include "assets/AssetRegistry.h"
#include "scene/SceneDocument.h"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>

TEST_CASE("scene serializer round trips hierarchy components and persistent assets") {
    renderlab::AssetRegistry assets;
    const auto mesh = assets.createMesh("test:mesh/cube", renderlab::MeshAsset{"Cube"});
    const auto material = assets.createMaterial(
        "test:material/red", renderlab::MaterialAsset{.name = "Red"});
    renderlab::SceneDocument source;
    const auto root = source.createEntity("Root");
    const auto child = source.createEntity("Child", root);
    source.tryGetTransform(child)->position = {1.0F, 2.0F, 3.0F};
    source.addMeshRenderer(child) = {.meshAsset = mesh, .materialAsset = material};
    source.addLight(root).intensity = 2.0F;

    const auto path = std::filesystem::temp_directory_path() / "renderlab_scene_test.json";
    REQUIRE(renderlab::SceneSerializer::save(source, assets, path));
    renderlab::SceneDocument loaded;
    REQUIRE(renderlab::SceneSerializer::load(loaded, assets, path));

    REQUIRE(loaded.contains(root));
    REQUIRE(loaded.contains(child));
    CHECK(loaded.tryGetEntity(child)->parent == root);
    CHECK(loaded.tryGetTransform(child)->position.y == 2.0F);
    CHECK(loaded.tryGetMeshRenderer(child)->meshAsset == mesh);
    CHECK(loaded.tryGetMeshRenderer(child)->materialAsset == material);
    CHECK(loaded.tryGetLight(root)->intensity == 2.0F);
    std::filesystem::remove(path);
}

TEST_CASE("scene loading is transactional when an asset is missing") {
    renderlab::AssetRegistry assets;
    const auto mesh = assets.createMesh("test:mesh", renderlab::MeshAsset{"Mesh"});
    renderlab::SceneDocument source;
    const auto entity = source.createEntity("Saved");
    source.addMeshRenderer(entity).meshAsset = mesh;
    const auto path = std::filesystem::temp_directory_path() / "renderlab_bad_asset_scene.json";
    REQUIRE(renderlab::SceneSerializer::save(source, assets, path));

    renderlab::AssetRegistry emptyAssets;
    renderlab::SceneDocument destination;
    destination.createEntity("Keep Me");
    CHECK_FALSE(renderlab::SceneSerializer::load(destination, emptyAssets, path));
    CHECK(destination.entities().size() == 1);
    std::filesystem::remove(path);
}
