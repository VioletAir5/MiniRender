#include "scene/EntitySnapshot.h"

#include "scene/SceneDocument.h"

#include <catch2/catch_test_macros.hpp>

TEST_CASE("entity snapshot restores a complete subtree with stable IDs") {
    renderlab::SceneDocument scene;
    const renderlab::EntityId root = scene.createEntity("Root");
    const renderlab::EntityId child = scene.createEntity("Child", root);

    scene.tryGetTransform(root)->position = {1.0F, 2.0F, 3.0F};
    scene.addMeshRenderer(root) = renderlab::MeshRendererComponent{
        .meshAsset = {7, 2}, .materialAsset = {4, 3}, .visible = false};
    scene.addCamera(child).primary = true;
    scene.addLight(child).intensity = 3.5F;

    const auto snapshot = renderlab::captureEntitySnapshot(scene, root);
    REQUIRE(snapshot.has_value());
    REQUIRE(scene.destroyEntity(root));

    REQUIRE(renderlab::restoreEntitySnapshot(scene, *snapshot, true) == root);
    REQUIRE(scene.contains(root));
    REQUIRE(scene.contains(child));
    CHECK(scene.tryGetEntity(child)->parent == root);
    CHECK(scene.tryGetTransform(root)->position.y == 2.0F);
    CHECK(scene.tryGetMeshRenderer(root)->meshAsset.index == 7);
    CHECK_FALSE(scene.tryGetMeshRenderer(root)->visible);
    CHECK(scene.tryGetCamera(child)->primary);
    CHECK(scene.tryGetLight(child)->intensity == 3.5F);
}

TEST_CASE("entity snapshot can clone a subtree with fresh IDs") {
    renderlab::SceneDocument scene;
    const renderlab::EntityId root = scene.createEntity("Root");
    const renderlab::EntityId child = scene.createEntity("Child", root);
    scene.addMeshRenderer(child).meshAsset = {5, 1};

    const auto snapshot = renderlab::captureEntitySnapshot(scene, root);
    REQUIRE(snapshot.has_value());
    const renderlab::EntityId clone = renderlab::restoreEntitySnapshot(scene, *snapshot, false);

    REQUIRE(clone != renderlab::NullEntity);
    REQUIRE(clone != root);
    const auto* cloneMetadata = scene.tryGetEntity(clone);
    REQUIRE(cloneMetadata != nullptr);
    REQUIRE(cloneMetadata->children.size() == 1);
    const renderlab::EntityId clonedChild = cloneMetadata->children.front();
    CHECK(clonedChild != child);
    CHECK(scene.tryGetEntity(clonedChild)->parent == clone);
    CHECK(scene.tryGetMeshRenderer(clonedChild)->meshAsset.index == 5);
}

TEST_CASE("preserved snapshot restore rejects ID conflicts without changing the scene") {
    renderlab::SceneDocument scene;
    const renderlab::EntityId entity = scene.createEntity("Existing");
    const auto snapshot = renderlab::captureEntitySnapshot(scene, entity);
    REQUIRE(snapshot.has_value());

    CHECK(renderlab::restoreEntitySnapshot(scene, *snapshot, true) == renderlab::NullEntity);
    CHECK(scene.entities().size() == 1);
    CHECK(scene.tryGetEntity(entity)->name == "Existing");
}
