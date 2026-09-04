#pragma once

namespace renderlab {

class IRenderCommandList;
struct RenderFrame;

// 图形 API 后端的最小接口，使上层渲染链路不依赖具体实现。
class IRenderBackend {
public:
    virtual ~IRenderBackend() = default;

    // 在当前渲染上下文中创建后端资源，成功时返回 true。
    virtual bool initialize() = 0;
    // 在资源所属上下文仍有效时释放全部后端资源。
    virtual void shutdown() = 0;
    // 更新后端视口尺寸。
    virtual void resize(int width, int height) = 0;
    // 建立/结束一帧，并向 API 无关的 RenderPipeline 提供命令记录接口。
    virtual void beginFrame(const RenderFrame& frame) = 0;
    [[nodiscard]] virtual IRenderCommandList& commandList() = 0;
    virtual void endFrame() = 0;
};

} // namespace renderlab
