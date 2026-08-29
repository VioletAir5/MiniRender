#include "renderer/backends/opengl/OpenGLMeshCache.h"

#include "assets/AssetRegistry.h"

#include <limits>
#include <utility>

namespace renderlab {
namespace {

// 超过该帧数未使用的 GPU 网格可被主动回收。
constexpr std::uint64_t UnusedFrameLimit = 300;
// 表示垃圾回收时没有必须保留的槽位。
constexpr std::uint32_t NoProtectedIndex =
    std::numeric_limits<std::uint32_t>::max();

// 估算顶点与索引缓冲大小，不含驱动和 VAO 的额外开销。
std::size_t primitiveGpuBytes(const MeshPrimitive& primitive) {
    return primitive.vertices.size() * sizeof(Vertex) +
           primitive.indices.size() * sizeof(std::uint32_t);
}

} // namespace

OpenGLMeshCache::OpenGLMeshCache(
    const AssetRegistry& registry,
    const std::size_t memoryBudget) noexcept
    : registry_(registry), memoryBudget_(memoryBudget) {}

OpenGLMeshCache::~OpenGLMeshCache() = default;

const CachedOpenGLMesh* OpenGLMeshCache::resolve(
    const MeshHandle handle,
    const std::uint64_t frameNumber) {
    // 先校验代际并取得 revision，避免上传已被槽位复用的旧资产。
    const auto source = registry_.tryGetMeshView(handle);
    if (!source.has_value() || source->asset == nullptr) {
        if (handle.index < entries_.size()) {
            auto& staleEntry = entries_[handle.index];
            if (staleEntry.has_value() &&
                staleEntry->generation == handle.generation) {
                currentGpuBytes_ -= staleEntry->gpuBytes;
                staleEntry.reset();
            }
        }
        return nullptr;
    }

    if (entries_.size() <= handle.index) {
        entries_.resize(static_cast<std::size_t>(handle.index) + 1U);
    }

    auto& entry = entries_[handle.index];
    if (entry.has_value() &&
        entry->generation == handle.generation &&
        entry->revision == source->revision) {
        entry->lastUsedFrame = frameNumber;
        return &*entry;
    }

    // 先在临时对象中完成全部上传，成功后再替换旧缓存。
    CachedOpenGLMesh uploaded;
    if (!upload(uploaded, handle, *source->asset, source->revision)) {
        return nullptr;
    }
    uploaded.lastUsedFrame = frameNumber;

    if (entry.has_value()) {
        currentGpuBytes_ -= entry->gpuBytes;
    }
    currentGpuBytes_ += uploaded.gpuBytes;
    entry.emplace(std::move(uploaded));

    evictUntilWithinBudget(frameNumber, handle.index);
    return entry.has_value() ? &*entry : nullptr;
}

void OpenGLMeshCache::collectGarbage(const std::uint64_t frameNumber) {
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

void OpenGLMeshCache::clear() noexcept {
    entries_.clear();
    currentGpuBytes_ = 0;
}

std::size_t OpenGLMeshCache::currentGpuBytes() const noexcept {
    return currentGpuBytes_;
}

std::size_t OpenGLMeshCache::memoryBudget() const noexcept {
    return memoryBudget_;
}

bool OpenGLMeshCache::upload(CachedOpenGLMesh& destination,
                             const MeshHandle handle,
                             const MeshAsset& source,
                             const std::uint32_t revision) {
    if (source.primitives.empty()) {
        return false;
    }

    destination.generation = handle.generation;
    destination.revision = revision;
    destination.primitives.reserve(source.primitives.size());

    for (const MeshPrimitive& primitive : source.primitives) {
        CachedOpenGLPrimitive cached;
        // 失败返回时 destination 析构会回收此前已成功上传的 primitive。
        if (!cached.mesh.upload(primitive)) {
            return false;
        }

        cached.defaultMaterial = primitive.defaultMaterial;
        cached.gpuBytes = primitiveGpuBytes(primitive);
        destination.gpuBytes += cached.gpuBytes;
        destination.primitives.push_back(std::move(cached));
    }

    return true;
}

void OpenGLMeshCache::evictUntilWithinBudget(
    const std::uint64_t frameNumber,
    const std::uint32_t protectedIndex) {
    while (currentGpuBytes_ > memoryBudget_) {
        std::size_t oldestIndex = entries_.size();
        std::uint64_t oldestFrame = std::numeric_limits<std::uint64_t>::max();

        for (std::size_t index = 0; index < entries_.size(); ++index) {
            const auto& entry = entries_[index];
            if (!entry.has_value() || index == protectedIndex ||
                entry->lastUsedFrame == frameNumber) {
                continue;
            }

            if (entry->lastUsedFrame < oldestFrame) {
                oldestFrame = entry->lastUsedFrame;
                oldestIndex = index;
            }
        }

        if (oldestIndex == entries_.size()) {
            // 当前帧可见工作集优先于软预算，因此允许暂时超限。
            return;
        }

        currentGpuBytes_ -= entries_[oldestIndex]->gpuBytes;
        entries_[oldestIndex].reset();
    }
}

} // namespace renderlab
