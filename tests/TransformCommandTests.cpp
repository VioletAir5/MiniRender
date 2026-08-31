#include "editor/TransformCommand.h"

#include "scene/SceneDocument.h"

#include <QUndoStack>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("transform command supports undo and redo with complete snapshots") {
    renderlab::SceneDocument scene;
    const renderlab::EntityId entity = scene.createEntity("Editable");
    const renderlab::TransformComponent before = *scene.tryGetTransform(entity);

    renderlab::TransformComponent after = before;
    after.position = {1.0F, 2.0F, 3.0F};
    after.rotationDegrees = {10.0F, 20.0F, 30.0F};
    after.scale = {2.0F, 3.0F, 4.0F};

    QUndoStack undoStack;
    undoStack.push(new renderlab::TransformCommand(scene, entity, before, after,
                                                   QStringLiteral("Change Transform")));

    REQUIRE(undoStack.count() == 1);
    CHECK(scene.tryGetTransform(entity)->position.x == 1.0F);
    CHECK(scene.tryGetTransform(entity)->rotationDegrees.y == 20.0F);
    CHECK(scene.tryGetTransform(entity)->scale.z == 4.0F);

    undoStack.undo();
    CHECK(scene.tryGetTransform(entity)->position.x == 0.0F);
    CHECK(scene.tryGetTransform(entity)->rotationDegrees.y == 0.0F);
    CHECK(scene.tryGetTransform(entity)->scale.z == 1.0F);

    undoStack.redo();
    CHECK(scene.tryGetTransform(entity)->position.x == 1.0F);
    CHECK(scene.tryGetTransform(entity)->rotationDegrees.y == 20.0F);
    CHECK(scene.tryGetTransform(entity)->scale.z == 4.0F);
}
