#include "SettingsPage.h"

#include <QDesktopServices>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QPushButton>
#include <QScrollArea>
#include <QUrl>
#include <QVBoxLayout>

namespace pacqt {

SettingsPage::SettingsPage(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    auto *content = new QWidget(scrollArea);
    auto *contentLayout = new QVBoxLayout(content);

    auto *statusBox = new QGroupBox(QStringLiteral("Wallet Status"), this);
    auto *statusLayout = new QFormLayout(statusBox);
    m_walletPathLabel = new QLabel(this);
    m_walletPathLabel->setWordWrap(true);
    m_walletStateLabel = new QLabel(this);
    m_nodeStatusLabel = new QLabel(this);
    m_activeUpstreamLabel = new QLabel(this);
    m_activeUpstreamLabel->setWordWrap(true);
    auto *openWalletPathButton = new QPushButton(QStringLiteral("Open Wallet Location"), this);
    auto *openBackupPathButton = new QPushButton(QStringLiteral("Open Backup Folder"), this);
    statusLayout->addRow(QStringLiteral("Wallet Path"), m_walletPathLabel);
    statusLayout->addRow(QStringLiteral("Wallet State"), m_walletStateLabel);
    statusLayout->addRow(QStringLiteral("Node"), m_nodeStatusLabel);
    statusLayout->addRow(QStringLiteral("Active Upstream"), m_activeUpstreamLabel);
    statusLayout->addRow(QString(), openWalletPathButton);
    statusLayout->addRow(QString(), openBackupPathButton);

    auto *upstreamBox = new QGroupBox(QStringLiteral("RPC Upstreams"), this);
    auto *upstreamLayout = new QFormLayout(upstreamBox);
    m_upstreamCombo = new QComboBox(this);
    auto *activateButton = new QPushButton(QStringLiteral("Use Selected Upstream"), this);
    m_upstreamNameEdit = new QLineEdit(this);
    m_upstreamUrlEdit = new QLineEdit(this);
    m_makeActiveCheck = new QCheckBox(QStringLiteral("Make active after adding"), this);
    auto *addUpstreamButton = new QPushButton(QStringLiteral("Add Custom Upstream"), this);
    upstreamLayout->addRow(QStringLiteral("Known Profiles"), m_upstreamCombo);
    upstreamLayout->addRow(QString(), activateButton);
    upstreamLayout->addRow(QStringLiteral("Name"), m_upstreamNameEdit);
    upstreamLayout->addRow(QStringLiteral("URL"), m_upstreamUrlEdit);
    upstreamLayout->addRow(QString(), m_makeActiveCheck);
    upstreamLayout->addRow(QString(), addUpstreamButton);

    auto *backendBox = new QGroupBox(QStringLiteral("Backend Connection"), this);
    auto *backendLayout = new QFormLayout(backendBox);
    m_urlEdit = new QLineEdit(QStringLiteral("http://127.0.0.1:19709"), this);
    auto *applyButton = new QPushButton(QStringLiteral("Apply URL"), this);
    backendLayout->addRow(QStringLiteral("Wallet API URL"), m_urlEdit);
    backendLayout->addRow(QString(), applyButton);

    auto *processBox = new QGroupBox(QStringLiteral("Local pacwallet Service"), this);
    auto *processLayout = new QFormLayout(processBox);
    m_programEdit = new QLineEdit(QStringLiteral("pacwallet"), this);
    m_argumentsEdit = new QLineEdit(QStringLiteral("serve --network mainnet --rpc http://127.0.0.1:9509 --listen 127.0.0.1:19709"), this);
    auto *startButton = new QPushButton(QStringLiteral("Start Backend"), this);
    auto *stopButton = new QPushButton(QStringLiteral("Stop Backend"), this);
    processLayout->addRow(QStringLiteral("Program"), m_programEdit);
    processLayout->addRow(QStringLiteral("Arguments"), m_argumentsEdit);
    processLayout->addRow(QString(), startButton);
    processLayout->addRow(QString(), stopButton);

    auto *securityBox = new QGroupBox(QStringLiteral("Wallet Security"), this);
    auto *securityLayout = new QFormLayout(securityBox);
    m_encryptPassphraseEdit = new QLineEdit(this);
    m_encryptPassphraseEdit->setEchoMode(QLineEdit::Password);
    auto *encryptButton = new QPushButton(QStringLiteral("Encrypt Wallet"), this);
    m_oldPassphraseEdit = new QLineEdit(this);
    m_oldPassphraseEdit->setEchoMode(QLineEdit::Password);
    m_newPassphraseEdit = new QLineEdit(this);
    m_newPassphraseEdit->setEchoMode(QLineEdit::Password);
    auto *changePassphraseButton = new QPushButton(QStringLiteral("Change Passphrase"), this);
    securityLayout->addRow(QStringLiteral("New Passphrase"), m_encryptPassphraseEdit);
    securityLayout->addRow(QString(), encryptButton);
    securityLayout->addRow(QStringLiteral("Current Passphrase"), m_oldPassphraseEdit);
    securityLayout->addRow(QStringLiteral("Replacement Passphrase"), m_newPassphraseEdit);
    securityLayout->addRow(QString(), changePassphraseButton);

    auto *importBox = new QGroupBox(QStringLiteral("Import Private Key"), this);
    auto *importLayout = new QFormLayout(importBox);
    m_importLabelEdit = new QLineEdit(this);
    m_importKeyEdit = new QLineEdit(this);
    m_importPassphraseEdit = new QLineEdit(this);
    m_importPassphraseEdit->setEchoMode(QLineEdit::Password);
    auto *importButton = new QPushButton(QStringLiteral("Import Key"), this);
    importLayout->addRow(QStringLiteral("Label"), m_importLabelEdit);
    importLayout->addRow(QStringLiteral("Private Key Hex"), m_importKeyEdit);
    importLayout->addRow(QStringLiteral("Passphrase"), m_importPassphraseEdit);
    importLayout->addRow(QString(), importButton);

    auto *backupBox = new QGroupBox(QStringLiteral("Archived Backups"), this);
    auto *backupLayout = new QVBoxLayout(backupBox);
    m_backupsList = new QListWidget(this);
    backupLayout->addWidget(m_backupsList);

    m_logView = new QTextEdit(this);
    m_logView->setReadOnly(true);

    contentLayout->addWidget(statusBox);
    contentLayout->addWidget(upstreamBox);
    contentLayout->addWidget(backendBox);
    contentLayout->addWidget(processBox);
    contentLayout->addWidget(securityBox);
    contentLayout->addWidget(importBox);
    contentLayout->addWidget(backupBox);
    contentLayout->addWidget(m_logView, 1);
    contentLayout->addStretch(1);

    scrollArea->setWidget(content);
    layout->addWidget(scrollArea, 1);

    connect(applyButton, &QPushButton::clicked, this, [this]() {
        emit backendUrlChanged(m_urlEdit->text());
    });
    connect(startButton, &QPushButton::clicked, this, [this]() {
        emit startBackendRequested(m_programEdit->text(), m_argumentsEdit->text().split(' ', Qt::SkipEmptyParts));
    });
    connect(stopButton, &QPushButton::clicked, this, [this]() {
        emit stopBackendRequested();
    });
    connect(activateButton, &QPushButton::clicked, this, [this]() {
        const QString id = m_upstreamCombo->currentData().toString();
        if (!id.isEmpty()) {
            emit selectUpstreamRequested(id);
        }
    });
    connect(addUpstreamButton, &QPushButton::clicked, this, [this]() {
        emit addUpstreamRequested(m_upstreamNameEdit->text(), m_upstreamUrlEdit->text(), m_makeActiveCheck->isChecked());
    });
    connect(encryptButton, &QPushButton::clicked, this, [this]() {
        emit encryptWalletRequested(m_encryptPassphraseEdit->text());
    });
    connect(changePassphraseButton, &QPushButton::clicked, this, [this]() {
        emit changePassphraseRequested(m_oldPassphraseEdit->text(), m_newPassphraseEdit->text());
    });
    connect(importButton, &QPushButton::clicked, this, [this]() {
        emit importPrivateKeyRequested(m_importLabelEdit->text(), m_importKeyEdit->text(), m_importPassphraseEdit->text());
    });
    connect(openWalletPathButton, &QPushButton::clicked, this, [this]() {
        if (!m_walletPath.isEmpty()) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(m_walletPath));
        }
    });
    connect(openBackupPathButton, &QPushButton::clicked, this, [this]() {
        if (!m_backupDir.isEmpty()) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(m_backupDir));
        }
    });
}

void SettingsPage::setBackendUrl(const QString &url)
{
    m_urlEdit->setText(url);
}

void SettingsPage::setBackendProgram(const QString &program)
{
    m_programEdit->setText(program);
}

void SettingsPage::setBackendArguments(const QStringList &arguments)
{
    m_argumentsEdit->setText(arguments.join(' '));
}

void SettingsPage::setOverview(const pacqt::Overview &overview)
{
    m_walletPath = overview.wallet.path;
    m_backupDir = overview.wallet.backupDir;
    m_walletPathLabel->setText(overview.wallet.path);
    m_walletStateLabel->setText(overview.wallet.exists
            ? (overview.wallet.encrypted ? QStringLiteral("Encrypted") : QStringLiteral("Ready"))
            : QStringLiteral("No wallet created yet"));

    QString nodeText = overview.node.online
        ? QStringLiteral("%1 at height %2 (%3 peers, mempool %4)")
              .arg(overview.node.network, QString::number(overview.node.bestHeight), QString::number(overview.node.peerCount), QString::number(overview.node.mempoolSize))
        : QStringLiteral("Offline");
    if (!overview.node.error.isEmpty()) {
        nodeText += QStringLiteral(" - %1").arg(overview.node.error);
    }
    m_nodeStatusLabel->setText(nodeText);
    m_activeUpstreamLabel->setText(QStringLiteral("%1 [%2]").arg(overview.upstream.activeUrl, overview.upstream.activeId));

    m_upstreamCombo->blockSignals(true);
    m_upstreamCombo->clear();
    for (const UpstreamProfile &profile : overview.upstream.profiles) {
        QString label = QStringLiteral("%1  (%2)").arg(profile.name, profile.url);
        if (profile.id == overview.upstream.activeId) {
            label += QStringLiteral("  [active]");
        }
        m_upstreamCombo->addItem(label, profile.id);
    }
    const int activeIndex = m_upstreamCombo->findData(overview.upstream.activeId);
    if (activeIndex >= 0) {
        m_upstreamCombo->setCurrentIndex(activeIndex);
    }
    m_upstreamCombo->blockSignals(false);

    m_backupsList->clear();
    for (const BackupInfo &backup : overview.wallet.backups) {
        m_backupsList->addItem(QStringLiteral("%1  (%2 bytes)").arg(backup.name, QString::number(backup.sizeBytes)));
    }
}

QString SettingsPage::backendUrl() const
{
    return m_urlEdit->text();
}

QString SettingsPage::backendProgram() const
{
    return m_programEdit->text();
}

QStringList SettingsPage::backendArguments() const
{
    return m_argumentsEdit->text().split(' ', Qt::SkipEmptyParts);
}

void SettingsPage::appendLog(const QString &line)
{
    m_logView->append(line.trimmed());
}

} // namespace pacqt
