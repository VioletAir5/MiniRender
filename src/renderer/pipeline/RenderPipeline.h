#pragma once

#include "renderer/pipeline/IRenderPass.h"
#include "renderer/pipeline/RenderGraph.h"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace renderlab {

// 一个 Pipeline 实例中的 Pass 配置。type 由具体后端工厂解析。
struct RenderPassDescriptor {
    std::string name;
    std::string type;
    bool enabled{true};
    // 可选 Pass 初始化失败时只禁用自身；必需 Pass 失败会使整个 Pipeline 初始化失败。
    bool required{true};
    std::vector<std::string> dependsOn;
    std::vector<std::string> reads;
    std::vector<std::string> writes;
};

// API 无关的有序 Pass 配置；以后可直接由项目资产或编辑器配置生成。
struct RenderPipelineDescriptor {
    std::vector<RenderResourceDescriptor> resources;
    std::vector<RenderPassDescriptor> passes;
};

// 后端通过注册稳定 type ID，把 API 无关描述转换为自己的 Pass 实现。
class RenderPassFactory final {
  public:
    using Creator = std::function<std::unique_ptr<IRenderPass>()>;

    [[nodiscard]] bool registerType(std::string type, Creator creator);
    [[nodiscard]] std::unique_ptr<IRenderPass> create(std::string_view type) const;

  private:
    std::unordered_map<std::string, Creator> creators_;
};

// 统一拥有 Pass，并管理初始化、尺寸变化、执行和逆序释放。
class RenderPipeline final {
  public:
    RenderPipeline() = default;
    RenderPipeline(const RenderPipeline&) = delete;
    RenderPipeline& operator=(const RenderPipeline&) = delete;

    // 根据描述创建全部 Pass；仅构造对象，不访问图形 API。
    [[nodiscard]] bool build(const RenderPipelineDescriptor& descriptor,
                             const RenderPassFactory& factory, std::string& error);
    // 初始化已启用 Pass；必需 Pass 失败时回滚此前成功初始化的 Pass。
    [[nodiscard]] bool initialize(std::string& error);
    void shutdown() noexcept;
    void resize(int width, int height);
    void execute(const RenderPassExecutionContext& context);

    [[nodiscard]] bool setEnabled(std::string_view name, bool enabled);
    [[nodiscard]] bool contains(std::string_view name) const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;
    [[nodiscard]] bool initialized() const noexcept;

  private:
    struct Entry {
        RenderPassDescriptor descriptor;
        std::unique_ptr<IRenderPass> pass;
        bool active{false};
    };

    std::vector<Entry> entries_;
    bool initialized_{false};
    int width_{0};
    int height_{0};
};

} // namespace renderlab
