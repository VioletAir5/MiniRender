#pragma once

#include <compare>
#include <cstdint>

namespace renderlab {

// 带代际编号的类型安全资产句柄，可检测槽位复用后遗留的旧引用。
template<typename Tag>
struct AssetHandle {
    std::uint32_t index{0};
    std::uint32_t generation{0};

    // 格式有效不等于资产存在，后者仍需由 AssetRegistry 确认。
    [[nodiscard]] bool valid() const noexcept {
        return index != 0 && generation != 0;
    }

    // 句柄按槽位索引和代际编号比较。
    auto operator<=>(const AssetHandle&) const = default;
};

// 标签类型只用于在编译期区分不同种类的资产句柄。
struct MeshAssetTag;
struct MaterialAssetTag;
struct TextureAssetTag;
struct ShaderAssetTag;

// 分别引用网格、材质、纹理和 Shader 资产的强类型句柄。
using MeshHandle = AssetHandle<MeshAssetTag>;
using MaterialHandle = AssetHandle<MaterialAssetTag>;
using TextureHandle = AssetHandle<TextureAssetTag>;
using ShaderHandle = AssetHandle<ShaderAssetTag>;

} // namespace renderlab
