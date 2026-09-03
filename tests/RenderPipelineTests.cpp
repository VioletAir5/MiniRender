#include "renderer/RenderFrame.h"
#include "renderer/RenderPipeline.h"
#include "renderer/rhi/IRenderBackend.h"
#include "renderer/rhi/IRenderCommandList.h"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

namespace {

class RecordingCommandList final : public renderlab::IRenderCommandList {
public:
    void setStencilState(const renderlab::StencilState& state) override {
        events.push_back(state.enabled ? "stencil:on" : "stencil:off");
    }

    void setDepthWriteEnabled(const bool enabled) override {
        events.push_back(enabled ? "depth-write:on" : "depth-write:off");
    }

    void setBlendEnabled(const bool enabled) override {
        events.push_back(enabled ? "blend:on" : "blend:off");
    }

    void setCullEnabled(const bool enabled) override {
        events.push_back(enabled ? "cull:on" : "cull:off");
    }

    void drawMesh(const renderlab::RenderItem&,
                  const renderlab::MeshDrawParameters& parameters) override {
        events.push_back(parameters.overrideColor.has_value()
                             ? "mesh:outline"
                             : "mesh:surface");
    }

    void drawEditorGrid(const glm::mat4&, const glm::mat4&) override {
        events.push_back("grid");
    }

    std::vector<std::string> events;
};

class RecordingBackend final : public renderlab::IRenderBackend {
public:
    bool initialize() override { return true; }
    void shutdown() override {}
    void resize(int, int) override {}

    void beginFrame(const renderlab::RenderFrame&) override {
        events.push_back("begin");
    }

    renderlab::IRenderCommandList& commandList() override {
        return commands;
    }

    void endFrame() override {
        events.push_back("end");
    }

    RecordingCommandList commands;
    std::vector<std::string> events;
};

} // namespace

TEST_CASE("render pipeline executes API-neutral passes in dependency order") {
    renderlab::RenderFrame frame;
    frame.hasCamera = true;
    frame.items.push_back(renderlab::RenderItem{
        .entity = renderlab::EntityId{7},
    });
    frame.selectionOutline = renderlab::SelectionOutline{
        .entity = renderlab::EntityId{7},
    };

    renderlab::RenderPipeline pipeline;
    RecordingBackend backend;
    pipeline.render(frame, backend);

    REQUIRE(backend.events == std::vector<std::string>{"begin", "end"});
    REQUIRE(backend.commands.events == std::vector<std::string>{
        "stencil:on",
        "mesh:surface",
        "stencil:on",
        "depth-write:off",
        "mesh:outline",
        "depth-write:on",
        "stencil:off",
        "blend:on",
        "cull:off",
        "depth-write:off",
        "grid",
        "depth-write:on",
        "blend:off",
    });
}

TEST_CASE("render pipeline keeps frame lifecycle without a camera") {
    renderlab::RenderFrame frame;
    RecordingBackend backend;

    renderlab::RenderPipeline{}.render(frame, backend);

    REQUIRE(backend.events == std::vector<std::string>{"begin", "end"});
    REQUIRE(backend.commands.events.empty());
}

TEST_CASE("render pipeline finds and disables passes by stable name") {
    renderlab::RenderPipeline pipeline;

    REQUIRE(pipeline.findRenderPass("Forward") != nullptr);
    REQUIRE(pipeline.findRenderPass("Outline") != nullptr);
    REQUIRE(pipeline.findRenderPass("Grid") != nullptr);
    REQUIRE(pipeline.findRenderPass("Missing") == nullptr);
    REQUIRE_FALSE(pipeline.setRenderPassEnabled("Missing", false));
    REQUIRE(pipeline.setRenderPassEnabled("Outline", false));
    REQUIRE_FALSE(pipeline.findRenderPass("Outline")->isEnabled());

    renderlab::RenderFrame frame;
    frame.hasCamera = true;
    frame.items.push_back(renderlab::RenderItem{
        .entity = renderlab::EntityId{9},
    });
    frame.selectionOutline = renderlab::SelectionOutline{
        .entity = renderlab::EntityId{9},
    };

    RecordingBackend backend;
    pipeline.render(frame, backend);

    REQUIRE(backend.commands.events == std::vector<std::string>{
        "stencil:on",
        "mesh:surface",
        "stencil:off",
        "blend:on",
        "cull:off",
        "depth-write:off",
        "grid",
        "depth-write:on",
        "blend:off",
    });
}
