#pragma once

#include "renderer/RenderFrame.h"
#include "renderer/RenderView.h"

namespace renderlab {

class SceneDocument;

class SceneRenderer {
public:
    [[nodiscard]] RenderFrame buildFrame(const SceneDocument& scene,
                                         int viewportWidth,
                                         int viewportHeight) const;
    [[nodiscard]] RenderFrame buildFrame(const SceneDocument& scene,
                                         const RenderView& view) const;
};

} // namespace renderlab
