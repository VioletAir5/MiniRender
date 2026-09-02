#include "renderer/ShaderLibrary.h"

#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>

namespace {

class TemporaryShaderDirectory final {
public:
    TemporaryShaderDirectory() {
        const auto suffix =
            std::chrono::steady_clock::now().time_since_epoch().count();
        path = std::filesystem::temp_directory_path() /
               ("renderlab-shader-test-" + std::to_string(suffix));
        std::filesystem::create_directories(path);
        std::ofstream{path / "simple.vert"} << "vertex-source";
        std::ofstream{path / "simple.frag"} << "fragment-source";
    }

    ~TemporaryShaderDirectory() {
        std::error_code ignored;
        std::filesystem::remove_all(path, ignored);
    }

    std::filesystem::path path;
};

} // namespace

TEST_CASE("shader library registers stable IDs and loads both stages") {
    TemporaryShaderDirectory directory;
    renderlab::ShaderLibrary library{directory.path};
    const renderlab::ShaderAsset asset{
        .name = "Simple",
        .vertexSource = "simple.vert",
        .fragmentSource = "simple.frag",
    };

    const renderlab::ShaderHandle first =
        library.registerShader("shader.simple", asset);
    const renderlab::ShaderHandle duplicate =
        library.registerShader("shader.simple", asset);

    REQUIRE(first.valid());
    CHECK(duplicate == first);
    CHECK(library.find("shader.simple") == first);

    std::string error;
    const auto source = library.load(first, error);
    REQUIRE(source.has_value());
    CHECK(error.empty());
    CHECK(source->vertexSource == "vertex-source");
    CHECK(source->fragmentSource == "fragment-source");
}

TEST_CASE("shader library rejects source paths outside its root") {
    TemporaryShaderDirectory directory;
    renderlab::ShaderLibrary library{directory.path};
    const renderlab::ShaderHandle shader = library.registerShader(
        "shader.invalid", renderlab::ShaderAsset{
            .vertexSource = "../outside.vert",
            .fragmentSource = "simple.frag",
        });

    std::string error;
    CHECK_FALSE(library.load(shader, error).has_value());
    CHECK_FALSE(error.empty());
}
