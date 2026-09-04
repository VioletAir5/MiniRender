#pragma once

namespace renderlab {

struct OpenGLPassContext;
class RenderPassFactory;

// 注册当前 OpenGL 后端对稳定 Pass type ID 的实现。
// 增加 OpenGL Pass 时只需扩展这个组合根，不需要修改 OpenGLBackend。
[[nodiscard]] bool registerBuiltInOpenGLPasses(RenderPassFactory& factory,
                                               OpenGLPassContext& context);

} // namespace renderlab
