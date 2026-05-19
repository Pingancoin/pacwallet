#pragma once

#include "ApiClient.h"
#include "ServiceController.h"
#include "pages/MultisigPage.h"
#include "pages/OverviewPage.h"
#include "pages/ReceivePage.h"
#include "pages/SendPage.h"
#include "pages/SettingsPage.h"
#include "pages/TransactionsPage.h"
#include "pages/WelcomePage.h"

#include <QListWidget>
#include <QMainWindow>
#include <QStackedWidget>
#include <QTimer>

class QCloseEvent;

namespace pacqt {

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void buildUi();
    void refreshOverview();
    void showError(const QString &operation, const QString &message);
    void setWalletAvailable(bool available);
    void loadSettings();
    void saveSettings() const;

    ApiClient m_api;
    ServiceController m_service;
    bool m_walletAvailable = false;

    QListWidget *m_nav;
    QStackedWidget *m_stack;
    WelcomePage *m_welcomePage;
    OverviewPage *m_overviewPage;
    ReceivePage *m_receivePage;
    SendPage *m_sendPage;
    TransactionsPage *m_transactionsPage;
    MultisigPage *m_multisigPage;
    SettingsPage *m_settingsPage;
    QTimer m_refreshTimer;
};

} // namespace pacqt
