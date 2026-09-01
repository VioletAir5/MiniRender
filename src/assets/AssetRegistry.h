#pragma once

#include "assets/AssetHandle.h"
#include "assets/MeshAsset.h"
#include "assets/MaterialAsset.h"
#include "assets/TextureAsset.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>


namespace renderlab {

// 当前允许同时驻留的资产上限，均不包含保留的空槽位。
inline constexpr std::uint32_t MaxMeshAssets = 512;
inline constexpr std::uint32_t MaxMaterialAssets = 512;
inline constexpr std::uint32_t MaxTextureAssets = 512;

// 描述资产槽位从创建、加载到可用或失败的生命周期状态。
enum class AssetState {
    Empty,
    Loading,
    Ready,
    Failed,
};

// 注册表内部槽位；generation 识别槽位复用，revision 识别内容更新。
template<typename T>
struct AssetSlot {
    std::uint32_t generation{1};
    std::uint32_t revision{0};
    AssetState state{AssetState::Empty};
    std::unique_ptr<T> asset;
};

// 一次只读查询结果；asset 指针只在对应资产未被销毁期间有效。
struct MeshAssetView {
    const MeshAsset* asset{nullptr};
    std::uint32_t revision{0};
};

// 材质查询结果；asset 指针只在对应材质未被销毁期间有效。
struct MaterialAssetView {
    const MaterialAsset* asset{nullptr};
    std::uint32_t revision{0};
};

struct TextureAssetView {
    const TextureAsset* asset{nullptr};
    std::uint32_t revision{0};
};

// 集中拥有 CPU 侧资产，并通过带代际句柄提供稳定、可校验的访问。
class AssetRegistry {
public:
    // 建立零号保留槽位并预留容量，避免正常上限内的 vector 重分配。
    AssetRegistry();
    // 接管网格并返回有效句柄；达到容量上限时返回空句柄。
    MeshHandle createMesh(MeshAsset mesh);
    // 使用跨运行稳定的标识注册网格；标识已存在时返回原资产。
    MeshHandle createMesh(std::string id, MeshAsset mesh);
    // 销毁匹配代际的资产并回收槽位；句柄失效时返回 false。
    bool destroyMesh(MeshHandle handle);

    // 返回匹配句柄的只读资产；句柄无效、代际过期或未就绪时返回 nullptr。
    [[nodiscard]]
    const MeshAsset* tryGetMesh(MeshHandle handle) const noexcept;

    // 同时返回资产与修订号，供 GPU 缓存判断是否需要重新上传。
    [[nodiscard]]
    std::optional<MeshAssetView>
    tryGetMeshView(MeshHandle handle) const noexcept;
    [[nodiscard]] MeshHandle findMesh(std::string_view id) const noexcept;
    [[nodiscard]] std::optional<std::string> meshId(MeshHandle handle) const;

    // 接管材质并返回有效句柄；达到容量上限时返回空句柄。
    MaterialHandle createMaterial(MaterialAsset material);
    // 使用跨运行稳定的标识注册材质；标识已存在时返回原资产。
    MaterialHandle createMaterial(std::string id, MaterialAsset material);
    [[nodiscard]] MaterialHandle findMaterial(std::string_view id) const noexcept;
    [[nodiscard]] std::optional<std::string> materialId(MaterialHandle handle) const;
    // 销毁匹配代际的材质并回收槽位；句柄失效时返回 false。
    bool destroyMaterial(MaterialHandle handle);

    // 查询就绪材质；无效或过期句柄返回 nullptr。
    [[nodiscard]]
    const MaterialAsset* tryGetMaterial(MaterialHandle handle) const noexcept;

    // 同时返回材质和修订号，为后续后端缓存保留接口。
    [[nodiscard]]
    std::optional<MaterialAssetView> tryGetMaterialView(MaterialHandle handle) const noexcept;

    TextureHandle createTexture(TextureAsset texture);
    TextureHandle createTexture(std::string id, TextureAsset texture);
    bool destroyTexture(TextureHandle handle);
    [[nodiscard]] const TextureAsset* tryGetTexture(TextureHandle handle) const noexcept;
    [[nodiscard]] std::optional<TextureAssetView>
    tryGetTextureView(TextureHandle handle) const noexcept;
    [[nodiscard]] TextureHandle findTexture(std::string_view id) const noexcept;
    [[nodiscard]] std::optional<std::string> textureId(TextureHandle handle) const;

private:
    // 下标即句柄 index；零号槽位永远不分配。
    std::vector<AssetSlot<MeshAsset>> meshes_;
    // 可复用槽位索引的后进先出空闲表。
    std::vector<std::uint32_t> freeMeshSlots_;

    // 材质槽位与可复用索引表采用和网格相同的代际管理规则。
    std::vector<AssetSlot<MaterialAsset>> materials_;
    std::vector<std::uint32_t> freeMaterialSlots_;

    std::vector<AssetSlot<TextureAsset>> textures_;
    std::vector<std::uint32_t> freeTextureSlots_;

    std::unordered_map<std::string, MeshHandle> meshesById_;
    std::unordered_map<std::uint64_t, std::string> meshIdsByHandle_;
    std::unordered_map<std::string, MaterialHandle> materialsById_;
    std::unordered_map<std::uint64_t, std::string> materialIdsByHandle_;
    std::unordered_map<std::string, TextureHandle> texturesById_;
    std::unordered_map<std::uint64_t, std::string> textureIdsByHandle_;
};

} // namespace renderlab
