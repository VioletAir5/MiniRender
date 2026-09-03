#pragma once

#include "renderer/passes/ForwardPass.h"
#include "renderer/passes/GridPass.h"
#include "renderer/passes/OutlinePass.h"

#include <list>

namespace renderlab {

class IRenderBackend;
class IRenderPass;
struct RenderFrame;

// 拥有默认 Pass，并用非拥有链表表达可调整的执行顺序。
class RenderPipeline {
public:
    RenderPipeline();
    RenderPipeline(const RenderPipeline&) = delete;
    RenderPipeline& operator=(const RenderPipeline&) = delete;
    RenderPipeline(RenderPipeline&&) = delete;
    RenderPipeline& operator=(RenderPipeline&&) = delete;

    void addRenderPassToFront(IRenderPass* pass);
    void addRenderPassToBack(IRenderPass* pass);
    void removeRenderPass(IRenderPass* pass);

    void render(const RenderFrame& frame, IRenderBackend& backend);

private:
    ForwardPass forwardPass_;
    OutlinePass outlinePass_;
    GridPass gridPass_;
    std::list<IRenderPass*> passes_;
};

} // namespace renderlab
