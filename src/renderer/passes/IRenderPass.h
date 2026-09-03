#pragma once

namespace renderlab {

struct RenderPassContext;

// API 无关的渲染阶段；只表达渲染策略，不直接调用原生图形 API。
class IRenderPass {
public:
    virtual ~IRenderPass() = default;
    virtual void execute(RenderPassContext& context) = 0;
};

} // namespace renderlab
