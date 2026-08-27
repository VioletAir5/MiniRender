#include "editor/EditorCamera.h"
#include "renderer/SceneRenderer.h"
#include "scene/SceneDocument.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <glm/glm.hpp>

TEST_CASE("editor camera creates a valid default render view") {
    renderlab::EditorCamera camera;

    const renderlab::RenderView view = camera.renderView(1280, 720);
    const glm::vec4 originInView = view.view * glm::vec4{0.0F, 0.0F, 0.0F, 1.0F};

    REQUIRE(view.valid);
    REQUIRE(view.cameraPosition.x == Catch::Approx(0.0F));
    REQUIRE(view.cameraPosition.y == Catch::Approx(0.0F));
    REQUIRE(view.cameraPosition.z == Catch::Approx(5.0F));
    REQUIRE(originInView.z == Catch::Approx(-5.0F));
}

TEST_CASE("editor camera supports orbit pan zoom and pitch limits") {
    renderlab::EditorCamera camera;
    const glm::vec3 initialPosition = camera.position();

    camera.orbit(120.0F, -40.0F);
    REQUIRE(glm::distance(camera.position(), initialPosition) > 0.01F);

    const float initialDistance = camera.distance();
    camera.zoom(1.0F);
    REQUIRE(camera.distance() < initialDistance);

    const glm::vec3 initialTarget = camera.target();
    camera.pan(20.0F, -10.0F, 720);
    REQUIRE(glm::distance(camera.target(), initialTarget) > 0.001F);

    camera.orbit(0.0F, -10000.0F);
    REQUIRE(camera.pitchDegrees() == Catch::Approx(89.0F));
}

TEST_CASE("scene renderer accepts an editor view without a scene camera") {
    renderlab::SceneDocument scene;
    const renderlab::EntityId cube = scene.createEntity("Cube");
    scene.addMeshRenderer(cube).meshAsset = renderlab::BuiltinCubeMeshAsset;

    renderlab::EditorCamera camera;
    const renderlab::RenderFrame frame =
        renderlab::SceneRenderer{}.buildFrame(scene, camera.renderView(1280, 720));

    REQUIRE(frame.hasCamera);
    REQUIRE(frame.items.size() == 1);
    REQUIRE(frame.items.front().entity == cube);
}
