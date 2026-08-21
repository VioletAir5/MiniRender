#include "app/MainWindow.h"

#include "core/AppInfo.h"
#include "renderer/RenderViewport.h"

#include <QApplication>
#include <QDockWidget>
#include <QLabel>
#include <QListWidget>
#include <QMenuBar>
#include <QStatusBar>
#include <QTreeWidget>

namespace renderlab {
namespace {

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

    setCentralWidget(new RenderViewport(this));
    createMenus();
    createDockPanels();
    statusBar()->showMessage(tr("Ready — OpenGL viewport initialized"));
}

void MainWindow::createMenus() {
    auto* fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(tr("New Scene"));
    fileMenu->addAction(tr("Import Model..."));
    fileMenu->addSeparator();
    fileMenu->addAction(tr("Exit"), qApp, &QApplication::quit);

    menuBar()->addMenu(tr("&Edit"));
    menuBar()->addMenu(tr("&Create"));
    menuBar()->addMenu(tr("&Render"));
    menuBar()->addMenu(tr("&Learn"));
}

void MainWindow::createDockPanels() {
    auto* sceneTree = new QTreeWidget(this);
    sceneTree->setHeaderLabel(tr("Scene"));
    sceneTree->addTopLevelItem(new QTreeWidgetItem({tr("Camera")}));
    sceneTree->addTopLevelItem(new QTreeWidgetItem({tr("Directional Light")}));
    addDockWidget(Qt::LeftDockWidgetArea, makeDock(tr("Scene"), sceneTree, this));

    addDockWidget(Qt::RightDockWidgetArea,
                  makeDock(tr("Inspector"), new QLabel(tr("No object selected"), this), this));

    auto* output = new QListWidget(this);
    output->addItem(tr("RenderLab initialized"));
    addDockWidget(Qt::BottomDockWidgetArea, makeDock(tr("Console"), output, this));
}

} // namespace renderlab

