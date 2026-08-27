#pragma once

#include "editor/EditorCamera.h"
#include "renderer/SceneRenderer.h"

#include <QWidget>
#include <QPoint>

#include <memory>

class QEvent;

namespace renderlab {

class IRenderSurface;
class SceneDocument;

class RenderViewport final : public QWidget {
    Q_OBJECT

public:
    explicit RenderViewport(QWidget* parent = nullptr);
    ~RenderViewport() override;

    void setScene(const SceneDocument* scene);
    void requestRender();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    const SceneDocument* scene_{nullptr};
    EditorCamera editorCamera_;
    SceneRenderer sceneRenderer_;
    std::unique_ptr<IRenderSurface> surface_;
    QPoint lastMousePosition_;
};

} // namespace renderlab

