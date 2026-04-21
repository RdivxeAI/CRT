#include "mainwindow.h"

#include <QApplication>
#include <QFont>
#include <QIcon>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QApplication::setStyle(QStringLiteral("Fusion"));
    app.setApplicationName(QStringLiteral("CRT_EMULATOR"));
    app.setApplicationDisplayName(QStringLiteral("CRT PHYSICS EMULATOR"));
    app.setOrganizationName(QStringLiteral("MechaML Researchers"));
    app.setWindowIcon(QIcon(QStringLiteral(":/branding/logo.png")));
    app.setFont(QFont(QStringLiteral("Tahoma"), 8));

    MainWindow window;
    window.show();
    return app.exec();
}
