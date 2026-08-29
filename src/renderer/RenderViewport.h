#pragma once

#include "editor/EditorCamera.h"
#include "renderer/SceneRenderer.h"

#include <QWidget>
#include <QPoint>

#include <memory>

class QEvent;

namespace renderlab {

class AssetRegistry;
class IRenderSurface;
class SceneDocument;

// 编辑器视口控制器：拥有相机和场景提取器，并组合一个可替换渲染表面。
class RenderViewport final : public QWidget {
    Q_OBJECT

public:
    // registry 必须比视口及其内部渲染表面存活更久。
    explicit RenderViewport(const AssetRegistry& registry,
                            QWidget* parent = nullptr);
    ~RenderViewport() override;

    // 设置非拥有的场景指针；调用方负责保证其生命周期。
    void setScene(const SceneDocument* scene);
    // 重新提取场景并请求表面绘制。
    void requestRender();

protected:
    // 将表面上的鼠标和滚轮事件转换为编辑器相机操作。
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    const SceneDocument* scene_{nullptr};
    EditorCamera editorCamera_;
    SceneRenderer sceneRenderer_;
    std::unique_ptr<IRenderSurface> surface_;
    QPoint lastMousePosition_;
};

} // namespace renderlab
