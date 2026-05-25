#include "MainWindow.h"

#include <QApplication>
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Pingancoin Wallet"));
    app.setOrganizationName(QStringLiteral("Pingancoin"));
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/pingancoin-icon-256.png")));

    pacqt::MainWindow window;
    window.show();

    return app.exec();
}
