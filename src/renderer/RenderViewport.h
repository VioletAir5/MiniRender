#pragma once

#include "editor/EditorCamera.h"
#include "editor/TranslateGizmo.h"
#include "renderer/SceneRenderer.h"
#include "scene/Components.h"
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
class ShaderLibrary;
class TransformGizmoOverlay;

// 编辑器视口控制器：拥有相机和场景提取器，并组合一个可替换渲染表面。
class RenderViewport final : public QWidget {
    Q_OBJECT

  public:
    // registry 必须比视口及其内部渲染表面存活更久。
    explicit RenderViewport(const AssetRegistry& registry, const ShaderLibrary& shaderLibrary,
                            QWidget* parent = nullptr);
    ~RenderViewport() override;

    // 设置非拥有的场景指针；调用方负责保证其生命周期。
    void setScene(SceneDocument* scene);
    // 同步编辑器的当前选择，供聚焦及后续轮廓渲染使用。
    void setSelectedEntity(EntityId entity);
    // 重新提取场景并请求表面绘制。
    void requestRender();
    void setGizmoMode(GizmoMode mode);
    void setGizmoSpace(GizmoSpace space);
    [[nodiscard]] GizmoMode gizmoMode() const noexcept;
    [[nodiscard]] GizmoSpace gizmoSpace() const noexcept;

  signals:
    // 用户在视口点击后，请求编辑器切换当前选择；空实体表示清空选择。
    void selectionRequested(EntityId entity);
    // 拖动期间通知 Inspector 刷新实时预览。
    void transformPreviewed(EntityId entity);
    // 鼠标释放后提交一次完整变换，交给 MainWindow 写入 UndoStack。
    void transformEditCommitted(EntityId entity, TransformComponent before,
                                TransformComponent after);

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
    [[nodiscard]] static NavigationMode navigationModeFor(const QMouseEvent& event) noexcept;

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

    [[nodiscard]] bool beginGizmoDrag(QMouseEvent& event);
    [[nodiscard]] bool updateGizmoDrag(QMouseEvent& event);
    [[nodiscard]] bool endGizmoDrag(QMouseEvent& event);
    void cancelGizmoDrag();
    void updateGizmoOverlay(const RenderView& view);
    const AssetRegistry& registry_;
    SceneDocument* scene_{nullptr};
    EntityId selectedEntity_{NullEntity};
    EditorCamera editorCamera_;
    SceneRenderer sceneRenderer_;
    std::unique_ptr<IRenderSurface> surface_;
    TransformGizmoOverlay* gizmoOverlay_{nullptr};
    TranslateGizmoGeometry gizmoGeometry_;
    GizmoAxis gizmoAxis_{GizmoAxis::None};
    QPoint gizmoStartMouse_;
    TransformComponent gizmoBefore_;
    EntityId gizmoEntity_{NullEntity};

    GizmoMode gizmoMode_{GizmoMode::Translate};
    GizmoSpace gizmoSpace_{GizmoSpace::World};
    NavigationMode navigationMode_{NavigationMode::None};
    Qt::MouseButton navigationButton_{Qt::NoButton};
    QPoint lastMousePosition_;
};

} // namespace renderlab
