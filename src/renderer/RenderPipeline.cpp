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

IRenderPass* RenderPipeline::findRenderPass(const std::string_view name) noexcept {
    for (IRenderPass* pass : passes_) {
        if (pass != nullptr && pass->name() == name) {
            return pass;
        }
    }
    return nullptr;
}

const IRenderPass* RenderPipeline::findRenderPass(
    const std::string_view name) const noexcept {
    for (const IRenderPass* pass : passes_) {
        if (pass != nullptr && pass->name() == name) {
            return pass;
        }
    }
    return nullptr;
}

bool RenderPipeline::setRenderPassEnabled(
    const std::string_view name, const bool enabled) noexcept {
    IRenderPass* pass = findRenderPass(name);
    if (pass == nullptr) {
        return false;
    }
    pass->setEnabled(enabled);
    return true;
}

void RenderPipeline::render(const RenderFrame& frame, IRenderBackend& backend) {
    backend.beginFrame(frame);

    if (frame.hasCamera) {
        RenderPassContext context{
            .frame = frame,
            .commands = backend.commandList(),
        };
        for (IRenderPass* pass : passes_) {
            if (pass != nullptr && pass->isEnabled()) {
                pass->execute(context);
            }
        }
    }

    backend.endFrame();
}

} // namespace renderlab
