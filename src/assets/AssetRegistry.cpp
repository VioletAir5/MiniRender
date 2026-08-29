#include "assets/AssetRegistry.h"

#include <utility>

namespace renderlab {

AssetRegistry::AssetRegistry() {
    // 零号槽位永不分配，使默认构造的句柄可以稳定表示“无资产”。
    meshes_.reserve(MaxMeshAssets + 1U);
    meshes_.emplace_back();
}

MeshHandle AssetRegistry::createMesh(MeshAsset mesh) {
    std::uint32_t index = 0;

    if (!freeMeshSlots_.empty()) {
        // 优先复用已销毁资产留下的槽位，避免注册表持续增长。
        index = freeMeshSlots_.back();
        freeMeshSlots_.pop_back();
    } else {
        if (meshes_.size() - 1U >= MaxMeshAssets) {
            return {};
        }

        index = static_cast<std::uint32_t>(meshes_.size());
        meshes_.emplace_back();
    }

    auto& slot = meshes_[index];
    slot.asset = std::make_unique<MeshAsset>(std::move(mesh));
    slot.state = AssetState::Ready;
    ++slot.revision;

    return {index, slot.generation};
}

bool AssetRegistry::destroyMesh(const MeshHandle handle) {
    if (!handle.valid() || handle.index >= meshes_.size()) {
        return false;
    }

    const std::uint32_t index = handle.index;
    auto& slot = meshes_[index];

    if (slot.state == AssetState::Empty ||
        slot.generation != handle.generation) {
        return false;
    }

    slot.asset.reset();
    slot.state = AssetState::Empty;
    slot.revision = 0;

    // 递增代际可让指向旧资产的句柄立即失效；零值始终留给空句柄。
    ++slot.generation;
    if (slot.generation == 0) {
        ++slot.generation;
    }

    freeMeshSlots_.push_back(index);
    return true;
}

const MeshAsset* AssetRegistry::tryGetMesh(
    const MeshHandle handle) const noexcept {
    const auto view = tryGetMeshView(handle);
    return view.has_value() ? view->asset : nullptr;
}

std::optional<MeshAssetView> AssetRegistry::tryGetMeshView(
    const MeshHandle handle) const noexcept {
    if (!handle.valid() || handle.index >= meshes_.size()) {
        return std::nullopt;
    }

    const auto& slot = meshes_[handle.index];
    if (slot.generation != handle.generation ||
        slot.state != AssetState::Ready) {
        return std::nullopt;
    }

    return MeshAssetView{slot.asset.get(), slot.revision};
}

} // namespace renderlab