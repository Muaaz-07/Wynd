//
// Created by Muhammad Muaaz on 3/11/2026.
//
#include <QApplication>
#include <QIcon>
#include <QStringList>
#include "ui/MainWindow.h"

// Enters the Qt event loop after creating the main weather window.
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/assets/icons/Wynd.ico"));

    app.setOrganizationName("Wynd");
    app.setApplicationName("Wynd");

    const QStringList arguments = app.arguments();
    const bool startHiddenInTray =
        arguments.contains(QStringLiteral("--tray"), Qt::CaseInsensitive);

    MainWindow window(startHiddenInTray);
    if (!startHiddenInTray || !window.supportsTrayMode()) {
        window.show();
    }

    return app.exec();
}
