#include "app/MainWindow.h"

#include "core/AppInfo.h"

#include <QApplication>
#include <QCoreApplication>
#include <QSurfaceFormat>

int main(int argc, char* argv[]) {
    QSurfaceFormat format;
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setDepthBufferSize(24);
    format.setStencilBufferSize(8);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication application(argc, argv);
    QCoreApplication::setApplicationName(QString::fromUtf8(renderlab::applicationName()));
    QCoreApplication::setApplicationVersion(QString::fromUtf8(renderlab::applicationVersion()));
    QCoreApplication::setOrganizationName("RenderLab");

    renderlab::MainWindow window;
    window.show();
    return application.exec();
}

