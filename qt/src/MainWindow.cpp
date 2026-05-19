#include "MainWindow.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QSettings>
#include <QSplitter>
#include <QStatusBar>
#include <QWidget>

namespace pacqt {

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
        m_transactionsPage->setOverview(overview);
        m_multisigPage->setOverview(overview);
        m_settingsPage->setOverview(overview);
        statusBar()->showMessage(QStringLiteral("Wallet synced from %1").arg(overview.rpcUrl), 3000);
    });
    connect(&m_api, &ApiClient::transactionReady, this, [this](const TransactionDetail &detail) {
        m_transactionsPage->setTransactionDetail(detail);
    });
    connect(&m_api, &ApiClient::receiveQrReady, this, [this](const QString &address, const QByteArray &png) {
        m_receivePage->setQrImage(address, png);
    });
    connect(&m_api, &ApiClient::walletCreated, this, [this]() {
        statusBar()->showMessage(QStringLiteral("Wallet created."), 3000);
        refreshOverview();
    });
    connect(&m_api, &ApiClient::walletEncrypted, this, [this]() {
        statusBar()->showMessage(QStringLiteral("Wallet encrypted."), 3000);
        refreshOverview();
    });
    connect(&m_api, &ApiClient::walletPassphraseChanged, this, [this]() {
        statusBar()->showMessage(QStringLiteral("Wallet passphrase changed."), 3000);
        refreshOverview();
    });
    connect(&m_api, &ApiClient::walletRestored, this, [this]() {
        statusBar()->showMessage(QStringLiteral("Wallet restored."), 3000);
        refreshOverview();
    });
    connect(&m_api, &ApiClient::addressCreated, this, [this]() {
        statusBar()->showMessage(QStringLiteral("New receive address created."), 3000);
        refreshOverview();
    });
    connect(&m_api, &ApiClient::privateKeyImported, this, [this]() {
        statusBar()->showMessage(QStringLiteral("Private key imported."), 3000);
        refreshOverview();
    });
    connect(&m_api, &ApiClient::upstreamsUpdated, this, [this]() {
        statusBar()->showMessage(QStringLiteral("RPC upstreams updated."), 3000);
        refreshOverview();
    });
    connect(&m_api, &ApiClient::transactionSubmitted, this, [this](const QString &txid) {
        statusBar()->showMessage(QStringLiteral("Transaction submitted: %1").arg(txid), 5000);
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
    connect(m_settingsPage, &SettingsPage::encryptWalletRequested, &m_api, &ApiClient::encryptWallet);
    connect(m_settingsPage, &SettingsPage::changePassphraseRequested, &m_api, &ApiClient::changePassphrase);
    connect(m_settingsPage, &SettingsPage::importPrivateKeyRequested, &m_api, &ApiClient::importPrivateKey);
    connect(m_settingsPage, &SettingsPage::addUpstreamRequested, &m_api, &ApiClient::addUpstream);
    connect(m_settingsPage, &SettingsPage::selectUpstreamRequested, &m_api, &ApiClient::selectUpstream);
    connect(m_settingsPage, &SettingsPage::backendUrlChanged, this, [this](const QString &url) {
        m_api.setBaseUrl(QUrl(url));
        saveSettings();
        statusBar()->showMessage(QStringLiteral("Backend URL updated."), 3000);
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
        statusBar()->showMessage(QStringLiteral("Local pacwallet service started."), 3000);
        refreshOverview();
    });
    connect(&m_service, &ServiceController::serviceStopped, this, [this]() {
        statusBar()->showMessage(QStringLiteral("Local pacwallet service stopped."), 3000);
    });

    m_refreshTimer.setInterval(15000);
    connect(&m_refreshTimer, &QTimer::timeout, this, &MainWindow::refreshOverview);
    m_refreshTimer.start();

    refreshOverview();
}

void MainWindow::buildUi()
{
    setWindowTitle(QStringLiteral("Pingancoin Wallet"));
    resize(1360, 860);

    auto *root = new QWidget(this);
    auto *layout = new QHBoxLayout(root);

    m_nav = new QListWidget(this);
    m_nav->addItems({
        QStringLiteral("Welcome"),
        QStringLiteral("Overview"),
        QStringLiteral("Receive"),
        QStringLiteral("Send"),
        QStringLiteral("Transactions"),
        QStringLiteral("Multisig"),
        QStringLiteral("Settings"),
    });
    m_nav->setFixedWidth(180);

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

    layout->addWidget(m_nav);
    layout->addWidget(m_stack, 1);
    setCentralWidget(root);

    connect(m_nav, &QListWidget::currentRowChanged, m_stack, &QStackedWidget::setCurrentIndex);
    m_nav->setCurrentRow(0);
    statusBar()->showMessage(QStringLiteral("Native Qt wallet ready."));
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
    QMessageBox::warning(this, QStringLiteral("Pingancoin Wallet"), QStringLiteral("%1 failed: %2").arg(operation, message));
    statusBar()->showMessage(QStringLiteral("%1 failed.").arg(operation), 4000);
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
    const QString backendUrl = settings.value(QStringLiteral("backend/url"), QStringLiteral("http://127.0.0.1:19709")).toString();
    const QString backendProgram = settings.value(QStringLiteral("backend/program"), QStringLiteral("pacwallet")).toString();
    const QStringList backendArguments = settings.value(QStringLiteral("backend/arguments"),
        QStringList{QStringLiteral("serve"), QStringLiteral("--network"), QStringLiteral("mainnet"), QStringLiteral("--rpc"), QStringLiteral("http://127.0.0.1:9509"), QStringLiteral("--listen"), QStringLiteral("127.0.0.1:19709")}).toStringList();

    m_api.setBaseUrl(QUrl(backendUrl));
    m_service.setProgram(backendProgram);
    m_service.setArguments(backendArguments);
    m_settingsPage->setBackendUrl(backendUrl);
    m_settingsPage->setBackendProgram(backendProgram);
    m_settingsPage->setBackendArguments(backendArguments);
}

void MainWindow::saveSettings() const
{
    QSettings settings(QStringLiteral("Pingancoin"), QStringLiteral("pacwallet-qt"));
    settings.setValue(QStringLiteral("backend/url"), m_api.baseUrl().toString());
    settings.setValue(QStringLiteral("backend/program"), m_service.program());
    settings.setValue(QStringLiteral("backend/arguments"), m_service.arguments());
}

} // namespace pacqt
