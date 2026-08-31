#include "editor/EntityCommands.h"

#include "scene/SceneDocument.h"

#include <QUndoStack>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("create entity command creates and restores the same entity") {
    renderlab::SceneDocument scene;
    renderlab::EntitySnapshot snapshot;
    snapshot.name = "Cube";
    snapshot.transform.position.x = 2.0F;
    snapshot.meshRenderer = renderlab::MeshRendererComponent{.meshAsset = {3, 1}};

    QUndoStack undoStack;
    auto* command =
        new renderlab::CreateEntityCommand(scene, snapshot, QStringLiteral("Create Cube"));
    undoStack.push(command);
    const renderlab::EntityId created = command->entity();

    REQUIRE(scene.contains(created));
    CHECK(scene.tryGetTransform(created)->position.x == 2.0F);
    CHECK(scene.tryGetMeshRenderer(created)->meshAsset.index == 3);

    undoStack.undo();
    CHECK_FALSE(scene.contains(created));
    undoStack.redo();
    CHECK(scene.contains(created));
    CHECK(command->entity() == created);
}

TEST_CASE("delete entity command restores a complete hierarchy") {
    renderlab::SceneDocument scene;
    const renderlab::EntityId parent = scene.createEntity("Parent");
    const renderlab::EntityId child = scene.createEntity("Child", parent);
    scene.addLight(child).range = 25.0F;

    QUndoStack undoStack;
    undoStack.push(
        new renderlab::DeleteEntityCommand(scene, parent, QStringLiteral("Delete Entity")));
    CHECK_FALSE(scene.contains(parent));
    CHECK_FALSE(scene.contains(child));

    undoStack.undo();
    REQUIRE(scene.contains(parent));
    REQUIRE(scene.contains(child));
    CHECK(scene.tryGetEntity(child)->parent == parent);
    CHECK(scene.tryGetLight(child)->range == 25.0F);

    undoStack.redo();
    CHECK_FALSE(scene.contains(parent));
}

TEST_CASE("duplicate entity command assigns fresh stable IDs") {
    renderlab::SceneDocument scene;
    const renderlab::EntityId source = scene.createEntity("Object");
    const renderlab::EntityId sourceChild = scene.createEntity("Child", source);

    QUndoStack undoStack;
    auto* command = new renderlab::DuplicateEntityCommand(
        scene, source, QStringLiteral("Duplicate Entity"));
    undoStack.push(command);
    const renderlab::EntityId duplicate = command->entity();

    REQUIRE(duplicate != renderlab::NullEntity);
    REQUIRE(duplicate != source);
    REQUIRE(scene.tryGetEntity(duplicate)->children.size() == 1);
    CHECK(scene.tryGetEntity(duplicate)->name == "Object Copy");
    CHECK(scene.tryGetEntity(duplicate)->children.front() != sourceChild);

    undoStack.undo();
    CHECK_FALSE(scene.contains(duplicate));
    undoStack.redo();
    CHECK(scene.contains(duplicate));
    CHECK(command->entity() == duplicate);
}

TEST_CASE("rename entity command supports undo and redo") {
    renderlab::SceneDocument scene;
    const renderlab::EntityId entity = scene.createEntity("Before");
    QUndoStack undoStack;

    undoStack.push(new renderlab::RenameEntityCommand(
        scene, entity, "Before", "After", QStringLiteral("Rename Entity")));
    CHECK(scene.tryGetEntity(entity)->name == "After");
    undoStack.undo();
    CHECK(scene.tryGetEntity(entity)->name == "Before");
    undoStack.redo();
    CHECK(scene.tryGetEntity(entity)->name == "After");
}
