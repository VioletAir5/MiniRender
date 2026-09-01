#include "renderer/backends/opengl/OpenGLTextureCache.h"

#include "assets/AssetRegistry.h"

#include <limits>
#include <utility>

namespace renderlab {
namespace {

constexpr std::uint64_t UnusedFrameLimit = 300;
constexpr std::uint32_t NoProtectedIndex =
    std::numeric_limits<std::uint32_t>::max();

std::size_t channelCount(const TextureFormat format) noexcept {
    switch (format) {
    case TextureFormat::R8: return 1;
    case TextureFormat::RG8: return 2;
    case TextureFormat::RGB8: return 3;
    case TextureFormat::RGBA8: return 4;
    }
    return 0;
}

// 统计像素级显存；mipmap 使用 1/3 基础层大小近似。
std::size_t textureGpuBytes(const TextureAsset& texture) noexcept {
    const std::size_t base = static_cast<std::size_t>(texture.width) *
                             static_cast<std::size_t>(texture.height) *
                             channelCount(texture.format);
    return texture.sampler.generateMipmaps ? base + base / 3U : base;
}

} // namespace

OpenGLTextureCache::OpenGLTextureCache(
    const AssetRegistry& registry, const std::size_t memoryBudget) noexcept
    : registry_(registry), memoryBudget_(memoryBudget) {
}

OpenGLTextureCache::~OpenGLTextureCache() = default;

const OpenGLTexture* OpenGLTextureCache::resolve(
    const TextureHandle handle, const std::uint64_t frameNumber) {
    const auto source = registry_.tryGetTextureView(handle);
    if (!source.has_value() || source->asset == nullptr) {
        if (handle.index < entries_.size()) {
            auto& stale = entries_[handle.index];
            if (stale.has_value() && stale->generation == handle.generation) {
                currentGpuBytes_ -= stale->gpuBytes;
                stale.reset();
            }
        }
        return nullptr;
    }

    if (entries_.size() <= handle.index) {
        entries_.resize(static_cast<std::size_t>(handle.index) + 1U);
    }

    auto& entry = entries_[handle.index];
    if (entry.has_value() && entry->generation == handle.generation &&
        entry->revision == source->revision) {
        entry->lastUsedFrame = frameNumber;
        return &entry->texture;
    }

    CachedOpenGLTexture uploaded;
    if (!uploaded.texture.upload(*source->asset)) {
        return nullptr;
    }
    uploaded.generation = handle.generation;
    uploaded.revision = source->revision;
    uploaded.lastUsedFrame = frameNumber;
    uploaded.gpuBytes = textureGpuBytes(*source->asset);

    if (entry.has_value()) currentGpuBytes_ -= entry->gpuBytes;
    currentGpuBytes_ += uploaded.gpuBytes;
    entry.emplace(std::move(uploaded));

    evictUntilWithinBudget(frameNumber, handle.index);
    return entry.has_value() ? &entry->texture : nullptr;
}

void OpenGLTextureCache::collectGarbage(const std::uint64_t frameNumber) {
    for (auto& entry : entries_) {
        if (!entry.has_value() || frameNumber < entry->lastUsedFrame ||
            frameNumber - entry->lastUsedFrame < UnusedFrameLimit) {
            continue;
        }
        currentGpuBytes_ -= entry->gpuBytes;
        entry.reset();
    }
    evictUntilWithinBudget(frameNumber, NoProtectedIndex);
}

void OpenGLTextureCache::clear() noexcept {
    entries_.clear();
    currentGpuBytes_ = 0;
}

std::size_t OpenGLTextureCache::currentGpuBytes() const noexcept {
    return currentGpuBytes_;
}

std::size_t OpenGLTextureCache::memoryBudget() const noexcept {
    return memoryBudget_;
}

void OpenGLTextureCache::evictUntilWithinBudget(
    const std::uint64_t frameNumber, const std::uint32_t protectedIndex) {
    while (currentGpuBytes_ > memoryBudget_) {
        std::size_t oldestIndex = entries_.size();
        std::uint64_t oldestFrame = std::numeric_limits<std::uint64_t>::max();
        for (std::size_t index = 0; index < entries_.size(); ++index) {
            const auto& entry = entries_[index];
            if (!entry.has_value() || index == protectedIndex ||
                entry->lastUsedFrame == frameNumber) continue;
            if (entry->lastUsedFrame < oldestFrame) {
                oldestFrame = entry->lastUsedFrame;
                oldestIndex = index;
            }
        }
        if (oldestIndex == entries_.size()) return;
        currentGpuBytes_ -= entries_[oldestIndex]->gpuBytes;
        entries_[oldestIndex].reset();
    }
}

} // namespace renderlab
