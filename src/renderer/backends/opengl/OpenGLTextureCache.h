#pragma once

#include "assets/AssetHandle.h"
#include "renderer/backends/opengl/OpenGLTexture.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace renderlab {

class AssetRegistry;

inline constexpr std::size_t DefaultTextureGpuBudget =
    256ULL * 1024ULL * 1024ULL;

// 一份 TextureAsset 对应的 GPU 纹理及缓存生命周期信息。
struct CachedOpenGLTexture {
    OpenGLTexture texture;
    std::uint32_t generation{0};
    std::uint32_t revision{0};
    std::uint64_t lastUsedFrame{0};
    std::size_t gpuBytes{0};
};

// 将 TextureAsset 延迟上传为 OpenGLTexture，并按最近使用时间回收。
class OpenGLTextureCache final {
public:
    explicit OpenGLTextureCache(
        const AssetRegistry& registry,
        std::size_t memoryBudget = DefaultTextureGpuBudget) noexcept;
    ~OpenGLTextureCache();

    OpenGLTextureCache(const OpenGLTextureCache&) = delete;
    OpenGLTextureCache& operator=(const OpenGLTextureCache&) = delete;

    [[nodiscard]] const OpenGLTexture*
    resolve(TextureHandle handle, std::uint64_t frameNumber);
    void collectGarbage(std::uint64_t frameNumber);
    void clear() noexcept;

    [[nodiscard]] std::size_t currentGpuBytes() const noexcept;
    [[nodiscard]] std::size_t memoryBudget() const noexcept;

private:
    void evictUntilWithinBudget(std::uint64_t frameNumber,
                                std::uint32_t protectedIndex);

    const AssetRegistry& registry_;
    std::vector<std::optional<CachedOpenGLTexture>> entries_;
    std::size_t currentGpuBytes_{0};
    std::size_t memoryBudget_{DefaultTextureGpuBudget};
};

} // namespace renderlab
