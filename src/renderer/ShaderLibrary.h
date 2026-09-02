#pragma once

#include "assets/AssetHandle.h"
#include "assets/ShaderAsset.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace renderlab {

inline constexpr std::uint32_t MaxShaderAssets = 256;

// 管理稳定 Shader ID，并从统一资源根目录加载 API 无关的源码快照。
class ShaderLibrary final {
public:
    explicit ShaderLibrary(std::filesystem::path rootDirectory);

    // 注册相对路径形式的 Shader 资产；相同稳定 ID 会返回已有句柄。
    ShaderHandle registerShader(std::string id, ShaderAsset asset);
    [[nodiscard]] ShaderHandle find(std::string_view id) const;
    [[nodiscard]] const ShaderAsset* tryGet(ShaderHandle handle) const noexcept;

    // 同时读取两个 Stage；任一文件无效时返回空值并填写可展示的错误信息。
    [[nodiscard]] std::optional<ShaderSourceBundle>
    load(ShaderHandle handle, std::string& error) const;

    [[nodiscard]] const std::filesystem::path& rootDirectory() const noexcept;

private:
    struct Entry {
        std::uint32_t generation{1};
        std::string id;
        std::optional<ShaderAsset> asset;
    };

    [[nodiscard]] std::optional<std::filesystem::path>
    resolveSourcePath(const std::filesystem::path& relativePath) const;

    std::filesystem::path rootDirectory_;
    std::vector<Entry> entries_;
    std::unordered_map<std::string, ShaderHandle> shadersById_;
};

} // namespace renderlab
