#pragma once

#include "renderer/backends/opengl/OpenGLGridRenderer.h"

namespace renderlab {

struct OpenGLPassContext;
struct RenderFrame;

// 在场景和轮廓之后绘制编辑器网格，并隔离覆盖层所需的 OpenGL 状态。
class OpenGLGridPass final {
public:
    bool initialize();
    void shutdown() noexcept;
    void render(const RenderFrame& frame, OpenGLPassContext& context);

private:
    OpenGLGridRenderer renderer_;
};

} // namespace renderlab
