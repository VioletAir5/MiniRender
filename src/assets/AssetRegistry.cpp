#include "assets/AssetRegistry.h"

namespace renderlab {

const MeshAsset* AssetRegistry::tryGetMesh(
    const MeshHandle handle) const noexcept {

    if (!handle.valid() || handle.index >= meshes_.size()) {
        return nullptr;
    }

    const auto& slot = meshes_[handle.index];

    if (slot.generation != handle.generation ||
        slot.state != AssetState::Ready) {
        return nullptr;
    }

    return slot.asset.get();
}

} // namespace renderlab
