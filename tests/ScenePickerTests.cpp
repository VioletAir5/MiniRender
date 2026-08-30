#include "assets/AssetRegistry.h"
#include "editor/ScenePicker.h"
#include "renderer/RenderFrame.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <utility>

namespace {

// 创建覆盖视口中心的最小三角形资产，隔离程序化网格实现对测试的影响。
renderlab::MeshHandle createTriangle(renderlab::AssetRegistry& registry) {
    renderlab::MeshPrimitive primitive;
    primitive.vertices = {
        renderlab::Vertex{.position = {-1.0F, -1.0F, 0.0F}},
        renderlab::Vertex{.position = {1.0F, -1.0F, 0.0F}},
        renderlab::Vertex{.position = {0.0F, 1.0F, 0.0F}},
    };
    primitive.indices = {0, 1, 2};

    renderlab::MeshAsset mesh;
    mesh.name = "Picking Triangle";
    mesh.primitives.push_back(std::move(primitive));
    return registry.createMesh(std::move(mesh));
}

// 构造固定相机帧，使像素射线和命中距离具有确定结果。
renderlab::RenderFrame createFrame() {
    renderlab::RenderFrame frame;
    frame.view = glm::lookAt(glm::vec3{0.0F, 0.0F, 5.0F},
                             glm::vec3{0.0F},
                             glm::vec3{0.0F, 1.0F, 0.0F});
    frame.projection = glm::perspective(glm::radians(60.0F), 4.0F / 3.0F,
                                        0.1F, 100.0F);
    frame.hasCamera = true;
    return frame;
}

} // namespace

TEST_CASE("scene picker selects the nearest triangle under the cursor") {
    renderlab::AssetRegistry registry;
    const renderlab::MeshHandle mesh = createTriangle(registry);
    renderlab::RenderFrame frame = createFrame();

    frame.items.push_back(renderlab::RenderItem{
        .entity = 1,
        .meshAsset = mesh,
        .model = glm::translate(glm::mat4{1.0F}, {0.0F, 0.0F, -1.0F}),
    });
    frame.items.push_back(renderlab::RenderItem{
        .entity = 2,
        .meshAsset = mesh,
        .model = glm::translate(glm::mat4{1.0F}, {0.0F, 0.0F, 1.0F}),
    });

    const auto result = renderlab::ScenePicker::pick(
        frame, registry, 400.0F, 300.0F, 800, 600);

    REQUIRE(result.has_value());
    REQUIRE(result->entity == 2);
    REQUIRE(result->worldPosition.z == Catch::Approx(1.0F));
}

TEST_CASE("scene picker returns no entity when the cursor misses all geometry") {
    renderlab::AssetRegistry registry;
    const renderlab::MeshHandle mesh = createTriangle(registry);
    renderlab::RenderFrame frame = createFrame();
    frame.items.push_back(renderlab::RenderItem{
        .entity = 1,
        .meshAsset = mesh,
    });

    REQUIRE_FALSE(renderlab::ScenePicker::pick(
                      frame, registry, 5.0F, 5.0F, 800, 600)
                      .has_value());
}

TEST_CASE("scene picker calculates transformed world bounds for focusing") {
    renderlab::AssetRegistry registry;
    const renderlab::MeshHandle mesh = createTriangle(registry);
    renderlab::RenderFrame frame = createFrame();
    frame.items.push_back(renderlab::RenderItem{
        .entity = 7,
        .meshAsset = mesh,
        .model = glm::translate(glm::mat4{1.0F}, {2.0F, 3.0F, 4.0F}),
    });

    const auto bounds =
        renderlab::ScenePicker::worldBounds(frame, registry, 7);

    REQUIRE(bounds.has_value());
    REQUIRE(bounds->center.x == Catch::Approx(2.0F));
    REQUIRE(bounds->center.y == Catch::Approx(3.0F));
    REQUIRE(bounds->center.z == Catch::Approx(4.0F));
    REQUIRE(bounds->radius == Catch::Approx(1.4142135F));
}
