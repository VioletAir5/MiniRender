#pragma once

#include <compare>
#include <cstdint>

namespace renderlab {

template<typename Tag>
struct AssetHandle {
    std::uint32_t index{0};
    std::uint32_t generation{0};

    [[nodiscard]] bool valid() const noexcept {
        return index != 0 && generation != 0;
    }

    auto operator<=>(const AssetHandle&) const = default;
};

struct MeshAssetTag;
struct MaterialAssetTag;

using MeshHandle = AssetHandle<MeshAssetTag>;
using MaterialHandle = AssetHandle<MaterialAssetTag>;

} // namespace renderlab
