#pragma once

#include "assets/AssetHandle.h"
#include "assets/MeshAsset.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace renderlab {

enum class AssetState {
    Empty,
    Loading,
    Ready,
    Failed,
};

template<typename T>
struct AssetSlot {
    std::uint32_t generation{1};
    std::uint32_t revision{0};
    AssetState state{AssetState::Empty};
    std::unique_ptr<T> asset;
};

class AssetRegistry {
public:
    MeshHandle createMesh(MeshAsset mesh);
    bool destroyMesh(MeshHandle handle);

    [[nodiscard]]
    const MeshAsset* tryGetMesh(MeshHandle handle) const noexcept;

private:
    std::vector<AssetSlot<MeshAsset>> meshes_;
    std::vector<std::uint32_t> freeMeshSlots_;
};

} // namespace renderlab
