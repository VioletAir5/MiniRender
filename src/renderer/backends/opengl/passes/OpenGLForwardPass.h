#pragma once

namespace renderlab {

struct OpenGLPassContext;
struct RenderFrame;
struct RenderItem;

// 绘制场景表面，并在存在选择时把选中实体写入模板缓冲。
class OpenGLForwardPass final {
public:
    [[nodiscard]] const RenderItem*
    render(const RenderFrame& frame, OpenGLPassContext& context) const;
};

} // namespace renderlab
