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
class QShowEvent;

namespace pacqt {

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void closeEvent(QCloseEvent *event) override;
    void showEvent(QShowEvent *event) override;

private:
    QString pageTitleForIndex(int index) const;
    QString pageSubtitleForIndex(int index) const;
    void updatePageHeader(int index);
    void applyLanguage(const QString &code, bool persist = true);
    void retranslateUi();
    void buildUi();
    void refreshOverview();
    void showError(const QString &operation, const QString &message);
    void setWalletAvailable(bool available);
    void loadSettings();
    void saveSettings() const;
    void ensureLocalBackendRunning();
    static QString defaultBackendProgram();
    static QStringList defaultBackendArguments();
    static QString defaultBackendURL();
    static bool isLocalBackendURL(const QUrl &url);

    ApiClient m_api;
    ServiceController m_service;
    bool m_walletAvailable = false;
    bool m_initialSizeApplied = false;
    QString m_languageCode;

    QLabel *m_brandLabel;
    QLabel *m_brandSubLabel;
    QListWidget *m_nav;
    QLabel *m_pageTitleLabel;
    QLabel *m_pageSubtitleLabel;
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
