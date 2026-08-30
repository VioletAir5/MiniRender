#include "app/MainWindow.h"

#include "core/AppInfo.h"
#include "renderer/RenderViewport.h"

#include <QApplication>
#include <QDockWidget>
#include <QLabel>
#include <QListWidget>
#include <QMenuBar>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QTreeWidget>
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

// 汇总实体已拥有的组件名称，供简单属性面板显示。
QString componentSummary(const SceneDocument& scene, const EntityId entity) {
    QStringList components{QStringLiteral("Transform")};

    if (scene.tryGetMeshRenderer(entity) != nullptr) {
        components.push_back(QStringLiteral("Mesh Renderer"));
    }
    if (scene.tryGetCamera(entity) != nullptr) {
        components.push_back(QStringLiteral("Camera"));
    }
    if (scene.tryGetLight(entity) != nullptr) {
        components.push_back(QStringLiteral("Light"));
    }

    return components.join(QStringLiteral(", "));
}

} // namespace

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setObjectName("RenderLabMainWindow");
    setWindowTitle(QStringLiteral("%1 %2")
                       .arg(QString::fromUtf8(applicationName()))
                       .arg(QString::fromUtf8(applicationVersion())));
    resize(1440, 900);

    createDefaultScene();

    viewport_ = new RenderViewport(assetRegistry_, this);
    connect(viewport_, &RenderViewport::selectionRequested,
            this, &MainWindow::selectEntity);
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
    defaultMaterial_ = assetRegistry_.createMaterial(MaterialAsset{
        .name = "Default Material",
        .baseColorFactor = {0.8F, 0.3F, 0.15F, 1.0F},
    });

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
    fileMenu->addAction(tr("New Scene"));
    fileMenu->addAction(tr("Import Model..."));
    fileMenu->addSeparator();
    fileMenu->addAction(tr("Exit"), qApp, &QApplication::quit);

    menuBar()->addMenu(tr("&Edit"));

    auto* createMenu = menuBar()->addMenu(tr("&Create"));
    auto* emptyAction = createMenu->addAction(tr("Empty"));
    connect(emptyAction, &QAction::triggered, this, [this] {
        scene_.createEntity("Empty");
        refreshSceneTree();
    });

    auto* cubeAction = createMenu->addAction(tr("Cube"));
    connect(cubeAction, &QAction::triggered, this, [this] {
        const EntityId cube = scene_.createEntity("Cube");
        auto& renderer = scene_.addMeshRenderer(cube);
        renderer.meshAsset = proceduralMeshes_.unitCube();
        renderer.materialAsset = defaultMaterial_;
        refreshSceneTree();
        viewport_->requestRender();
    });

    auto* planeAction = createMenu->addAction(tr("Plane"));
    connect(planeAction, &QAction::triggered, this, [this] {
        const EntityId plane = scene_.createEntity("Plane");
        auto& renderer = scene_.addMeshRenderer(plane);
        renderer.meshAsset = proceduralMeshes_.unitPlane();
        renderer.materialAsset = defaultMaterial_;
        // createEntity 保证默认 Transform 存在，此处可安全修改。
        auto* transform = scene_.tryGetTransform(plane);
        transform->position.y = -1.0F;
        transform->scale = {4.0F, 1.0F, 4.0F};
        refreshSceneTree();
        viewport_->requestRender();
    });

    auto* sphereAction = createMenu->addAction(tr("UV Sphere"));
    connect(sphereAction, &QAction::triggered, this, [this] {
        const EntityId sphere = scene_.createEntity("UV Sphere");
        auto& renderer = scene_.addMeshRenderer(sphere);
        renderer.meshAsset = proceduralMeshes_.uvSphere(32, 16);
        renderer.materialAsset = defaultMaterial_;
        scene_.tryGetTransform(sphere)->position.x = 2.0F;
        refreshSceneTree();
        viewport_->requestRender();
    });

    menuBar()->addMenu(tr("&Render"));
    menuBar()->addMenu(tr("&Learn"));
}

void MainWindow::createDockPanels() {
    sceneTree_ = new QTreeWidget(this);
    sceneTree_->setHeaderLabel(tr("Scene"));
    connect(sceneTree_, &QTreeWidget::currentItemChanged, this,
            [this](QTreeWidgetItem* current) {
                const EntityId entity = current == nullptr
                                            ? NullEntity
                                            : current->data(0, Qt::UserRole).toULongLong();
                selectEntity(entity);
            });
    addDockWidget(Qt::LeftDockWidgetArea, makeDock(tr("Scene"), sceneTree_, this));

    inspectorLabel_ = new QLabel(tr("No object selected"), this);
    inspectorLabel_->setAlignment(Qt::AlignLeft | Qt::AlignTop);
    inspectorLabel_->setMargin(8);
    addDockWidget(Qt::RightDockWidgetArea,
                  makeDock(tr("Inspector"), inspectorLabel_, this));

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
    if (inspectorLabel_ == nullptr) {
        return;
    }

    const EntityMetadata* metadata = scene_.tryGetEntity(entity);
    if (metadata == nullptr) {
        inspectorLabel_->setText(tr("No object selected"));
        return;
    }

    inspectorLabel_->setText(
        tr("Name: %1\nEntity ID: %2\nComponents: %3")
            .arg(QString::fromStdString(metadata->name))
            .arg(entity)
            .arg(componentSummary(scene_, entity)));
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

