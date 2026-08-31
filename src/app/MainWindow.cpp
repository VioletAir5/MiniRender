#include "app/MainWindow.h"

#include "app/TransformInspector.h"
#include "editor/EntityCommands.h"

#include "core/AppInfo.h"
#include "renderer/RenderViewport.h"

#include <QAction>
#include <QApplication>
#include <QDockWidget>
#include <QInputDialog>
#include <QKeySequence>
#include <QLineEdit>
#include <QListWidget>
#include <QMenuBar>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QTreeWidget>
#include <QUndoStack>
#include <QVariant>

namespace renderlab {
namespace {

// 创建统一配置的停靠面板，并把 content 的所有权交给 QDockWidget。
QDockWidget* makeDock(const QString& title, QWidget* content, QWidget* parent) {
    auto* dock = new QDockWidget(title, parent);
    dock->setObjectName(title);
    dock->setWidget(content);
    return dock;
}

} // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setObjectName("RenderLabMainWindow");
    setWindowTitle(QStringLiteral("%1 %2")
                       .arg(QString::fromUtf8(applicationName()))
                       .arg(QString::fromUtf8(applicationVersion())));
    resize(1440, 900);

    undoStack_ = new QUndoStack(this);
    connect(undoStack_, &QUndoStack::indexChanged, this, [this] {
        if (!scene_.contains(selectedEntity_)) {
            selectedEntity_ = NullEntity;
        }
        // 实体命令可能改变名称和层级；统一从 SceneDocument 重建编辑器视图。
        refreshSceneTree();
        if (viewport_ != nullptr) {
            viewport_->requestRender();
        }
    });

    createDefaultScene();

    viewport_ = new RenderViewport(assetRegistry_, this);
    connect(viewport_, &RenderViewport::selectionRequested, this, &MainWindow::selectEntity);
    viewport_->setScene(&scene_);
    setCentralWidget(viewport_);

    createMenus();
    createDockPanels();
    statusBar()->showMessage(tr("Ready — SceneDocument initialized"));
}

MainWindow::~MainWindow() {
    // 视口后端引用 assetRegistry_，必须在注册表析构前主动销毁 Qt 中央控件。
    delete takeCentralWidget();
    viewport_ = nullptr;
}

void MainWindow::createDefaultScene() {
    // 新建场景只替换 SceneDocument，默认材质资产可以跨场景安全复用。
    if (!defaultMaterial_.valid()) {
        defaultMaterial_ = assetRegistry_.createMaterial(MaterialAsset{
            .name = "Default Material",
            .baseColorFactor = {0.8F, 0.3F, 0.15F, 1.0F},
        });
    }

    const EntityId camera = scene_.createEntity("Camera");
    scene_.addCamera(camera).primary = true;
    scene_.tryGetTransform(camera)->position = {0.0F, 0.0F, 5.0F};

    const EntityId light = scene_.createEntity("Directional Light");
    scene_.addLight(light);
    scene_.tryGetTransform(light)->rotationDegrees = {-45.0F, 45.0F, 0.0F};

    const EntityId cube = scene_.createEntity("Cube");
    auto& renderer = scene_.addMeshRenderer(cube);
    renderer.meshAsset = proceduralMeshes_.unitCube();
    renderer.materialAsset = defaultMaterial_;
    scene_.tryGetTransform(cube)->rotationDegrees = {-20.0F, 30.0F, 0.0F};
}

void MainWindow::createMenus() {
    auto* fileMenu = menuBar()->addMenu(tr("&File"));
    auto* newSceneAction = fileMenu->addAction(tr("New Scene"));
    connect(newSceneAction, &QAction::triggered, this,
            &MainWindow::resetScene);
    fileMenu->addAction(tr("Import Model..."));
    fileMenu->addSeparator();
    fileMenu->addAction(tr("Exit"), qApp, &QApplication::quit);

    auto* editMenu = menuBar()->addMenu(tr("&Edit"));
    auto* undoAction = editMenu->addAction(tr("&Undo"));
    undoAction->setShortcut(QKeySequence::Undo);
    undoAction->setEnabled(false);
    connect(undoAction, &QAction::triggered, this, [this] {
        if (transformInspector_ != nullptr) {
            transformInspector_->commitPendingEdit();
        }
        undoStack_->undo();
    });
    connect(undoStack_, &QUndoStack::canUndoChanged, undoAction, &QAction::setEnabled);
    connect(undoStack_, &QUndoStack::undoTextChanged, undoAction,
            [this, undoAction](const QString& text) {
                undoAction->setText(text.isEmpty() ? tr("&Undo") : tr("&Undo %1").arg(text));
            });

    auto* redoAction = editMenu->addAction(tr("&Redo"));
    redoAction->setShortcut(QKeySequence::Redo);
    redoAction->setEnabled(false);
    connect(redoAction, &QAction::triggered, this, [this] {
        if (transformInspector_ != nullptr) {
            transformInspector_->commitPendingEdit();
        }
        undoStack_->redo();
    });
    connect(undoStack_, &QUndoStack::canRedoChanged, redoAction, &QAction::setEnabled);
    connect(undoStack_, &QUndoStack::redoTextChanged, redoAction,
            [this, redoAction](const QString& text) {
                redoAction->setText(text.isEmpty() ? tr("&Redo") : tr("&Redo %1").arg(text));
            });

    editMenu->addSeparator();
    auto* duplicateAction = editMenu->addAction(tr("&Duplicate"));
    duplicateAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_D));
    connect(duplicateAction, &QAction::triggered, this, &MainWindow::duplicateSelectedEntity);

    auto* renameAction = editMenu->addAction(tr("&Rename"));
    renameAction->setShortcut(QKeySequence(Qt::Key_F2));
    connect(renameAction, &QAction::triggered, this, &MainWindow::renameSelectedEntity);

    auto* deleteAction = editMenu->addAction(tr("&Delete"));
    deleteAction->setShortcut(QKeySequence(Qt::Key_Delete));
    connect(deleteAction, &QAction::triggered, this, &MainWindow::deleteSelectedEntity);

    auto* createMenu = menuBar()->addMenu(tr("&Create"));
    auto* emptyAction = createMenu->addAction(tr("Empty"));
    connect(emptyAction, &QAction::triggered, this, [this] {
        EntitySnapshot snapshot;
        snapshot.name = "Empty";
        createEntity(std::move(snapshot), tr("Create Empty"));
    });

    auto* cubeAction = createMenu->addAction(tr("Cube"));
    connect(cubeAction, &QAction::triggered, this, [this] {
        EntitySnapshot snapshot;
        snapshot.name = "Cube";
        snapshot.meshRenderer = MeshRendererComponent{
            .meshAsset = proceduralMeshes_.unitCube(), .materialAsset = defaultMaterial_};
        createEntity(std::move(snapshot), tr("Create Cube"));
    });

    auto* planeAction = createMenu->addAction(tr("Plane"));
    connect(planeAction, &QAction::triggered, this, [this] {
        EntitySnapshot snapshot;
        snapshot.name = "Plane";
        snapshot.transform.position.y = -1.0F;
        snapshot.transform.scale = {4.0F, 1.0F, 4.0F};
        snapshot.meshRenderer = MeshRendererComponent{
            .meshAsset = proceduralMeshes_.unitPlane(), .materialAsset = defaultMaterial_};
        createEntity(std::move(snapshot), tr("Create Plane"));
    });

    auto* sphereAction = createMenu->addAction(tr("UV Sphere"));
    connect(sphereAction, &QAction::triggered, this, [this] {
        EntitySnapshot snapshot;
        snapshot.name = "UV Sphere";
        snapshot.transform.position.x = 2.0F;
        snapshot.meshRenderer = MeshRendererComponent{
            .meshAsset = proceduralMeshes_.uvSphere(32, 16), .materialAsset = defaultMaterial_};
        createEntity(std::move(snapshot), tr("Create UV Sphere"));
    });

    menuBar()->addMenu(tr("&Render"));
    menuBar()->addMenu(tr("&Learn"));
}

void MainWindow::createEntity(EntitySnapshot snapshot, const QString& commandText) {
    if (transformInspector_ != nullptr) {
        transformInspector_->commitPendingEdit();
    }
    auto* command = new CreateEntityCommand(scene_, std::move(snapshot), commandText);
    undoStack_->push(command);
    selectEntity(command->entity());
}

void MainWindow::deleteSelectedEntity() {
    if (!scene_.contains(selectedEntity_)) {
        return;
    }
    if (transformInspector_ != nullptr) {
        transformInspector_->commitPendingEdit();
    }
    undoStack_->push(
        new DeleteEntityCommand(scene_, selectedEntity_, tr("Delete Entity")));
}

void MainWindow::duplicateSelectedEntity() {
    if (!scene_.contains(selectedEntity_)) {
        return;
    }
    if (transformInspector_ != nullptr) {
        transformInspector_->commitPendingEdit();
    }
    auto* command =
        new DuplicateEntityCommand(scene_, selectedEntity_, tr("Duplicate Entity"));
    undoStack_->push(command);
    selectEntity(command->entity());
}

void MainWindow::renameSelectedEntity() {
    const EntityMetadata* metadata = scene_.tryGetEntity(selectedEntity_);
    if (metadata == nullptr) {
        return;
    }
    if (transformInspector_ != nullptr) {
        transformInspector_->commitPendingEdit();
    }

    bool accepted = false;
    const QString oldName = QString::fromStdString(metadata->name);
    const QString newName = QInputDialog::getText(this, tr("Rename Entity"), tr("Name:"),
                                                  QLineEdit::Normal, oldName, &accepted)
                                .trimmed();
    if (!accepted || newName.isEmpty() || newName == oldName) {
        return;
    }

    undoStack_->push(new RenameEntityCommand(scene_, selectedEntity_, metadata->name,
                                             newName.toStdString(), tr("Rename Entity")));
}

void MainWindow::resetScene() {
    // 命令保存 SceneDocument 的非拥有指针，场景替换前必须先结束事务并清空历史。
    if (transformInspector_ != nullptr) {
        transformInspector_->commitPendingEdit();
    }
    if (undoStack_ != nullptr) {
        undoStack_->clear();
    }

    scene_ = SceneDocument{};
    selectedEntity_ = NullEntity;
    createDefaultScene();

    if (viewport_ != nullptr) {
        viewport_->setSelectedEntity(NullEntity);
        viewport_->setScene(&scene_);
    }
    refreshSceneTree();
    statusBar()->showMessage(tr("New scene created"), 3000);
}

void MainWindow::createDockPanels() {
    sceneTree_ = new QTreeWidget(this);
    sceneTree_->setHeaderLabel(tr("Scene"));
    connect(sceneTree_, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem* current) {
        const EntityId entity =
            current == nullptr ? NullEntity : current->data(0, Qt::UserRole).toULongLong();
        selectEntity(entity);
    });
    addDockWidget(Qt::LeftDockWidgetArea, makeDock(tr("Scene"), sceneTree_, this));

    transformInspector_ = new TransformInspector(this);
    transformInspector_->setUndoStack(undoStack_);
    connect(transformInspector_, &TransformInspector::transformEdited, this, [this](EntityId) {
        if (viewport_ != nullptr) {
            viewport_->requestRender();
        }
    });
    addDockWidget(Qt::RightDockWidgetArea, makeDock(tr("Inspector"), transformInspector_, this));

    auto* output = new QListWidget(this);
    output->addItem(tr("RenderLab initialized"));
    output->addItem(tr("Default SceneDocument created"));
    addDockWidget(Qt::BottomDockWidgetArea, makeDock(tr("Console"), output, this));

    refreshSceneTree();
}

void MainWindow::refreshSceneTree() {
    if (sceneTree_ == nullptr) {
        return;
    }

    const EntityId preservedSelection =
        scene_.contains(selectedEntity_) ? selectedEntity_ : NullEntity;
    // 树控件重建会使旧 QTreeWidgetItem 指针失效，因此同步清空索引。
    sceneTree_->clear();
    sceneTreeItems_.clear();
    for (const EntityId root : scene_.rootEntities()) {
        addEntityToTree(root, nullptr);
    }
    sceneTree_->expandAll();
    selectEntity(preservedSelection);
}

void MainWindow::addEntityToTree(const EntityId entity, QTreeWidgetItem* parentItem) {
    const EntityMetadata* metadata = scene_.tryGetEntity(entity);
    if (metadata == nullptr) {
        return;
    }

    auto* item = new QTreeWidgetItem({QString::fromStdString(metadata->name)});
    item->setData(0, Qt::UserRole, QVariant::fromValue<qulonglong>(entity));
    sceneTreeItems_.emplace(entity, item);

    if (parentItem == nullptr) {
        sceneTree_->addTopLevelItem(item);
    } else {
        parentItem->addChild(item);
    }

    for (const EntityId child : metadata->children) {
        addEntityToTree(child, item);
    }
}

void MainWindow::updateInspector(const EntityId entity) {
    if (transformInspector_ == nullptr) {
        return;
    }
    transformInspector_->setEntity(&scene_, entity);
}

void MainWindow::selectEntity(const EntityId entity) {
    // 选择属于编辑器瞬时状态，不写入 SceneDocument，也不会被场景序列化。
    selectedEntity_ = scene_.contains(entity) ? entity : NullEntity;

    if (viewport_ != nullptr) {
        viewport_->setSelectedEntity(selectedEntity_);
    }

    if (sceneTree_ != nullptr) {
        // setCurrentItem 会再次触发 currentItemChanged，阻断信号可避免递归。
        const QSignalBlocker blocker{sceneTree_};
        const auto iterator = sceneTreeItems_.find(selectedEntity_);
        if (iterator == sceneTreeItems_.end()) {
            sceneTree_->setCurrentItem(nullptr);
            sceneTree_->clearSelection();
        } else {
            sceneTree_->setCurrentItem(iterator->second);
            sceneTree_->scrollToItem(iterator->second);
        }
    }

    updateInspector(selectedEntity_);
}

} // namespace renderlab
