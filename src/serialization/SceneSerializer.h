#pragma once

#include <filesystem>
#include <string>

namespace renderlab {

class AssetRegistry;
class SceneDocument;

// 场景读写结果，失败时携带可直接显示给用户的原因。
struct SceneIoResult {
    bool success{false};
    std::string error;

    [[nodiscard]] explicit operator bool() const noexcept { return success; }
};

// 将 API 无关的场景数据保存为带版本号的 JSON 文档。
class SceneSerializer final {
  public:
    [[nodiscard]] static SceneIoResult save(const SceneDocument& scene,
                                            const AssetRegistry& assets,
                                            const std::filesystem::path& path);
    [[nodiscard]] static SceneIoResult load(SceneDocument& destination,
                                            const AssetRegistry& assets,
                                            const std::filesystem::path& path);
};

} // namespace renderlab
