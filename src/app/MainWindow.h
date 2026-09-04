#pragma once

#include "assets/AssetRegistry.h"
#include "assets/ProceduralMeshLibrary.h"
#include "renderer/ShaderLibrary.h"
#include "scene/EntityId.h"
#include "scene/SceneDocument.h"

#include <QMainWindow>

#include <filesystem>
#include <unordered_map>

class QTreeWidget;
class QTreeWidgetItem;
class QUndoStack;
class QCloseEvent;
class QMenu;

namespace renderlab {
class RenderViewport;
class TransformInspector;
struct EntitySnapshot;

// 编辑器主窗口，组合资产、场景、程序化网格库及渲染视口。
class MainWindow final : public QMainWindow {
    Q_OBJECT

  public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

  protected:
    // 窗口关闭前为尚未保存的场景提供保存、放弃和取消三种选择。
    void closeEvent(QCloseEvent* event) override;

  private:
    // 创建启动时用于验证最小渲染链路的默认场景。
    void createDefaultScene();
    // 选择 glTF/GLB 文件并以单个可撤销实体子树导入当前场景。
    void importModel();
    void openScene();
    // 返回是否已完成保存；用户取消文件对话框时返回 false。
    bool saveScene();
    bool saveSceneAs();
    // 保存成功后更新窗口关联路径及状态提示。
    bool saveSceneTo(const std::filesystem::path& path);
    // 从指定路径事务式加载场景，成功后才替换编辑器状态。
    bool openSceneFrom(const std::filesystem::path& path);
    // 若场景已修改，询问用户是否保存；false 表示取消当前操作。
    bool confirmSceneReplacement();
    // 同步文件名和脏状态星号到窗口标题。
    void updateWindowTitle();
    // 记录并重建最近场景菜单，最多保留八个有效顺序项。
    void addRecentScene(const std::filesystem::path& path);
    void rebuildRecentScenesMenu();
    // 确保场景文件可能引用的内置资产已经注册。
    void ensureBuiltinAssets();
    // 提交当前编辑、清空旧命令，再用默认内容替换场景。
    void resetScene();
    // 创建菜单及程序化图元添加动作。
    void createMenus();
    // 通过命令创建实体，并选中新创建的根实体。
    void createEntity(EntitySnapshot snapshot, const QString& commandText);
    // 删除当前选中实体及其子树。
    void deleteSelectedEntity();
    // 复制当前选中实体及其子树。
    void duplicateSelectedEntity();
    // 弹出名称输入框，并通过命令修改当前实体名称。
    void renameSelectedEntity();
    // 创建场景树和属性检查器停靠面板。
    void createDockPanels();
    // 根据 SceneDocument 完整重建场景树控件。
    void refreshSceneTree();
    // 递归添加一个实体及其子节点。
    void addEntityToTree(EntityId entity, QTreeWidgetItem* parentItem);
    // 显示所选实体的基础组件信息。
    void updateInspector(EntityId entity);
    // 作为编辑器选择状态的唯一入口，同步视口、场景树和属性面板。
    void selectEntity(EntityId entity);

    // 视口在析构函数中先销毁；ShaderLibrary 和 AssetRegistry 随后才释放。
    ShaderLibrary shaderLibrary_;
    ShaderHandle defaultShader_;
    AssetRegistry assetRegistry_;
    ProceduralMeshLibrary proceduralMeshes_{assetRegistry_};
    MaterialHandle defaultMaterial_;
    SceneDocument scene_;
    EntityId selectedEntity_{NullEntity};
    QUndoStack* undoStack_{nullptr};
    QTreeWidget* sceneTree_{nullptr};
    std::unordered_map<EntityId, QTreeWidgetItem*> sceneTreeItems_;
    TransformInspector* transformInspector_{nullptr};
    RenderViewport* viewport_{nullptr};
    std::filesystem::path currentScenePath_;
    QMenu* recentScenesMenu_{nullptr};
};

} // namespace renderlab
