#pragma once

#include "renderer/RenderFrame.h"

class QWidget;

namespace renderlab {

class IRenderSurface {
public:
    virtual ~IRenderSurface() = default;

    [[nodiscard]] virtual QWidget& widget() noexcept = 0;
    virtual void setFrame(RenderFrame frame) = 0;
    virtual void requestRender() = 0;
};

} // namespace renderlab
