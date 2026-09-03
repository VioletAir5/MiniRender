#include "renderer/RenderPipeline.h"

#include "renderer/RenderFrame.h"
#include "renderer/passes/IRenderPass.h"
#include "renderer/passes/RenderPassContext.h"
#include "renderer/rhi/IRenderBackend.h"
#include "renderer/rhi/IRenderCommandList.h"

namespace renderlab {

RenderPipeline::RenderPipeline() {
    addRenderPassToBack(&forwardPass_);
    addRenderPassToBack(&outlinePass_);
    addRenderPassToBack(&gridPass_);
}

void RenderPipeline::addRenderPassToFront(IRenderPass* pass) {
    if (pass == nullptr) {
        return;
    }
    passes_.remove(pass);
    passes_.push_front(pass);
}

void RenderPipeline::addRenderPassToBack(IRenderPass* pass) {
    if (pass == nullptr) {
        return;
    }
    passes_.remove(pass);
    passes_.push_back(pass);
}

void RenderPipeline::removeRenderPass(IRenderPass* pass) {
    passes_.remove(pass);
}

void RenderPipeline::render(const RenderFrame& frame, IRenderBackend& backend) {
    backend.beginFrame(frame);

    if (frame.hasCamera) {
        RenderPassContext context{
            .frame = frame,
            .commands = backend.commandList(),
        };
        for (IRenderPass* pass : passes_) {
            pass->execute(context);
        }
    }

    backend.endFrame();
}

} // namespace renderlab
