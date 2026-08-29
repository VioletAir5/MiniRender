#pragma once

#include "renderer/RenderFrame.h"

namespace renderlab {

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
    // 消费一份 API 无关的帧快照并提交绘制命令。
    virtual void render(const RenderFrame& frame) = 0;
};

} // namespace renderlab
