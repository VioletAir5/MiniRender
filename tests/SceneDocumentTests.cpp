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

TEST_CASE("transform setters update an existing entity and reject unknown IDs") {
    renderlab::SceneDocument scene;
    const renderlab::EntityId entity = scene.createEntity("Editable");

    renderlab::TransformComponent completeTransform;
    completeTransform.position = {-1.0F, -2.0F, -3.0F};
    completeTransform.rotationDegrees = {-10.0F, -20.0F, -30.0F};
    completeTransform.scale = {0.5F, 0.75F, 1.25F};
    REQUIRE(scene.setTransform(entity, completeTransform));

    REQUIRE(scene.setPosition(entity, {1.0F, 2.0F, 3.0F}));
    REQUIRE(scene.setRotation(entity, {10.0F, 20.0F, 30.0F}));
    REQUIRE(scene.setScale(entity, {2.0F, 3.0F, 4.0F}));

    const renderlab::TransformComponent* transform = scene.tryGetTransform(entity);
    REQUIRE(transform != nullptr);
    CHECK(transform->position.x == 1.0F);
    CHECK(transform->position.y == 2.0F);
    CHECK(transform->position.z == 3.0F);
    CHECK(transform->rotationDegrees.x == 10.0F);
    CHECK(transform->rotationDegrees.y == 20.0F);
    CHECK(transform->rotationDegrees.z == 30.0F);
    CHECK(transform->scale.x == 2.0F);
    CHECK(transform->scale.y == 3.0F);
    CHECK(transform->scale.z == 4.0F);

    CHECK_FALSE(scene.setTransform(renderlab::NullEntity, {}));
    CHECK_FALSE(scene.setPosition(renderlab::NullEntity, {}));
    CHECK_FALSE(scene.setRotation(renderlab::NullEntity, {}));
    CHECK_FALSE(scene.setScale(renderlab::NullEntity, {}));
}
