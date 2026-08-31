#include "app/MainWindow.h"

#include "app/TransformInspector.h"
#include "assets/AssetId.h"
#include "editor/EntityCommands.h"
#include "editor/TransformCommand.h"
#include "serialization/SceneSerializer.h"

#include "core/AppInfo.h"
#include "renderer/RenderViewport.h"

#include <QAction>
#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QDockWidget>
#include <QFileInfo>
#include <QFileDialog>
#include <QInputDialog>
#include <QKeySequence>
#include <QLineEdit>
#include <QListWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QToolBar>
#include <QTimer>
#include <QTreeWidget>
#include <QUndoStack>
#include <QVariant>

namespace renderlab {
namespace {
constexpr int kMaximumRecentScenes = 8;
constexpr auto kRecentScenesSettingsKey = "recentScenes";

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
    resize(1440, 900);

    undoStack_ = new QUndoStack(this);
    connect(undoStack_, &QUndoStack::cleanChanged, this,
            [this] { updateWindowTitle(); });
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
    undoStack_->setClean();
    updateWindowTitle();

    viewport_ = new RenderViewport(assetRegistry_, this);
    connect(viewport_, &RenderViewport::selectionRequested, this, &MainWindow::selectEntity);
    connect(viewport_, &RenderViewport::transformPreviewed, this,
            [this](EntityId entity) { updateInspector(entity); });
    connect(viewport_, &RenderViewport::transformEditCommitted, this,
            [this](EntityId entity, const TransformComponent& before,
                   const TransformComponent& after) {
                QString text = tr("Move Entity");
                if (viewport_->gizmoMode() == GizmoMode::Rotate) {
                    text = tr("Rotate Entity");
                } else if (viewport_->gizmoMode() == GizmoMode::Scale) {
                    text = tr("Scale Entity");
                }
                undoStack_->push(new TransformCommand(scene_, entity, before, after, text));
            });
    viewport_->setScene(&scene_);
    setCentralWidget(viewport_);

    createMenus();
    createDockPanels();
    statusBar()->showMessage(tr("Ready — SceneDocument initialized"));
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (confirmSceneReplacement()) {
        event->accept();
    } else {
        event->ignore();
    }
}

MainWindow::~MainWindow() {
    // 视口后端引用 assetRegistry_，必须在注册表析构前主动销毁 Qt 中央控件。
    delete takeCentralWidget();
    viewport_ = nullptr;
}

void MainWindow::createDefaultScene() {
    // 新建场景只替换 SceneDocument，默认材质资产可以跨场景安全复用。
    if (!defaultMaterial_.valid()) {
        defaultMaterial_ = assetRegistry_.createMaterial(
            std::string{asset_ids::DefaultMaterial}, MaterialAsset{
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
    auto* openAction = fileMenu->addAction(tr("Open Scene..."));
    openAction->setShortcut(QKeySequence::Open);
    connect(openAction, &QAction::triggered, this, &MainWindow::openScene);
    auto* saveAction = fileMenu->addAction(tr("Save"));
    saveAction->setShortcut(QKeySequence::Save);
    connect(saveAction, &QAction::triggered, this, &MainWindow::saveScene);
    auto* saveAsAction = fileMenu->addAction(tr("Save As..."));
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    connect(saveAsAction, &QAction::triggered, this, &MainWindow::saveSceneAs);
    recentScenesMenu_ = fileMenu->addMenu(tr("Open Recent"));
    rebuildRecentScenesMenu();
    fileMenu->addSeparator();
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


    auto* toolbar = addToolBar(tr("Transform"));
    toolbar->setObjectName(QStringLiteral("TransformToolbar"));
    auto* modeGroup = new QActionGroup(toolbar);
    modeGroup->setExclusive(true);
    auto addMode = [this, toolbar, modeGroup](const QString& text, const QString& tip,
                                               const GizmoMode mode, const bool checked) {
        auto* action = toolbar->addAction(text);
        action->setCheckable(true);
        action->setChecked(checked);
        action->setToolTip(tip);
        modeGroup->addAction(action);
        connect(action, &QAction::triggered, this,
                [this, mode] { viewport_->setGizmoMode(mode); });
    };
    addMode(tr("Move"), tr("Move tool (W)"), GizmoMode::Translate, true);
    addMode(tr("Rotate"), tr("Rotate tool (E)"), GizmoMode::Rotate, false);
    addMode(tr("Scale"), tr("Scale tool (R)"), GizmoMode::Scale, false);
    toolbar->addSeparator();
    auto* localAction = toolbar->addAction(tr("World"));
    localAction->setCheckable(true);
    localAction->setToolTip(tr("Toggle World/Local transform space"));
    connect(localAction, &QAction::toggled, this, [this, localAction](const bool local) {
        viewport_->setGizmoSpace(local ? GizmoSpace::Local : GizmoSpace::World);
        localAction->setText(local ? tr("Local") : tr("World"));
    });
    toolbar->addSeparator();
    auto* snapHint = toolbar->addAction(tr("Ctrl: Snap"));
    snapHint->setEnabled(false);
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

void MainWindow::ensureBuiltinAssets() {
    (void)proceduralMeshes_.unitCube();
    (void)proceduralMeshes_.unitPlane();
    (void)proceduralMeshes_.uvSphere(32, 16);
}

void MainWindow::openScene() {
    const QString file = QFileDialog::getOpenFileName(
        this, tr("Open Scene"), {}, tr("RenderLab Scene (*.renderlab *.json)"));
    if (file.isEmpty()) return;
    if (!confirmSceneReplacement()) return;
    (void)openSceneFrom(std::filesystem::path{file.toStdWString()});
}

bool MainWindow::openSceneFrom(const std::filesystem::path& path) {
    if (transformInspector_ != nullptr) transformInspector_->commitPendingEdit();
    ensureBuiltinAssets();

    const SceneIoResult result = SceneSerializer::load(scene_, assetRegistry_, path);
    if (!result) {
        QMessageBox::critical(this, tr("Open Scene Failed"),
                              QString::fromStdString(result.error));
        return false;
    }
    undoStack_->clear();
    selectedEntity_ = NullEntity;
    currentScenePath_ = path;
    viewport_->setScene(&scene_);
    refreshSceneTree();
    undoStack_->setClean();
    addRecentScene(path);
    updateWindowTitle();
    statusBar()->showMessage(tr("Scene opened"), 3000);
    return true;
}

bool MainWindow::saveScene() {
    if (currentScenePath_.empty()) {
        return saveSceneAs();
    }
    return saveSceneTo(currentScenePath_);
}

bool MainWindow::saveSceneAs() {
    QString file = QFileDialog::getSaveFileName(
        this, tr("Save Scene As"), {}, tr("RenderLab Scene (*.renderlab)"));
    if (file.isEmpty()) return false;
    if (!file.endsWith(QStringLiteral(".renderlab"), Qt::CaseInsensitive)) {
        file += QStringLiteral(".renderlab");
    }
    return saveSceneTo(std::filesystem::path{file.toStdWString()});
}

bool MainWindow::saveSceneTo(const std::filesystem::path& path) {
    if (transformInspector_ != nullptr) transformInspector_->commitPendingEdit();
    const SceneIoResult result = SceneSerializer::save(scene_, assetRegistry_, path);
    if (!result) {
        QMessageBox::critical(this, tr("Save Scene Failed"),
                              QString::fromStdString(result.error));
        return false;
    }
    currentScenePath_ = path;
    undoStack_->setClean();
    addRecentScene(path);
    updateWindowTitle();
    statusBar()->showMessage(tr("Scene saved"), 3000);
    return true;
}

bool MainWindow::confirmSceneReplacement() {
    if (transformInspector_ != nullptr) transformInspector_->commitPendingEdit();
    if (undoStack_ == nullptr || undoStack_->isClean()) return true;

    const auto choice = QMessageBox::warning(
        this, tr("Unsaved Scene"),
        tr("The current scene has unsaved changes. Do you want to save them?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
        QMessageBox::Save);
    if (choice == QMessageBox::Save) return saveScene();
    return choice == QMessageBox::Discard;
}

void MainWindow::updateWindowTitle() {
    const QString documentName = currentScenePath_.empty()
        ? tr("Untitled")
        : QFileInfo(QString::fromStdWString(currentScenePath_.wstring())).fileName();
    const QString modified = undoStack_ != nullptr && !undoStack_->isClean()
        ? QStringLiteral("*") : QString{};
    setWindowTitle(QStringLiteral("%1 %2 — %3%4")
                       .arg(QString::fromUtf8(applicationName()))
                       .arg(QString::fromUtf8(applicationVersion()))
                       .arg(documentName, modified));
}

void MainWindow::addRecentScene(const std::filesystem::path& path) {
    const QString normalized = QFileInfo(QString::fromStdWString(path.wstring())).absoluteFilePath();
    QSettings settings;
    QStringList scenes = settings.value(QLatin1String{kRecentScenesSettingsKey}).toStringList();
    scenes.removeAll(normalized);
    scenes.prepend(normalized);
    while (scenes.size() > kMaximumRecentScenes) scenes.removeLast();
    settings.setValue(QLatin1String{kRecentScenesSettingsKey}, scenes);
    // 延迟到当前 QAction::triggered 回调返回后再销毁旧菜单动作。
    QTimer::singleShot(0, this, [this] { rebuildRecentScenesMenu(); });
}

void MainWindow::rebuildRecentScenesMenu() {
    if (recentScenesMenu_ == nullptr) return;
    recentScenesMenu_->clear();
    QSettings settings;
    const QStringList scenes =
        settings.value(QLatin1String{kRecentScenesSettingsKey}).toStringList();
    for (const QString& scenePath : scenes) {
        auto* action = recentScenesMenu_->addAction(QFileInfo(scenePath).fileName());
        action->setToolTip(scenePath);
        connect(action, &QAction::triggered, this, [this, scenePath] {
            if (!QFileInfo::exists(scenePath)) {
                QMessageBox::warning(this, tr("Scene Not Found"),
                                     tr("The recent scene no longer exists:\n%1").arg(scenePath));
                QSettings settings;
                QStringList recent =
                    settings.value(QLatin1String{kRecentScenesSettingsKey}).toStringList();
                recent.removeAll(scenePath);
                settings.setValue(QLatin1String{kRecentScenesSettingsKey}, recent);
                QTimer::singleShot(0, this, [this] { rebuildRecentScenesMenu(); });
                return;
            }
            if (!confirmSceneReplacement()) return;
            (void)openSceneFrom(std::filesystem::path{scenePath.toStdWString()});
        });
    }
    if (recentScenesMenu_->isEmpty()) {
        auto* emptyAction = recentScenesMenu_->addAction(tr("No Recent Scenes"));
        emptyAction->setEnabled(false);
    }
}

void MainWindow::resetScene() {
    if (!confirmSceneReplacement()) return;
    // 命令保存 SceneDocument 的非拥有指针，场景替换前必须先结束事务并清空历史。
    if (transformInspector_ != nullptr) {
        transformInspector_->commitPendingEdit();
    }
    if (undoStack_ != nullptr) {
        undoStack_->clear();
    }

    scene_ = SceneDocument{};
    selectedEntity_ = NullEntity;
    currentScenePath_.clear();
    createDefaultScene();
    undoStack_->setClean();

    if (viewport_ != nullptr) {
        viewport_->setSelectedEntity(NullEntity);
        viewport_->setScene(&scene_);
    }
    refreshSceneTree();
    updateWindowTitle();
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
