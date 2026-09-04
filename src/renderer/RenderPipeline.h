#pragma once

#include "renderer/passes/ForwardPass.h"
#include "renderer/passes/GridPass.h"
#include "renderer/passes/OutlinePass.h"

#include <list>
#include <string_view>

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

    [[nodiscard]] IRenderPass* findRenderPass(std::string_view name) noexcept;
    [[nodiscard]] const IRenderPass* findRenderPass(
        std::string_view name) const noexcept;
    [[nodiscard]] bool setRenderPassEnabled(
        std::string_view name, bool enabled) noexcept;

    void render(const RenderFrame& frame, IRenderBackend& backend);

private:
    ForwardPass forwardPass_;
    OutlinePass outlinePass_;
    GridPass gridPass_;
    std::list<IRenderPass*> passes_;
};

} // namespace renderlab
