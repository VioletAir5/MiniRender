#pragma once

#include <cstdint>

namespace renderlab {

struct RenderFrame;

// 一次 Pass 执行所需的 API 无关帧信息。具体图形后端服务由 Pass 实现自行注入。
struct RenderPassExecutionContext {
    const RenderFrame& frame;
    std::uint64_t frameNumber{0};
    int viewportWidth{0};
    int viewportHeight{0};
};

// Render Pipeline 统一管理的最小 Pass 生命周期；接口不暴露 OpenGL/Vulkan 类型。
class IRenderPass {
  public:
    virtual ~IRenderPass() = default;

    // 创建 Pass 私有的 GPU 资源。失败时由 Pipeline 按 required 策略处理。
    [[nodiscard]] virtual bool initialize() {
        return true;
    }
    // 在所属图形上下文仍有效时释放资源。
    virtual void shutdown() noexcept {}
    // 通知 Pass 输出尺寸变化；无尺寸资源的 Pass 可以忽略。
    virtual void resize(int width, int height) {
        (void)width;
        (void)height;
    }
    // 执行当前帧。调用顺序由 RenderPipelineDescriptor 决定。
    virtual void execute(const RenderPassExecutionContext& context) = 0;
};

} // namespace renderlab
