#pragma once

#include "renderer/RenderFrame.h"

namespace renderlab {

class SceneDocument;

class SceneRenderer {
public:
    [[nodiscard]] RenderFrame buildFrame(const SceneDocument& scene,
                                         int viewportWidth,
                                         int viewportHeight) const;
};

} // namespace renderlab
