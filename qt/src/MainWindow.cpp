#include "MainWindow.h"
#include "Localization.h"

#include <QApplication>
#include <QCloseEvent>
#include <QFont>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QSettings>
#include <QShowEvent>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QWidget>

namespace pacqt {

namespace {

constexpr int kDefaultWindowWidth = 1024;
constexpr int kDefaultWindowHeight = 648;

}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    buildUi();
    loadSettings();

    connect(&m_api, &ApiClient::overviewReady, this, [this](const Overview &overview) {
        m_walletAvailable = overview.wallet.exists;
        setWalletAvailable(overview.wallet.exists);
        m_overviewPage->setOverview(overview);
        m_receivePage->setOverview(overview);
        m_sendPage->setOverview(overview);
        m_transactionsPage->setOverview(overview);
        m_multisigPage->setOverview(overview);
        m_settingsPage->setOverview(overview);
        statusBar()->showMessage(l10n::text(QStringLiteral("Wallet synced from %1")).arg(overview.rpcUrl), 3000);
    });
    connect(&m_api, &ApiClient::transactionReady, this, [this](const TransactionDetail &detail) {
        m_transactionsPage->setTransactionDetail(detail);
    });
    connect(&m_api, &ApiClient::receiveQrReady, this, [this](const QString &address, const QByteArray &png) {
        m_receivePage->setQrImage(address, png);
    });
    connect(&m_api, &ApiClient::walletCreated, this, [this]() {
        statusBar()->showMessage(l10n::text(QStringLiteral("Wallet created.")), 3000);
        refreshOverview();
    });
    connect(&m_api, &ApiClient::walletEncrypted, this, [this]() {
        statusBar()->showMessage(l10n::text(QStringLiteral("Wallet encrypted.")), 3000);
        refreshOverview();
    });
    connect(&m_api, &ApiClient::walletPassphraseChanged, this, [this]() {
        statusBar()->showMessage(l10n::text(QStringLiteral("Wallet passphrase changed.")), 3000);
        refreshOverview();
    });
    connect(&m_api, &ApiClient::walletRestored, this, [this]() {
        statusBar()->showMessage(l10n::text(QStringLiteral("Wallet restored.")), 3000);
        refreshOverview();
    });
    connect(&m_api, &ApiClient::addressCreated, this, [this]() {
        statusBar()->showMessage(l10n::text(QStringLiteral("New receive address created.")), 3000);
        refreshOverview();
    });
    connect(&m_api, &ApiClient::privateKeyImported, this, [this]() {
        statusBar()->showMessage(l10n::text(QStringLiteral("Private key imported.")), 3000);
        refreshOverview();
    });
    connect(&m_api, &ApiClient::upstreamsUpdated, this, [this]() {
        statusBar()->showMessage(l10n::text(QStringLiteral("RPC upstreams updated.")), 3000);
        refreshOverview();
    });
    connect(&m_api, &ApiClient::transactionSubmitted, this, [this](const QString &txid) {
        statusBar()->showMessage(l10n::text(QStringLiteral("Transaction submitted: %1")).arg(txid), 5000);
        refreshOverview();
    });
    connect(&m_api, &ApiClient::multisigPreviewReady, this, [this](const MultiSigPreviewResult &result) {
        m_multisigPage->setPreviewResult(result);
    });
    connect(&m_api, &ApiClient::requestFailed, this, &MainWindow::showError);

    connect(m_receivePage, &ReceivePage::qrRequested, &m_api, [this](const QString &address) {
        m_api.fetchReceiveQr(address);
    });
    connect(m_welcomePage, &WelcomePage::createWalletRequested, &m_api, &ApiClient::createWallet);
    connect(m_welcomePage, &WelcomePage::restoreWalletRequested, &m_api, &ApiClient::restoreWallet);
    connect(m_receivePage, &ReceivePage::createAddressRequested, &m_api, &ApiClient::createAddress);
    connect(m_sendPage, &SendPage::sendRequested, &m_api, &ApiClient::sendTransaction);
    connect(m_transactionsPage, &TransactionsPage::transactionSelected, &m_api, &ApiClient::fetchTransaction);
    connect(m_multisigPage, &MultisigPage::previewRequested, &m_api, &ApiClient::previewMultisig);
    connect(m_settingsPage, &SettingsPage::appLanguageChanged, this, [this](const QString &code) {
        applyLanguage(code);
    });
    connect(m_settingsPage, &SettingsPage::encryptWalletRequested, &m_api, &ApiClient::encryptWallet);
    connect(m_settingsPage, &SettingsPage::changePassphraseRequested, &m_api, &ApiClient::changePassphrase);
    connect(m_settingsPage, &SettingsPage::importPrivateKeyRequested, &m_api, &ApiClient::importPrivateKey);
    connect(m_settingsPage, &SettingsPage::addUpstreamRequested, &m_api, &ApiClient::addUpstream);
    connect(m_settingsPage, &SettingsPage::selectUpstreamRequested, &m_api, &ApiClient::selectUpstream);
    connect(m_settingsPage, &SettingsPage::backendUrlChanged, this, [this](const QString &url) {
        m_api.setBaseUrl(QUrl(url));
        saveSettings();
        statusBar()->showMessage(l10n::text(QStringLiteral("Backend URL updated.")), 3000);
        refreshOverview();
    });
    connect(m_settingsPage, &SettingsPage::startBackendRequested, this, [this](const QString &program, const QStringList &arguments) {
        m_service.setProgram(program);
        m_service.setArguments(arguments);
        saveSettings();
        m_service.start();
    });
    connect(m_settingsPage, &SettingsPage::stopBackendRequested, &m_service, &ServiceController::stop);
    connect(&m_service, &ServiceController::serviceLog, m_settingsPage, &SettingsPage::appendLog);
    connect(&m_service, &ServiceController::serviceError, this, [this](const QString &message) {
        showError(QStringLiteral("service"), message);
        m_settingsPage->appendLog(message);
    });
    connect(&m_service, &ServiceController::serviceStarted, this, [this]() {
        statusBar()->showMessage(l10n::text(QStringLiteral("Local pacwallet service started.")), 3000);
        refreshOverview();
    });
    connect(&m_service, &ServiceController::serviceStopped, this, [this]() {
        statusBar()->showMessage(l10n::text(QStringLiteral("Local pacwallet service stopped.")), 3000);
    });

    m_refreshTimer.setInterval(15000);
    connect(&m_refreshTimer, &QTimer::timeout, this, &MainWindow::refreshOverview);
    m_refreshTimer.start();

    refreshOverview();
}

void MainWindow::buildUi()
{
    auto font = QApplication::font();
    if (font.pointSizeF() < 12.0) {
        font.setPointSizeF(12.0);
        QApplication::setFont(font);
    }

    setWindowTitle(l10n::text(QStringLiteral("Pingancoin Wallet")));
    resize(kDefaultWindowWidth, kDefaultWindowHeight);

    auto *root = new QWidget(this);
    auto *layout = new QHBoxLayout(root);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(12);

    setStyleSheet(QStringLiteral(
        "QMainWindow { background: #f4f7fb; color: #0f172a; }"
        "QWidget { color: #0f172a; }"
        "QGroupBox { font-size: 13px; font-weight: 700; border: 1px solid #dbe4f0; border-radius: 10px; margin-top: 10px; padding-top: 10px; background: #ffffff; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 14px; padding: 0 4px; color: #111827; }"
        "QPushButton { background: #2563eb; color: white; border: none; border-radius: 8px; padding: 3px 6px; min-height: 22px; font-size: 10px; font-weight: 600; }"
        "QPushButton:hover { background: #1d4ed8; }"
        "QPushButton:disabled { background: #94a3b8; }"
        "QLineEdit, QComboBox, QSpinBox, QTextEdit, QListWidget, QTableWidget { background: white; border: 1px solid #cbd5e1; border-radius: 8px; }"
        "QLineEdit, QComboBox, QSpinBox { min-height: 28px; padding: 2px 8px; }"
        "QComboBox { padding-right: 28px; }"
        "QComboBox::drop-down { subcontrol-origin: padding; subcontrol-position: top right; width: 24px; border-left: 1px solid #dbe4f0; }"
        "QComboBox QAbstractItemView { border: 1px solid #dbe4f0; selection-background-color: #dbeafe; padding: 4px; }"
        "QTextEdit, QListWidget { padding: 6px; }"
        "QHeaderView::section { background: #eef2f7; border: none; border-bottom: 1px solid #d6deea; padding: 7px; font-weight: 700; }"
        "QTableWidget { gridline-color: #eef2f7; selection-background-color: #dbeafe; selection-color: #0f172a; alternate-background-color: #f8fbff; }"
        "QStatusBar { background: #ffffff; border-top: 1px solid #dbe4f0; }"
        "#navList { background: #ffffff; border: 1px solid #dbe4f0; border-radius: 12px; padding: 8px; font-size: 16px; }"
        "#navList::item { padding: 10px 10px; border-radius: 8px; margin: 2px 0; min-height: 28px; }"
        "#navList::item:selected { background: #dbeafe; color: #1d4ed8; font-weight: 700; }"
        "#pageHeader { background: #ffffff; border: 1px solid #dbe4f0; border-radius: 12px; }"
        "#pageTitle { font-size: 21px; font-weight: 700; color: #0f172a; }"
        "#pageSubtitle { font-size: 12px; color: #64748b; }"
        "#brandTitle { font-size: 17px; font-weight: 700; color: #0f172a; }"
        "#brandSubtitle { font-size: 12px; color: #64748b; }"));

    auto *navColumn = new QWidget(this);
    auto *navLayout = new QVBoxLayout(navColumn);
    navLayout->setContentsMargins(0, 0, 0, 0);
    navLayout->setSpacing(10);
    m_brandLabel = new QLabel(this);
    m_brandLabel->setObjectName(QStringLiteral("brandTitle"));
    m_brandSubLabel = new QLabel(this);
    m_brandSubLabel->setObjectName(QStringLiteral("brandSubtitle"));
    m_brandSubLabel->setWordWrap(true);
    navLayout->addWidget(m_brandLabel);
    navLayout->addWidget(m_brandSubLabel);

    m_nav = new QListWidget(this);
    m_nav->setObjectName(QStringLiteral("navList"));
    m_nav->setSpacing(4);
    m_nav->setFixedWidth(184);
    navLayout->addWidget(m_nav, 1);

    auto *contentColumn = new QWidget(this);
    auto *contentLayout = new QVBoxLayout(contentColumn);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(12);

    auto *headerBox = new QWidget(this);
    headerBox->setObjectName(QStringLiteral("pageHeader"));
    auto *headerLayout = new QVBoxLayout(headerBox);
    headerLayout->setContentsMargins(16, 14, 16, 14);
    headerLayout->setSpacing(3);
    m_pageTitleLabel = new QLabel(this);
    m_pageTitleLabel->setObjectName(QStringLiteral("pageTitle"));
    m_pageSubtitleLabel = new QLabel(this);
    m_pageSubtitleLabel->setObjectName(QStringLiteral("pageSubtitle"));
    m_pageSubtitleLabel->setWordWrap(true);
    headerLayout->addWidget(m_pageTitleLabel);
    headerLayout->addWidget(m_pageSubtitleLabel);

    m_stack = new QStackedWidget(this);
    m_welcomePage = new WelcomePage(this);
    m_overviewPage = new OverviewPage(this);
    m_receivePage = new ReceivePage(this);
    m_sendPage = new SendPage(this);
    m_transactionsPage = new TransactionsPage(this);
    m_multisigPage = new MultisigPage(this);
    m_settingsPage = new SettingsPage(this);

    m_stack->addWidget(m_welcomePage);
    m_stack->addWidget(m_overviewPage);
    m_stack->addWidget(m_receivePage);
    m_stack->addWidget(m_sendPage);
    m_stack->addWidget(m_transactionsPage);
    m_stack->addWidget(m_multisigPage);
    m_stack->addWidget(m_settingsPage);

    contentLayout->addWidget(headerBox);
    contentLayout->addWidget(m_stack, 1);

    layout->addWidget(navColumn);
    layout->addWidget(contentColumn, 1);
    setCentralWidget(root);

    connect(m_nav, &QListWidget::currentRowChanged, this, [this](int row) {
        m_stack->setCurrentIndex(row);
        updatePageHeader(row);
    });
    m_nav->setCurrentRow(0);
    retranslateUi();
    statusBar()->showMessage(l10n::text(QStringLiteral("Native Qt wallet ready.")));
}

void MainWindow::refreshOverview()
{
    m_api.fetchOverview();
}

void MainWindow::showError(const QString &operation, const QString &message)
{
    if (operation == QStringLiteral("overview") || operation == QStringLiteral("receive-qr")) {
        statusBar()->showMessage(QStringLiteral("%1 failed: %2").arg(operation, message), 5000);
        m_settingsPage->appendLog(QStringLiteral("%1 failed: %2").arg(operation, message));
        return;
    }
    QMessageBox::warning(this, l10n::text(QStringLiteral("Pingancoin Wallet")), l10n::text(QStringLiteral("%1 failed: %2")).arg(operation, message));
    statusBar()->showMessage(l10n::text(QStringLiteral("%1 failed.")).arg(operation), 4000);
}

void MainWindow::setWalletAvailable(bool available)
{
    for (int i = 1; i < m_nav->count(); ++i) {
        m_nav->item(i)->setFlags(available ? (Qt::ItemIsSelectable | Qt::ItemIsEnabled) : Qt::NoItemFlags);
    }
    if (!available) {
        m_nav->setCurrentRow(0);
    } else if (m_nav->currentRow() == 0) {
        m_nav->setCurrentRow(1);
    }
}

void MainWindow::loadSettings()
{
    QSettings settings(QStringLiteral("Pingancoin"), QStringLiteral("pacwallet-qt"));
    const QString languageCode = settings.value(QStringLiteral("ui/language"), l10n::defaultLanguageCode()).toString();
    const QString backendUrl = settings.value(QStringLiteral("backend/url"), QStringLiteral("http://127.0.0.1:19709")).toString();
    const QString backendProgram = settings.value(QStringLiteral("backend/program"), QStringLiteral("pacwallet")).toString();
    const QStringList backendArguments = settings.value(QStringLiteral("backend/arguments"),
        QStringList{QStringLiteral("serve"), QStringLiteral("--network"), QStringLiteral("mainnet"), QStringLiteral("--rpc"), QStringLiteral("http://127.0.0.1:9509"), QStringLiteral("--listen"), QStringLiteral("127.0.0.1:19709")}).toStringList();

    m_api.setBaseUrl(QUrl(backendUrl));
    m_service.setProgram(backendProgram);
    m_service.setArguments(backendArguments);
    applyLanguage(languageCode, false);
    m_settingsPage->setBackendUrl(backendUrl);
    m_settingsPage->setBackendProgram(backendProgram);
    m_settingsPage->setBackendArguments(backendArguments);
    restoreGeometry(settings.value(QStringLiteral("window/geometry")).toByteArray());
    resize(kDefaultWindowWidth, kDefaultWindowHeight);
}

void MainWindow::saveSettings() const
{
    QSettings settings(QStringLiteral("Pingancoin"), QStringLiteral("pacwallet-qt"));
    settings.setValue(QStringLiteral("ui/language"), m_languageCode);
    settings.setValue(QStringLiteral("backend/url"), m_api.baseUrl().toString());
    settings.setValue(QStringLiteral("backend/program"), m_service.program());
    settings.setValue(QStringLiteral("backend/arguments"), m_service.arguments());
    settings.setValue(QStringLiteral("window/geometry"), geometry().isValid() ? saveGeometry() : QByteArray());
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
    QMainWindow::closeEvent(event);
}

void MainWindow::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    if (!m_initialSizeApplied) {
        setFixedSize(kDefaultWindowWidth, kDefaultWindowHeight);
        QTimer::singleShot(120, this, [this]() {
            setMinimumSize(880, 620);
            setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
            resize(kDefaultWindowWidth, kDefaultWindowHeight);
        });
        m_initialSizeApplied = true;
    }
}

void MainWindow::applyLanguage(const QString &code, bool persist)
{
    m_languageCode = code.startsWith(QStringLiteral("zh")) ? QStringLiteral("zh_CN") : QStringLiteral("en");
    l10n::setCurrentLanguageCode(m_languageCode);
    m_settingsPage->setCurrentLanguageCode(m_languageCode);
    retranslateUi();
    if (persist) {
        saveSettings();
    }
    refreshOverview();
}

void MainWindow::retranslateUi()
{
    setWindowTitle(l10n::text(QStringLiteral("Pingancoin Wallet")));
    m_brandLabel->setText(l10n::text(QStringLiteral("Pingancoin Wallet")));
    m_brandSubLabel->setText(l10n::text(QStringLiteral("Native desktop wallet for balances, transfers, multisig, and node settings.")));
    const int currentRow = m_nav->currentRow();
    const QStringList items{
        l10n::text(QStringLiteral("Welcome")),
        l10n::text(QStringLiteral("Overview")),
        l10n::text(QStringLiteral("Receive")),
        l10n::text(QStringLiteral("Send")),
        l10n::text(QStringLiteral("Transactions")),
        l10n::text(QStringLiteral("Multisig")),
        l10n::text(QStringLiteral("Settings"))
    };
    m_nav->clear();
    m_nav->addItems(items);
    m_nav->setCurrentRow(currentRow < 0 ? 0 : qMin(currentRow, m_nav->count() - 1));
    m_welcomePage->retranslateUi();
    m_overviewPage->retranslateUi();
    m_receivePage->retranslateUi();
    m_sendPage->retranslateUi();
    m_transactionsPage->retranslateUi();
    m_multisigPage->retranslateUi();
    m_settingsPage->retranslateUi();
    updatePageHeader(m_nav->currentRow());
}

QString MainWindow::pageTitleForIndex(int index) const
{
    switch (index) {
    case 0: return l10n::text(QStringLiteral("Welcome"));
    case 1: return l10n::text(QStringLiteral("Overview"));
    case 2: return l10n::text(QStringLiteral("Receive"));
    case 3: return l10n::text(QStringLiteral("Send"));
    case 4: return l10n::text(QStringLiteral("Transactions"));
    case 5: return l10n::text(QStringLiteral("Multisig"));
    case 6: return l10n::text(QStringLiteral("Settings"));
    default: return l10n::text(QStringLiteral("Pingancoin Wallet"));
    }
}

QString MainWindow::pageSubtitleForIndex(int index) const
{
    switch (index) {
    case 0: return l10n::text(QStringLiteral("Create a new wallet on this Mac, or restore an existing wallet.json to continue from another machine."));
    case 1: return l10n::text(QStringLiteral("Review wallet balances, node status, stored keys, and spendable outputs at a glance."));
    case 2: return l10n::text(QStringLiteral("Generate fresh receive addresses, inspect public keys, and export QR codes for payments."));
    case 3: return l10n::text(QStringLiteral("Prepare a payment, choose fee and change behavior, then confirm before broadcasting."));
    case 4: return l10n::text(QStringLiteral("Search through transfers, coinbase rewards, and raw transaction details in one place."));
    case 5: return l10n::text(QStringLiteral("Preview the project multisig address, export signer data, and save the final scripts."));
    case 6: return l10n::text(QStringLiteral("Switch language, manage upstream nodes, tune the local service, and harden wallet security."));
    default: return QString();
    }
}

void MainWindow::updatePageHeader(int index)
{
    m_pageTitleLabel->setText(pageTitleForIndex(index));
    m_pageSubtitleLabel->setText(pageSubtitleForIndex(index));
}

} // namespace pacqt
