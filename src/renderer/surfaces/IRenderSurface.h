#pragma once

class QWidget;

namespace renderlab {

class SceneDocument;

class IRenderSurface {
public:
    virtual ~IRenderSurface() = default;

    [[nodiscard]] virtual QWidget& widget() noexcept = 0;
    virtual void setScene(const SceneDocument* scene) noexcept = 0;
    virtual void requestRender() = 0;
};

} // namespace renderlab
