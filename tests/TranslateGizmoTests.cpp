#include "editor/TranslateGizmo.h"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("translate gizmo projects axes and selects the closest handle") {
    const glm::mat4 view = glm::lookAt(glm::vec3{0.0F, 0.0F, 5.0F}, glm::vec3{0.0F},
                                      glm::vec3{0.0F, 1.0F, 0.0F});
    const glm::mat4 projection = glm::perspective(glm::radians(60.0F), 16.0F / 9.0F,
                                                  0.1F, 100.0F);
    const auto geometry = renderlab::TranslateGizmo::project(
        {}, view, projection, 1280, 720);
    REQUIRE(geometry.visible);
    CHECK(renderlab::TranslateGizmo::hitTest(geometry, geometry.endpoints[0]) ==
          renderlab::GizmoAxis::X);
    CHECK(renderlab::TranslateGizmo::hitTest(geometry, {10.0F, 10.0F}) ==
          renderlab::GizmoAxis::None);
}

TEST_CASE("translate gizmo converts screen drag into one world axis") {
    renderlab::TranslateGizmoGeometry geometry;
    geometry.visible = true;
    geometry.origin = {100.0F, 100.0F};
    geometry.endpoints[0] = {200.0F, 100.0F};
    geometry.worldScale = 2.0F;
    const glm::vec3 delta = renderlab::TranslateGizmo::dragDelta(
        geometry, renderlab::GizmoAxis::X, {50.0F, 20.0F});
    CHECK(delta.x == Catch::Approx(1.0F));
    CHECK(delta.y == Catch::Approx(0.0F));
    CHECK(delta.z == Catch::Approx(0.0F));
}
