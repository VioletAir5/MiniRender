#pragma once

namespace renderlab {

struct OpenGLPassContext;
struct RenderFrame;
struct RenderItem;

// 使用 Forward Pass 写入的模板值绘制选中实体外扩轮廓。
class OpenGLOutlinePass final {
public:
    void render(const RenderFrame& frame,
                const RenderItem* outlinedItem,
                OpenGLPassContext& context) const;
};

} // namespace renderlab
