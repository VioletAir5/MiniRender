#pragma once

#include "scene/EntityId.h"
#include "scene/SceneDocument.h"

#include <QMainWindow>

class QLabel;
class QTreeWidget;
class QTreeWidgetItem;

namespace renderlab {
class RenderViewport;


class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);

private:
    void createDefaultScene();
    void createMenus();
    void createDockPanels();
    void refreshSceneTree();
    void addEntityToTree(EntityId entity, QTreeWidgetItem* parentItem);
    void updateInspector(EntityId entity);

    SceneDocument scene_;
    QTreeWidget* sceneTree_{nullptr};
    QLabel* inspectorLabel_{nullptr};
    RenderViewport* viewport_{nullptr};
};

} // namespace renderlab

