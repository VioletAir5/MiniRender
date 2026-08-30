#pragma once

#include "editor/EditorCamera.h"
#include "renderer/SceneRenderer.h"
#include "scene/EntityId.h"

#include <QPoint>
#include <QWidget>

#include <memory>

class QEvent;
class QMouseEvent;

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
    // 同步编辑器的当前选择，供聚焦及后续轮廓渲染使用。
    void setSelectedEntity(EntityId entity);
    // 重新提取场景并请求表面绘制。
    void requestRender();

signals:
    // 用户在视口点击后，请求编辑器切换当前选择；空实体表示清空选择。
    void selectionRequested(EntityId entity);

protected:
    // 将表面上的鼠标、键盘和滚轮事件转换为编辑器相机操作。
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    // 一次拖动期间保持不变的导航模式。
    enum class NavigationMode {
        None,
        Orbit,
        Pan,
        Zoom,
    };

    // 根据按下时的鼠标按键和修饰键选择导航模式。
    [[nodiscard]] static NavigationMode
    navigationModeFor(const QMouseEvent& event) noexcept;

    // 开始、更新和结束一次完整的鼠标导航手势。
    [[nodiscard]] bool beginNavigation(QMouseEvent& event);
    [[nodiscard]] bool updateNavigation(QMouseEvent& event);
    [[nodiscard]] bool endNavigation(QMouseEvent& event);

    // 清理鼠标捕获、光标和导航状态；可重复调用。
    void cancelNavigation();

    // 对鼠标位置执行 CPU 三角形拾取。
    [[nodiscard]] EntityId pickEntity(const QPoint& viewportPosition) const;

    // 聚焦当前选择；未选择时恢复聚焦世界原点。
    void focusSelection();

    const AssetRegistry& registry_;
    const SceneDocument* scene_{nullptr};
    EntityId selectedEntity_{NullEntity};
    EditorCamera editorCamera_;
    SceneRenderer sceneRenderer_;
    std::unique_ptr<IRenderSurface> surface_;

    NavigationMode navigationMode_{NavigationMode::None};
    Qt::MouseButton navigationButton_{Qt::NoButton};
    QPoint lastMousePosition_;
};

} // namespace renderlab