#include "app/TransformInspector.h"

#include "scene/SceneDocument.h"

#include <QApplication>
#include <QSlider>
#include <QUndoStack>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

namespace {

// QWidget 测试必须先创建唯一的 QApplication 实例。
QApplication& testApplication() {
    static int argumentCount = 1;
    static char applicationName[] = "RenderLabTests";
    static char* arguments[] = {applicationName, nullptr};
    static QApplication application(argumentCount, arguments);
    return application;
}

} // namespace

TEST_CASE("TransformInspector groups one slider drag into one command") {
    (void)testApplication();

    renderlab::SceneDocument scene;
    const renderlab::EntityId entity = scene.createEntity("Cube");
    QUndoStack undoStack;
    renderlab::TransformInspector inspector;
    inspector.setUndoStack(&undoStack);
    inspector.setEntity(&scene, entity);

    auto* slider = inspector.findChild<QSlider*>(QStringLiteral("positionXSlider"));
    REQUIRE(slider != nullptr);

    slider->setSliderDown(true);
    slider->setValue(100);
    slider->setValue(200);
    REQUIRE(undoStack.count() == 0);
    slider->setSliderDown(false);

    REQUIRE(undoStack.count() == 1);
    REQUIRE(scene.tryGetTransform(entity)->position.x == Catch::Approx(2.0F));

    undoStack.undo();
    REQUIRE(scene.tryGetTransform(entity)->position.x == Catch::Approx(0.0F));
    undoStack.redo();
    REQUIRE(scene.tryGetTransform(entity)->position.x == Catch::Approx(2.0F));
}

TEST_CASE("TransformInspector commits a pending drag before switching entity") {
    (void)testApplication();

    renderlab::SceneDocument scene;
    const renderlab::EntityId first = scene.createEntity("First");
    const renderlab::EntityId second = scene.createEntity("Second");
    QUndoStack undoStack;
    renderlab::TransformInspector inspector;
    inspector.setUndoStack(&undoStack);
    inspector.setEntity(&scene, first);

    auto* slider = inspector.findChild<QSlider*>(QStringLiteral("positionXSlider"));
    REQUIRE(slider != nullptr);
    slider->setSliderDown(true);
    slider->setValue(300);

    // 选择切换必须先把旧实体的拖动事务提交到撤销栈。
    inspector.setEntity(&scene, second);
    slider->setSliderDown(false);

    REQUIRE(undoStack.count() == 1);
    REQUIRE(scene.tryGetTransform(first)->position.x == Catch::Approx(3.0F));
    REQUIRE(scene.tryGetTransform(second)->position.x == Catch::Approx(0.0F));

    undoStack.undo();
    REQUIRE(scene.tryGetTransform(first)->position.x == Catch::Approx(0.0F));
    REQUIRE(scene.tryGetTransform(second)->position.x == Catch::Approx(0.0F));
}

TEST_CASE("TransformInspector does not create a command when a drag has no change") {
    (void)testApplication();

    renderlab::SceneDocument scene;
    const renderlab::EntityId entity = scene.createEntity("Cube");
    QUndoStack undoStack;
    renderlab::TransformInspector inspector;
    inspector.setUndoStack(&undoStack);
    inspector.setEntity(&scene, entity);

    auto* slider = inspector.findChild<QSlider*>(QStringLiteral("positionXSlider"));
    REQUIRE(slider != nullptr);
    slider->setSliderDown(true);
    slider->setSliderDown(false);

    REQUIRE(undoStack.count() == 0);
}
