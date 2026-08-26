#pragma once

#include <QWidget>

#include <memory>

namespace renderlab {

class IRenderSurface;
class SceneDocument;

class RenderViewport final : public QWidget {
    Q_OBJECT

public:
    explicit RenderViewport(QWidget* parent = nullptr);
    ~RenderViewport() override;

    void setScene(const SceneDocument* scene) noexcept;
    void requestRender();

private:
    std::unique_ptr<IRenderSurface> surface_;
};

} // namespace renderlab

