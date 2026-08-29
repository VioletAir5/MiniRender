#pragma once

#include "assets/AssetRegistry.h"
#include "assets/ProceduralMeshLibrary.h"
#include "scene/EntityId.h"
#include "scene/SceneDocument.h"

#include <QMainWindow>

class QLabel;
class QTreeWidget;
class QTreeWidgetItem;

namespace renderlab {
class RenderViewport;


// 编辑器主窗口，组合资产、场景、程序化网格库及渲染视口。
class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private:
    // 创建启动时用于验证最小渲染链路的默认场景。
    void createDefaultScene();
    // 创建菜单及程序化图元添加动作。
    void createMenus();
    // 创建场景树和属性检查器停靠面板。
    void createDockPanels();
    // 根据 SceneDocument 完整重建场景树控件。
    void refreshSceneTree();
    // 递归添加一个实体及其子节点。
    void addEntityToTree(EntityId entity, QTreeWidgetItem* parentItem);
    // 显示所选实体的基础组件信息。
    void updateInspector(EntityId entity);

    // 声明顺序保证依赖 registry 的对象先析构、registry 最后析构。
    AssetRegistry assetRegistry_;
    ProceduralMeshLibrary proceduralMeshes_{assetRegistry_};
    SceneDocument scene_;
    QTreeWidget* sceneTree_{nullptr};
    QLabel* inspectorLabel_{nullptr};
    RenderViewport* viewport_{nullptr};
};

} // namespace renderlab

