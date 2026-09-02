#include "renderer/ShaderLibrary.h"

#include <fstream>
#include <iterator>
#include <utility>

namespace renderlab {
namespace {

std::optional<std::string> readTextFile(const std::filesystem::path& path) {
    std::ifstream stream{path, std::ios::binary};
    if (!stream) {
        return std::nullopt;
    }
    return std::string{std::istreambuf_iterator<char>{stream},
                       std::istreambuf_iterator<char>{}};
}

} // namespace

ShaderLibrary::ShaderLibrary(std::filesystem::path rootDirectory)
    : rootDirectory_(std::move(rootDirectory).lexically_normal()) {
    entries_.reserve(MaxShaderAssets + 1U);
    // 零号槽位保留给默认构造的空 ShaderHandle。
    entries_.emplace_back();
}

ShaderHandle ShaderLibrary::registerShader(std::string id, ShaderAsset asset) {
    if (id.empty() || asset.vertexSource.empty() || asset.fragmentSource.empty()) {
        return {};
    }
    if (const auto existing = shadersById_.find(id);
        existing != shadersById_.end()) {
        return existing->second;
    }
    if (entries_.size() - 1U >= MaxShaderAssets) {
        return {};
    }

    const std::uint32_t index = static_cast<std::uint32_t>(entries_.size());
    entries_.push_back(Entry{.id = id, .asset = std::move(asset)});
    const ShaderHandle handle{index, entries_.back().generation};
    shadersById_.emplace(std::move(id), handle);
    return handle;
}

ShaderHandle ShaderLibrary::find(const std::string_view id) const {
    const auto iterator = shadersById_.find(std::string{id});
    return iterator == shadersById_.end() ? ShaderHandle{} : iterator->second;
}

const ShaderAsset* ShaderLibrary::tryGet(const ShaderHandle handle) const noexcept {
    if (!handle.valid() || handle.index >= entries_.size()) {
        return nullptr;
    }
    const Entry& entry = entries_[handle.index];
    return entry.generation == handle.generation && entry.asset.has_value()
               ? &*entry.asset : nullptr;
}

std::optional<ShaderSourceBundle>
ShaderLibrary::load(const ShaderHandle handle, std::string& error) const {
    error.clear();
    const ShaderAsset* asset = tryGet(handle);
    if (asset == nullptr) {
        error = "Shader handle is invalid";
        return std::nullopt;
    }

    const auto vertexPath = resolveSourcePath(asset->vertexSource);
    const auto fragmentPath = resolveSourcePath(asset->fragmentSource);
    if (!vertexPath.has_value() || !fragmentPath.has_value()) {
        error = "Shader source path must stay inside the shader root";
        return std::nullopt;
    }

    auto vertexSource = readTextFile(*vertexPath);
    if (!vertexSource.has_value()) {
        error = "Unable to read vertex shader: " + vertexPath->string();
        return std::nullopt;
    }
    auto fragmentSource = readTextFile(*fragmentPath);
    if (!fragmentSource.has_value()) {
        error = "Unable to read fragment shader: " + fragmentPath->string();
        return std::nullopt;
    }
    return ShaderSourceBundle{.vertexSource = std::move(*vertexSource),
                              .fragmentSource = std::move(*fragmentSource)};
}

const std::filesystem::path& ShaderLibrary::rootDirectory() const noexcept {
    return rootDirectory_;
}

std::optional<std::filesystem::path> ShaderLibrary::resolveSourcePath(
    const std::filesystem::path& relativePath) const {
    if (relativePath.empty() || relativePath.is_absolute()) {
        return std::nullopt;
    }
    for (const auto& component : relativePath) {
        if (component == "..") {
            return std::nullopt;
        }
    }
    return (rootDirectory_ / relativePath).lexically_normal();
}

} // namespace renderlab
