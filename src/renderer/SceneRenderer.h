#pragma once

#include "renderer/RenderFrame.h"
#include "renderer/RenderView.h"

namespace renderlab {

class SceneDocument;

// 将 SceneDocument 提取成 RenderFrame，不直接调用任何图形 API。
class SceneRenderer {
public:
    // 使用场景中标记为 primary 的相机生成渲染帧；没有相机时返回无效视图。
    [[nodiscard]] RenderFrame buildFrame(const SceneDocument& scene,
                                         int viewportWidth,
                                         int viewportHeight) const;
    // 使用编辑器提供的视图生成渲染帧，并收集全部可见网格实体。
    [[nodiscard]] RenderFrame buildFrame(const SceneDocument& scene,
                                         const RenderView& view) const;
};

} // namespace renderlab
