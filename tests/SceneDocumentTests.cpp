#include "scene/SceneDocument.h"

#include <catch2/catch_test_macros.hpp>

#include <stdexcept>

TEST_CASE("entities receive stable unique IDs and default transforms") {
    renderlab::SceneDocument scene;

    const renderlab::EntityId first = scene.createEntity("First");
    const renderlab::EntityId second = scene.createEntity("Second");

    REQUIRE(first != renderlab::NullEntity);
    REQUIRE(second != renderlab::NullEntity);
    REQUIRE(first != second);
    REQUIRE(scene.contains(first));
    REQUIRE(scene.tryGetTransform(first) != nullptr);
    REQUIRE(scene.tryGetEntity(9999) == nullptr);
}

TEST_CASE("components are attached to an existing entity") {
    renderlab::SceneDocument scene;
    const renderlab::EntityId camera = scene.createEntity("Camera");

    scene.addCamera(camera).primary = true;

    REQUIRE(scene.tryGetCamera(camera) != nullptr);
    REQUIRE(scene.tryGetCamera(camera)->primary);
    REQUIRE(scene.tryGetLight(camera) == nullptr);
    REQUIRE_THROWS_AS(scene.addLight(9999), std::out_of_range);
}

TEST_CASE("parenting updates both sides and rejects cycles") {
    renderlab::SceneDocument scene;
    const renderlab::EntityId root = scene.createEntity("Root");
    const renderlab::EntityId child = scene.createEntity("Child", root);
    const renderlab::EntityId grandchild = scene.createEntity("Grandchild", child);

    REQUIRE(scene.tryGetEntity(child)->parent == root);
    REQUIRE(scene.tryGetEntity(root)->children == std::vector<renderlab::EntityId>{child});
    REQUIRE_FALSE(scene.setParent(root, grandchild));
    REQUIRE_FALSE(scene.setParent(root, root));
    REQUIRE(scene.setParent(child, renderlab::NullEntity));
    REQUIRE(scene.tryGetEntity(child)->parent == renderlab::NullEntity);
    REQUIRE(scene.tryGetEntity(root)->children.empty());
}

TEST_CASE("destroying a parent recursively removes its descendants and components") {
    renderlab::SceneDocument scene;
    const renderlab::EntityId root = scene.createEntity("Root");
    const renderlab::EntityId child = scene.createEntity("Child", root);
    const renderlab::EntityId grandchild = scene.createEntity("Grandchild", child);
    scene.addMeshRenderer(grandchild);

    REQUIRE(scene.destroyEntity(root));
    REQUIRE_FALSE(scene.contains(root));
    REQUIRE_FALSE(scene.contains(child));
    REQUIRE_FALSE(scene.contains(grandchild));
    REQUIRE(scene.tryGetMeshRenderer(grandchild) == nullptr);
    REQUIRE_FALSE(scene.destroyEntity(root));
}

TEST_CASE("an entity cannot be created under an unknown parent") {
    renderlab::SceneDocument scene;
    REQUIRE_THROWS_AS(scene.createEntity("Invalid", 42), std::invalid_argument);
    REQUIRE(scene.entities().empty());
}

