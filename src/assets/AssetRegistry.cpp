#include "assets/AssetRegistry.h"

#include <utility>

namespace renderlab {
namespace {

template<typename Tag>
std::uint64_t handleKey(const AssetHandle<Tag> handle) noexcept {
    return (static_cast<std::uint64_t>(handle.generation) << 32U) | handle.index;
}

std::uint32_t channelCount(TextureFormat format) noexcept {
    switch (format) {
    case TextureFormat::R8:
        return 1U;
    case TextureFormat::RG8:
        return 2U;
    case TextureFormat::RGB8:
        return 3U;
    case TextureFormat::RGBA8:
        return 4U;
    default:
        return 0U;
    }
}

bool vallidateTextureData(TextureAsset const& texture) noexcept {
    if (texture.width == 0 || texture.height == 0) {
        return false;
    }
    const std::uint32_t channels = channelCount(texture.format);
    if (channels == 0U) {
        return false;
    }
    const std::size_t expectedSize = static_cast<std::size_t>(texture.width) *
                                     static_cast<std::size_t>(texture.height) *
                                     static_cast<std::size_t>(channels);
    return texture.pixels.size() == expectedSize;
}
} // namespace

AssetRegistry::AssetRegistry() {
    // 零号槽位永不分配，使默认构造的句柄可以稳定表示“无资产”。
    meshes_.reserve(MaxMeshAssets + 1U);
    meshes_.emplace_back();

    // 材质同样保留零号槽位，使默认句柄可以表示空材质。
    materials_.reserve(MaxMaterialAssets + 1U);
    materials_.emplace_back();

    textures_.reserve(MaxTextureAssets + 1U);
    textures_.emplace_back();
}

MeshHandle AssetRegistry::createMesh(MeshAsset mesh) {
    return createMesh({}, std::move(mesh));
}

MeshHandle AssetRegistry::createMesh(std::string id, MeshAsset mesh) {
    if (!id.empty()) {
        const auto existing = meshesById_.find(id);
        if (existing != meshesById_.end() && tryGetMesh(existing->second) != nullptr) {
            return existing->second;
        }
    }
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

    const MeshHandle handle{index, slot.generation};
    if (!id.empty()) {
        meshIdsByHandle_[handleKey(handle)] = id;
        meshesById_[std::move(id)] = handle;
    }
    return handle;
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
    if (const auto id = meshIdsByHandle_.find(handleKey(handle)); id != meshIdsByHandle_.end()) {
        meshesById_.erase(id->second);
        meshIdsByHandle_.erase(id);
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

MeshHandle AssetRegistry::findMesh(const std::string_view id) const noexcept {
    const auto iterator = meshesById_.find(std::string{id});
    return iterator == meshesById_.end() || tryGetMesh(iterator->second) == nullptr
               ? MeshHandle{} : iterator->second;
}

std::optional<std::string> AssetRegistry::meshId(const MeshHandle handle) const {
    const auto iterator = meshIdsByHandle_.find(handleKey(handle));
    return iterator == meshIdsByHandle_.end() ? std::nullopt
                                              : std::optional<std::string>{iterator->second};
}

MaterialHandle AssetRegistry::createMaterial(MaterialAsset material) {
    return createMaterial({}, std::move(material));
}

MaterialHandle AssetRegistry::createMaterial(std::string id, MaterialAsset material) {
    if (!id.empty()) {
        const auto existing = materialsById_.find(id);
        if (existing != materialsById_.end() && tryGetMaterial(existing->second) != nullptr) {
            return existing->second;
        }
    }
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

    const MaterialHandle handle{index, slot.generation};
    if (!id.empty()) {
        materialIdsByHandle_[handleKey(handle)] = id;
        materialsById_[std::move(id)] = handle;
    }
    return handle;
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
    if (const auto id = materialIdsByHandle_.find(handleKey(handle));
        id != materialIdsByHandle_.end()) {
        materialsById_.erase(id->second);
        materialIdsByHandle_.erase(id);
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

MaterialHandle AssetRegistry::findMaterial(const std::string_view id) const noexcept {
    const auto iterator = materialsById_.find(std::string{id});
    return iterator == materialsById_.end() || tryGetMaterial(iterator->second) == nullptr
               ? MaterialHandle{} : iterator->second;
}

std::optional<std::string> AssetRegistry::materialId(const MaterialHandle handle) const {
    if (!handle.valid()) {
        return std::nullopt;
    }
    const auto iterator = materialIdsByHandle_.find(handleKey(handle));
    return iterator == materialIdsByHandle_.end()
               ? std::nullopt : std::optional<std::string>{iterator->second};
}


TextureHandle AssetRegistry::createTexture(TextureAsset texture) {
    return createTexture({}, std::move(texture));
}

TextureHandle AssetRegistry::createTexture(std::string id, TextureAsset texture) {
    if (!id.empty()) {
        const auto existing = texturesById_.find(id);
        if (existing != texturesById_.end() && tryGetTexture(existing->second) != nullptr) {
            return existing->second;
        }
    }

    std::uint32_t index = 0;
    if (!freeTextureSlots_.empty()) {
        index = freeTextureSlots_.back();
        freeTextureSlots_.pop_back();
    } else {
        if (textures_.size() - 1U >= MaxTextureAssets) {
            return {};
        }
        index = static_cast<std::uint32_t>(textures_.size());
        textures_.emplace_back();
    }
    auto& slot = textures_[index];
    slot.asset = std::make_unique<TextureAsset>(std::move(texture));
    slot.state = AssetState::Ready;
    ++slot.revision;

    const TextureHandle handle{index, slot.generation};
    if (!id.empty()) {
        textureIdsByHandle_[handleKey(handle)] = id;
        texturesById_[std::move(id)] = handle;
    }
    return handle;
}

bool AssetRegistry::destroyTexture(const TextureHandle handle) {
    if (!handle.valid() || handle.index >= textures_.size()) {
        return false;
    }
    const std::uint32_t index = handle.index;
    auto& slot = textures_[index];
    if (slot.state == AssetState::Empty || slot.generation != handle.generation) {
        return false;
    }
    if (const auto id = textureIdsByHandle_.find(handleKey(handle));
        id != textureIdsByHandle_.end()) {
        texturesById_.erase(id->second);
        textureIdsByHandle_.erase(id);
    }
    slot.asset.reset();
    slot.state = AssetState::Empty;
    slot.revision = 0;
    ++slot.generation;
    if (slot.generation == 0) {
        ++slot.generation;
    }
    freeTextureSlots_.push_back(index);
    return true;
}

const TextureAsset* AssetRegistry::tryGetTexture(TextureHandle handle) const noexcept {
    const auto view = tryGetTextureView(handle);
    return view.has_value() ? view->asset : nullptr;
}

std::optional<TextureAssetView>
AssetRegistry::tryGetTextureView(TextureHandle handle) const noexcept {
    if (!handle.valid() || handle.index >= textures_.size()) {
        return std::nullopt;
    }
    const auto& slot = textures_[handle.index];
    if (slot.generation != handle.generation || slot.state != AssetState::Ready) {
        return std::nullopt;
    }
    return TextureAssetView{slot.asset.get(), slot.revision};
}

TextureHandle AssetRegistry::findTexture(const std::string_view id) const noexcept {
    const auto iterator = texturesById_.find(std::string{id});
    return iterator == texturesById_.end() || tryGetTexture(iterator->second) == nullptr
               ? TextureHandle{} : iterator->second;
}

std::optional<std::string> AssetRegistry::textureId(const TextureHandle handle) const {
    if (!handle.valid()) {
        return std::nullopt;
    }
    const auto iterator = textureIdsByHandle_.find(handleKey(handle));
    return iterator == textureIdsByHandle_.end()
               ? std::nullopt : std::optional<std::string>{iterator->second};
}
} // namespace renderlab
