#pragma once

#include "renderer/RenderFrame.h"

namespace renderlab {

class IRenderBackend {
public:
    virtual ~IRenderBackend() = default;

    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    virtual void resize(int width, int height) = 0;
    virtual void render(const RenderFrame& frame) = 0;
};

} // namespace renderlab
