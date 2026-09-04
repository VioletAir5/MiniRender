#include "renderer/pipeline/BuiltInRenderPipeline.h"
#include "renderer/pipeline/RenderGraph.h"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

using renderlab::RenderGraphCompiler;
using renderlab::RenderGraphDescriptor;
using renderlab::RenderGraphPassDescriptor;
using renderlab::RenderResourceDescriptor;
using renderlab::RenderResourceFormat;

TEST_CASE("render graph derives pass order from transient resource dependencies") {
    const RenderGraphDescriptor descriptor{
        .resources =
            {
                {.name = "shadow.depth", .format = RenderResourceFormat::Depth32Float},
                {.name = "surface.color",
                 .format = RenderResourceFormat::Rgba8Unorm,
                 .external = true},
            },
        .passes =
            {
                {.name = "Forward", .reads = {"shadow.depth"}, .writes = {"surface.color"}},
                {.name = "Shadow", .writes = {"shadow.depth"}},
            },
    };

    renderlab::CompiledRenderGraph compiled;
    std::string error;
    REQUIRE(RenderGraphCompiler{}.compile(descriptor, compiled, error));
    CHECK(compiled.resources.size() == 2);
    REQUIRE(compiled.passes.size() == 2);
    CHECK(compiled.passes[0].name == "Shadow");
    CHECK(compiled.passes[1].name == "Forward");
    CHECK(compiled.passes[1].dependsOn == std::vector<std::string>{"Shadow"});
}

TEST_CASE("render graph rejects cycles and unknown resources") {
    renderlab::CompiledRenderGraph compiled;
    std::string error;

    CHECK_FALSE(RenderGraphCompiler{}.compile(
        RenderGraphDescriptor{
            .passes =
                {
                    {.name = "A", .dependsOn = {"B"}},
                    {.name = "B", .dependsOn = {"A"}},
                },
        },
        compiled, error));
    CHECK(error.find("cycle") != std::string::npos);

    CHECK_FALSE(RenderGraphCompiler{}.compile(
        RenderGraphDescriptor{
            .passes =
                {
                    {.name = "Reader", .reads = {"missing"}},
                },
        },
        compiled, error));
    CHECK(error.find("unknown resource") != std::string::npos);
}

TEST_CASE("render graph rejects missing and duplicate transient producers") {
    const RenderResourceDescriptor resource{
        .name = "scene.color",
        .format = RenderResourceFormat::Rgba16Float,
    };
    renderlab::CompiledRenderGraph compiled;
    std::string error;

    CHECK_FALSE(RenderGraphCompiler{}.compile(
        RenderGraphDescriptor{
            .resources = {resource},
            .passes =
                {
                    {.name = "Reader", .reads = {"scene.color"}},
                },
        },
        compiled, error));
    CHECK(error.find("no producer") != std::string::npos);

    CHECK_FALSE(RenderGraphCompiler{}.compile(
        RenderGraphDescriptor{
            .resources = {resource},
            .passes =
                {
                    {.name = "First", .writes = {"scene.color"}},
                    {.name = "Second", .writes = {"scene.color"}},
                },
        },
        compiled, error));
    CHECK(error.find("multiple producers") != std::string::npos);
}

TEST_CASE("render graph keeps declaration order when passes are independent") {
    const RenderGraphDescriptor descriptor{
        .passes =
            {
                {.name = "First"},
                {.name = "Second"},
                {.name = "Third"},
            },
    };
    renderlab::CompiledRenderGraph compiled;
    std::string error;
    REQUIRE(RenderGraphCompiler{}.compile(descriptor, compiled, error));
    REQUIRE(compiled.passes.size() == 3);
    CHECK(compiled.passes[0].name == "First");
    CHECK(compiled.passes[1].name == "Second");
    CHECK(compiled.passes[2].name == "Third");
}

TEST_CASE("render resource extents support viewport scaling and fixed sizes") {
    const auto halfViewport = renderlab::resolveRenderResourceExtent(
        RenderResourceDescriptor{
            .name = "half",
            .widthScale = 0.5F,
            .heightScale = 0.25F,
        },
        1920, 1080);
    REQUIRE(halfViewport.has_value());
    CHECK(halfViewport->width == 960);
    CHECK(halfViewport->height == 270);

    const auto fixed = renderlab::resolveRenderResourceExtent(
        RenderResourceDescriptor{
            .name = "shadow",
            .sizeMode = renderlab::RenderResourceSizeMode::Fixed,
            .width = 2048,
            .height = 2048,
        },
        0, 0);
    REQUIRE(fixed.has_value());
    CHECK(fixed->width == 2048);
    CHECK(fixed->height == 2048);
}

TEST_CASE("default editor graph schedules shadow before forward rendering") {
    const renderlab::RenderPipelineDescriptor pipeline = renderlab::defaultEditorRenderPipeline();
    renderlab::RenderGraphDescriptor graph{.resources = pipeline.resources};
    for (const renderlab::RenderPassDescriptor& pass : pipeline.passes) {
        graph.passes.push_back({
            .name = pass.name,
            .dependsOn = pass.dependsOn,
            .reads = pass.reads,
            .writes = pass.writes,
        });
    }

    renderlab::CompiledRenderGraph compiled;
    std::string error;
    REQUIRE(renderlab::RenderGraphCompiler{}.compile(graph, compiled, error));
    REQUIRE(compiled.passes.size() >= 2);
    CHECK(compiled.passes[0].name == "Directional Shadow");
    CHECK(compiled.passes[1].name == "Forward");
}
