#pragma once

#include "scene/EntitySnapshot.h"

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>

namespace renderlab {

class AssetRegistry;

// glTF 导入的事务结果；成功时 snapshot 保存可由编辑器命令创建的完整实体子树。
struct GltfImportResult {
    std::optional<EntitySnapshot> snapshot;
    std::string error;
    std::size_t meshCount{0};
    std::size_t materialCount{0};
    std::size_t textureCount{0};

    [[nodiscard]] explicit operator bool() const noexcept {
        return snapshot.has_value();
    }
};

// 将 glTF 2.0 / GLB 文件转换为 API 无关资产和实体快照。
class GltfImporter final {
public:
    explicit GltfImporter(AssetRegistry& registry) noexcept;

    [[nodiscard]] GltfImportResult import(const std::filesystem::path& path);

private:
    AssetRegistry& registry_;
};

} // namespace renderlab
