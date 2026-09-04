#include "renderer/SceneRenderer.h"
#include "scene/SceneDocument.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>

TEST_CASE("scene renderer builds an API-neutral render frame") {
    renderlab::SceneDocument scene;

    const renderlab::EntityId camera = scene.createEntity("Camera");
    scene.addCamera(camera).primary = true;
    scene.tryGetTransform(camera)->position = {0.0F, 0.0F, 5.0F};

    const renderlab::EntityId light = scene.createEntity("Sun");
    scene.addLight(light).intensity = 2.5F;
    scene.tryGetTransform(light)->rotationDegrees = {-90.0F, 0.0F, 0.0F};

    const renderlab::EntityId parent = scene.createEntity("Parent");
    scene.tryGetTransform(parent)->position = {2.0F, 0.0F, 0.0F};

    const renderlab::EntityId cube = scene.createEntity("Cube", parent);
    scene.tryGetTransform(cube)->position = {1.0F, 0.0F, 0.0F};
    scene.addMeshRenderer(cube).meshAsset = renderlab::BuiltinCubeMeshAsset;

    const renderlab::RenderFrame frame = renderlab::SceneRenderer{}.buildFrame(scene, 1280, 720);

    REQUIRE(frame.hasCamera);
    REQUIRE(frame.items.size() == 1);
    REQUIRE_FALSE(frame.selectionOutline.has_value());
    REQUIRE(frame.items.front().entity == cube);
    REQUIRE(frame.items.front().meshAsset == renderlab::BuiltinCubeMeshAsset);
    REQUIRE(frame.items.front().model[3][0] == Catch::Approx(3.0F));
    REQUIRE(frame.view[3][2] == Catch::Approx(-5.0F));
    REQUIRE(frame.cameraPosition.z == Catch::Approx(5.0F));
    REQUIRE(frame.lights.size() == 1);
    REQUIRE(frame.lights.front().type == renderlab::LightType::Directional);
    REQUIRE(frame.lights.front().intensity == Catch::Approx(2.5F));
    REQUIRE(frame.lights.front().direction.y == Catch::Approx(-1.0F));
    REQUIRE(frame.lights.front().direction.z == Catch::Approx(0.0F).margin(0.0001F));
    REQUIRE(frame.shadowCasters.size() == 1);
    CHECK(frame.shadowCasters.front().entity == cube);
}

TEST_CASE("scene renderer extracts point and spot light data") {
    renderlab::SceneDocument scene;
    const renderlab::EntityId point = scene.createEntity("Point");
    auto& pointLight = scene.addLight(point);
    pointLight.type = renderlab::LightType::Point;
    pointLight.range = 15.0F;
    scene.tryGetTransform(point)->position = {1.0F, 2.0F, 3.0F};

    const renderlab::EntityId spot = scene.createEntity("Spot");
    auto& spotLight = scene.addLight(spot);
    spotLight.type = renderlab::LightType::Spot;
    spotLight.innerConeDegrees = 15.0F;
    spotLight.outerConeDegrees = 25.0F;

    renderlab::RenderView view;
    view.valid = true;
    const renderlab::RenderFrame frame = renderlab::SceneRenderer{}.buildFrame(scene, view);

    REQUIRE(frame.lights.size() == 2);
    const auto pointData =
        std::find_if(frame.lights.begin(), frame.lights.end(),
                     [point](const auto& light) { return light.entity == point; });
    REQUIRE(pointData != frame.lights.end());
    CHECK(pointData->type == renderlab::LightType::Point);
    CHECK(pointData->position.x == Catch::Approx(1.0F));
    CHECK(pointData->range == Catch::Approx(15.0F));

    const auto spotData = std::find_if(frame.lights.begin(), frame.lights.end(),
                                       [spot](const auto& light) { return light.entity == spot; });
    REQUIRE(spotData != frame.lights.end());
    CHECK(spotData->innerConeCosine > spotData->outerConeCosine);
}

TEST_CASE("scene renderer produces an empty frame without a camera") {
    renderlab::SceneDocument scene;
    const renderlab::EntityId cube = scene.createEntity("Cube");
    scene.addMeshRenderer(cube).meshAsset = renderlab::BuiltinCubeMeshAsset;

    const renderlab::RenderFrame frame = renderlab::SceneRenderer{}.buildFrame(scene, 1280, 720);

    REQUIRE_FALSE(frame.hasCamera);
    REQUIRE(frame.items.empty());
}
