#pragma once

#include <string>
#include <vector>

namespace renderlab {

enum class RenderResourceKind {
    Texture2D,
    Buffer,
};

enum class RenderResourceFormat {
    Unknown,
    Rgba8Unorm,
    Rgba16Float,
    R32UnsignedInteger,
    Depth24Stencil8,
    Depth32Float,
};

enum class RenderResourceSizeMode {
    Viewport,
    Fixed,
};

// 描述逻辑渲染资源，不包含 GLuint、VkImage 等底层 API 句柄。
struct RenderResourceDescriptor {
    std::string name;
    RenderResourceKind kind{RenderResourceKind::Texture2D};
    RenderResourceFormat format{RenderResourceFormat::Unknown};
    RenderResourceSizeMode sizeMode{RenderResourceSizeMode::Viewport};
    float widthScale{1.0F};
    float heightScale{1.0F};
    int width{0};
    int height{0};
    // 外部资源由 Surface/Backend 提供，例如交换链或 Qt 默认 FBO。
    bool external{false};
};

struct RenderGraphPassDescriptor {
    std::string name;
    std::vector<std::string> dependsOn;
    std::vector<std::string> reads;
    std::vector<std::string> writes;
};

struct RenderGraphDescriptor {
    std::vector<RenderResourceDescriptor> resources;
    std::vector<RenderGraphPassDescriptor> passes;
};

struct CompiledRenderGraph {
    std::vector<RenderResourceDescriptor> resources;
    // 拓扑排序后的 Pass；dependsOn 已同时包含显式依赖和资源生产者依赖。
    std::vector<RenderGraphPassDescriptor> passes;
};

// 对 API 无关的资源和依赖进行编译，不创建任何 GPU 对象。
class RenderGraphCompiler final {
  public:
    [[nodiscard]] bool compile(const RenderGraphDescriptor& descriptor, CompiledRenderGraph& result,
                               std::string& error) const;
};

} // namespace renderlab
