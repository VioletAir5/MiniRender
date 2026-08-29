#pragma once

#include "assets/AssetHandle.h"
#include "renderer/backends/opengl/OpenGLMesh.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <vector>

namespace renderlab {

class AssetRegistry;

// GPU 网格缓存的默认软上限；当前帧正在使用的网格不会为满足预算而被驱逐。
inline constexpr std::size_t DefaultMeshGpuBudget =
    256ULL * 1024ULL * 1024ULL;

// 单个 CPU Primitive 对应的 GPU 网格、默认材质及估算显存占用。
struct CachedOpenGLPrimitive {
    OpenGLMesh mesh;
    MaterialHandle defaultMaterial;
    std::size_t gpuBytes{0};
};

// 一个 MeshAsset 的缓存记录，代际和修订号用于检测槽位复用与内容变化。
struct CachedOpenGLMesh {
    std::uint32_t generation{0};
    std::uint32_t revision{0};

    std::vector<CachedOpenGLPrimitive> primitives;

    std::uint64_t lastUsedFrame{0};
    std::size_t gpuBytes{0};
};

// 将 API 无关 MeshAsset 延迟上传为 OpenGLMesh，并按最近使用时间回收。
// 所有方法及析构都必须在同一有效 OpenGL 上下文中执行。
class OpenGLMeshCache {
public:
    // registry 必须比缓存存活更久；memoryBudget 是可暂时超出的软预算。
    explicit OpenGLMeshCache(
        const AssetRegistry& registry,
        std::size_t memoryBudget = DefaultMeshGpuBudget) noexcept;
    ~OpenGLMeshCache();

    OpenGLMeshCache(const OpenGLMeshCache&) = delete;
    OpenGLMeshCache& operator=(const OpenGLMeshCache&) = delete;

    // 返回本帧可绘制缓存；缺失或上传失败时返回 nullptr。
    // 返回指针在下一次修改缓存的操作前有效。
    [[nodiscard]] const CachedOpenGLMesh*
    resolve(MeshHandle handle, std::uint64_t frameNumber);

    // 回收长期未使用资源，并使用 LRU 策略尽量满足显存预算。
    void collectGarbage(std::uint64_t frameNumber);
    // 释放全部 GPU 网格并清零显存统计。
    void clear() noexcept;

    // 返回估算的当前占用和配置的软预算。
    [[nodiscard]] std::size_t currentGpuBytes() const noexcept;
    [[nodiscard]] std::size_t memoryBudget() const noexcept;

private:
    // 将一个 CPU MeshAsset 的全部 primitive 原子式构造成缓存记录。
    [[nodiscard]] bool upload(CachedOpenGLMesh& destination,
                              MeshHandle handle,
                              const MeshAsset& source,
                              std::uint32_t revision);
    // 驱逐最久未使用记录，但不会驱逐 protectedIndex 指向的当前网格。
    void evictUntilWithinBudget(std::uint64_t frameNumber,
                                std::uint32_t protectedIndex);

    const AssetRegistry& registry_;
    std::vector<std::optional<CachedOpenGLMesh>> entries_;
    std::size_t currentGpuBytes_{0};
    std::size_t memoryBudget_{DefaultMeshGpuBudget};
};

} // namespace renderlab
