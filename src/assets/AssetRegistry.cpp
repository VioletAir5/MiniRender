#include "assets/AssetRegistry.h"

#include <utility>

namespace renderlab {

AssetRegistry::AssetRegistry() {
    // 零号槽位永不分配，使默认构造的句柄可以稳定表示“无资产”。
    meshes_.reserve(MaxMeshAssets + 1U);
    meshes_.emplace_back();

    // 材质同样保留零号槽位，使默认句柄可以表示空材质。
    materials_.reserve(MaxMaterialAssets + 1U);
    materials_.emplace_back();
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

MaterialHandle AssetRegistry::createMaterial(MaterialAsset material) {
    std::uint32_t index = 0;

    if (!freeMaterialSlots_.empty()) {
        // 优先复用已销毁资产留下的槽位，避免注册表持续增长。
        index = freeMaterialSlots_.back();
        freeMaterialSlots_.pop_back();
    } else {
        if (materials_.size() - 1U >= MaxMaterialAssets) {
            return {};
        }

        index = static_cast<std::uint32_t>(materials_.size());
        materials_.emplace_back();
    }

    auto& slot = materials_[index];
    slot.asset = std::make_unique<MaterialAsset>(std::move(material));
    slot.state = AssetState::Ready;
    ++slot.revision;

    return {index, slot.generation};
}

bool AssetRegistry::destroyMaterial(const MaterialHandle handle) {
    if (!handle.valid() || handle.index >= materials_.size()) {
        return false;
    }

    const std::uint32_t index = handle.index;
    auto& slot = materials_[index];

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

    freeMaterialSlots_.push_back(index);
    return true;
}

const MaterialAsset* AssetRegistry::tryGetMaterial(
    const MaterialHandle handle) const noexcept {
    const auto view = tryGetMaterialView(handle);
    return view.has_value() ? view->asset : nullptr;
}

std::optional<MaterialAssetView> AssetRegistry::tryGetMaterialView(
    const MaterialHandle handle) const noexcept {
    if (!handle.valid() || handle.index >= materials_.size()) {
        return std::nullopt;
    }

    const auto& slot = materials_[handle.index];
    if (slot.generation != handle.generation ||
        slot.state != AssetState::Ready) {
        return std::nullopt;
    }

    return MaterialAssetView{slot.asset.get(), slot.revision};
}

} // namespace renderlab
