#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

namespace renderlab {

struct OpenGLPassContext;
struct RenderFrame;
struct RenderItem;

// 解析一个 RenderItem 的网格与材质并提交所有 primitive。
// overrideColor 用于轮廓等不应继承原材质表面参数的绘制。
void drawOpenGLSceneItem(OpenGLPassContext& context, const RenderFrame& frame,
                         const RenderItem& item, const glm::mat4& model,
                         const glm::vec4* overrideColor = nullptr);

} // namespace renderlab
