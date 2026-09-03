#pragma once

namespace renderlab {

class IRenderCommandList;
struct RenderFrame;
struct RenderItem;

// Pass 共享的单帧上下文。所有指针和引用均为非拥有关系。
struct RenderPassContext {
    const RenderFrame& frame;
    IRenderCommandList& commands;
    const RenderItem* outlinedItem{nullptr};
};

} // namespace renderlab
