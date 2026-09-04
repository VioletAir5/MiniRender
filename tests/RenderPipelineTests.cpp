#include "renderer/RenderFrame.h"
#include "renderer/pipeline/RenderPipeline.h"

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {

class RecordingPass final : public renderlab::IRenderPass {
  public:
    RecordingPass(std::string name, std::vector<std::string>& events,
                  const bool initializeResult = true)
        : name_(std::move(name)), events_(events), initializeResult_(initializeResult) {}

    bool initialize() override {
        events_.push_back("initialize:" + name_);
        return initializeResult_;
    }

    void shutdown() noexcept override {
        events_.push_back("shutdown:" + name_);
    }

    void resize(const int width, const int height) override {
        events_.push_back("resize:" + name_ + ":" + std::to_string(width) + "x" +
                          std::to_string(height));
    }

    void execute(const renderlab::RenderPassExecutionContext& context) override {
        events_.push_back("execute:" + name_ + ":" + std::to_string(context.frameNumber));
    }

  private:
    std::string name_;
    std::vector<std::string>& events_;
    bool initializeResult_{true};
};

} // namespace

TEST_CASE("render pipeline creates and executes backend passes in descriptor order") {
    std::vector<std::string> events;
    renderlab::RenderPassFactory factory;
    REQUIRE(factory.registerType(
        "test.first", [&events] { return std::make_unique<RecordingPass>("first", events); }));
    REQUIRE(factory.registerType(
        "test.second", [&events] { return std::make_unique<RecordingPass>("second", events); }));

    const renderlab::RenderPipelineDescriptor descriptor{
        .passes = {
            {.name = "First", .type = "test.first"},
            {.name = "Second", .type = "test.second"},
        }};

    renderlab::RenderPipeline pipeline;
    std::string error;
    REQUIRE(pipeline.build(descriptor, factory, error));
    REQUIRE(error.empty());
    CHECK(pipeline.size() == 2);
    CHECK(pipeline.contains("First"));

    pipeline.resize(1280, 720);
    REQUIRE(pipeline.initialize(error));

    const renderlab::RenderFrame frame;
    pipeline.execute(renderlab::RenderPassExecutionContext{
        .frame = frame,
        .frameNumber = 42,
        .viewportWidth = 1280,
        .viewportHeight = 720,
    });
    pipeline.shutdown();

    CHECK(events == std::vector<std::string>{
                        "initialize:first",
                        "resize:first:1280x720",
                        "initialize:second",
                        "resize:second:1280x720",
                        "execute:first:42",
                        "execute:second:42",
                        "shutdown:second",
                        "shutdown:first",
                    });
}

TEST_CASE("optional render pass failure does not stop the pipeline") {
    std::vector<std::string> events;
    renderlab::RenderPassFactory factory;
    REQUIRE(factory.registerType("test.optional", [&events] {
        return std::make_unique<RecordingPass>("optional", events, false);
    }));
    REQUIRE(factory.registerType("test.required", [&events] {
        return std::make_unique<RecordingPass>("required", events);
    }));

    renderlab::RenderPipeline pipeline;
    std::string error;
    REQUIRE(pipeline.build(
        renderlab::RenderPipelineDescriptor{
            .passes =
                {
                    {.name = "Optional", .type = "test.optional", .required = false},
                    {.name = "Required", .type = "test.required"},
                }},
        factory, error));
    REQUIRE(pipeline.initialize(error));

    const renderlab::RenderFrame frame;
    pipeline.execute(renderlab::RenderPassExecutionContext{
        .frame = frame,
        .frameNumber = 7,
    });
    pipeline.shutdown();

    CHECK(events == std::vector<std::string>{
                        "initialize:optional",
                        "shutdown:optional",
                        "initialize:required",
                        "execute:required:7",
                        "shutdown:required",
                    });
}

TEST_CASE("required render pass failure rolls back initialized passes") {
    std::vector<std::string> events;
    renderlab::RenderPassFactory factory;
    REQUIRE(factory.registerType(
        "test.good", [&events] { return std::make_unique<RecordingPass>("good", events); }));
    REQUIRE(factory.registerType(
        "test.bad", [&events] { return std::make_unique<RecordingPass>("bad", events, false); }));

    renderlab::RenderPipeline pipeline;
    std::string error;
    REQUIRE(pipeline.build(
        renderlab::RenderPipelineDescriptor{.passes =
                                                {
                                                    {.name = "Good", .type = "test.good"},
                                                    {.name = "Bad", .type = "test.bad"},
                                                }},
        factory, error));
    CHECK_FALSE(pipeline.initialize(error));
    CHECK_FALSE(pipeline.initialized());
    CHECK_FALSE(error.empty());
    CHECK(events == std::vector<std::string>{
                        "initialize:good",
                        "initialize:bad",
                        "shutdown:bad",
                        "shutdown:good",
                    });
}

TEST_CASE("render pipeline rejects unknown pass types and duplicate names") {
    std::vector<std::string> events;
    renderlab::RenderPassFactory factory;
    REQUIRE(factory.registerType(
        "test.pass", [&events] { return std::make_unique<RecordingPass>("pass", events); }));

    renderlab::RenderPipeline pipeline;
    std::string error;
    CHECK_FALSE(pipeline.build(
        renderlab::RenderPipelineDescriptor{.passes =
                                                {
                                                    {.name = "Missing", .type = "test.missing"},
                                                }},
        factory, error));
    CHECK_FALSE(error.empty());

    CHECK_FALSE(pipeline.build(
        renderlab::RenderPipelineDescriptor{.passes =
                                                {
                                                    {.name = "Same", .type = "test.pass"},
                                                    {.name = "Same", .type = "test.pass"},
                                                }},
        factory, error));
    CHECK_FALSE(error.empty());
}
