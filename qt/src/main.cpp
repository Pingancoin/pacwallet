#include "MainWindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Pingancoin Wallet"));
    app.setOrganizationName(QStringLiteral("Pingancoin"));

    pacqt::MainWindow window;
    window.show();

    return app.exec();
}
