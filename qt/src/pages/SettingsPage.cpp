#include "SettingsPage.h"
#include "../Localization.h"

#include <QDesktopServices>
#include <QFormLayout>
#include <QGroupBox>
#include <QGridLayout>
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
    layout->setContentsMargins(0, 0, 0, 0);
    auto *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    auto *content = new QWidget(scrollArea);
    auto *contentLayout = new QGridLayout(content);
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(12);
    contentLayout->setColumnStretch(0, 1);
    contentLayout->setColumnStretch(1, 1);

    m_statusBox = new QGroupBox(QStringLiteral("Wallet Status"), this);
    auto *statusLayout = new QFormLayout(m_statusBox);
    statusLayout->setHorizontalSpacing(12);
    statusLayout->setVerticalSpacing(8);
    m_walletPathLabel = new QLabel(this);
    m_walletPathLabel->setWordWrap(true);
    m_walletStateLabel = new QLabel(this);
    m_nodeStatusLabel = new QLabel(this);
    m_activeUpstreamLabel = new QLabel(this);
    m_activeUpstreamLabel->setWordWrap(true);
    m_openWalletPathButton = new QPushButton(QStringLiteral("Open Wallet Location"), this);
    m_openBackupPathButton = new QPushButton(QStringLiteral("Open Backup Folder"), this);
    auto *statusButtons = new QHBoxLayout();
    statusButtons->setSpacing(8);
    statusButtons->addWidget(m_openWalletPathButton);
    statusButtons->addWidget(m_openBackupPathButton);
    statusButtons->addStretch(1);
    statusLayout->addRow(QStringLiteral("Wallet Path"), m_walletPathLabel);
    statusLayout->addRow(QStringLiteral("Wallet State"), m_walletStateLabel);
    statusLayout->addRow(QStringLiteral("Node"), m_nodeStatusLabel);
    statusLayout->addRow(QStringLiteral("Active Upstream"), m_activeUpstreamLabel);
    statusLayout->addRow(QString(), statusButtons);

    m_appearanceBox = new QGroupBox(QStringLiteral("Appearance"), this);
    auto *appearanceLayout = new QFormLayout(m_appearanceBox);
    appearanceLayout->setHorizontalSpacing(12);
    appearanceLayout->setVerticalSpacing(8);
    m_languageCombo = new QComboBox(this);
    auto *appearanceHint = new QLabel(QStringLiteral("Choose how the wallet interface is displayed on this Mac."), this);
    appearanceHint->setWordWrap(true);
    appearanceHint->setStyleSheet(QStringLiteral("color: #475569;"));
    appearanceLayout->addRow(appearanceHint);
    appearanceLayout->addRow(QStringLiteral("Language"), m_languageCombo);

    m_upstreamBox = new QGroupBox(QStringLiteral("RPC Upstreams"), this);
    auto *upstreamLayout = new QFormLayout(m_upstreamBox);
    upstreamLayout->setHorizontalSpacing(12);
    upstreamLayout->setVerticalSpacing(8);
    m_upstreamCombo = new QComboBox(this);
    m_activateUpstreamButton = new QPushButton(QStringLiteral("Use Selected Upstream"), this);
    m_upstreamNameEdit = new QLineEdit(this);
    m_upstreamUrlEdit = new QLineEdit(this);
    m_makeActiveCheck = new QCheckBox(QStringLiteral("Make active after adding"), this);
    m_addUpstreamButton = new QPushButton(QStringLiteral("Add Custom Upstream"), this);
    auto *knownProfilesRow = new QHBoxLayout();
    knownProfilesRow->setSpacing(8);
    knownProfilesRow->addWidget(m_upstreamCombo, 1);
    knownProfilesRow->addWidget(m_activateUpstreamButton);
    auto *upstreamActionsRow = new QHBoxLayout();
    upstreamActionsRow->setSpacing(8);
    upstreamActionsRow->addWidget(m_makeActiveCheck);
    upstreamActionsRow->addStretch(1);
    upstreamActionsRow->addWidget(m_addUpstreamButton);
    upstreamLayout->addRow(QStringLiteral("Known Profiles"), knownProfilesRow);
    upstreamLayout->addRow(QStringLiteral("Name"), m_upstreamNameEdit);
    upstreamLayout->addRow(QStringLiteral("URL"), m_upstreamUrlEdit);
    upstreamLayout->addRow(QString(), upstreamActionsRow);

    m_backendBox = new QGroupBox(QStringLiteral("Backend Connection"), this);
    auto *backendLayout = new QFormLayout(m_backendBox);
    backendLayout->setHorizontalSpacing(12);
    backendLayout->setVerticalSpacing(8);
    m_urlEdit = new QLineEdit(QStringLiteral("http://127.0.0.1:19709"), this);
    m_applyUrlButton = new QPushButton(QStringLiteral("Apply URL"), this);
    auto *backendRow = new QHBoxLayout();
    backendRow->setSpacing(8);
    backendRow->addWidget(m_urlEdit, 1);
    backendRow->addWidget(m_applyUrlButton);
    backendLayout->addRow(QStringLiteral("Wallet API URL"), backendRow);

    m_processBox = new QGroupBox(QStringLiteral("Local pacwallet Service"), this);
    auto *processLayout = new QFormLayout(m_processBox);
    processLayout->setHorizontalSpacing(12);
    processLayout->setVerticalSpacing(8);
    m_programEdit = new QLineEdit(QStringLiteral("pacwallet"), this);
    m_argumentsEdit = new QLineEdit(QStringLiteral("serve --network mainnet --rpc http://127.0.0.1:9509 --listen 127.0.0.1:19709"), this);
    m_startBackendButton = new QPushButton(QStringLiteral("Start Backend"), this);
    m_stopBackendButton = new QPushButton(QStringLiteral("Stop Backend"), this);
    auto *processButtons = new QHBoxLayout();
    processButtons->setSpacing(8);
    processButtons->addWidget(m_startBackendButton);
    processButtons->addWidget(m_stopBackendButton);
    processButtons->addStretch(1);
    processLayout->addRow(QStringLiteral("Program"), m_programEdit);
    processLayout->addRow(QStringLiteral("Arguments"), m_argumentsEdit);
    processLayout->addRow(QString(), processButtons);

    m_securityBox = new QGroupBox(QStringLiteral("Wallet Security"), this);
    auto *securityLayout = new QFormLayout(m_securityBox);
    securityLayout->setHorizontalSpacing(12);
    securityLayout->setVerticalSpacing(8);
    m_encryptPassphraseEdit = new QLineEdit(this);
    m_encryptPassphraseEdit->setEchoMode(QLineEdit::Password);
    m_encryptButton = new QPushButton(QStringLiteral("Encrypt Wallet"), this);
    m_oldPassphraseEdit = new QLineEdit(this);
    m_oldPassphraseEdit->setEchoMode(QLineEdit::Password);
    m_newPassphraseEdit = new QLineEdit(this);
    m_newPassphraseEdit->setEchoMode(QLineEdit::Password);
    m_changePassphraseButton = new QPushButton(QStringLiteral("Change Passphrase"), this);
    auto *securityButtons = new QHBoxLayout();
    securityButtons->setSpacing(8);
    securityButtons->addWidget(m_encryptButton);
    securityButtons->addWidget(m_changePassphraseButton);
    securityButtons->addStretch(1);
    securityLayout->addRow(QStringLiteral("New Passphrase"), m_encryptPassphraseEdit);
    securityLayout->addRow(QStringLiteral("Current Passphrase"), m_oldPassphraseEdit);
    securityLayout->addRow(QStringLiteral("Replacement Passphrase"), m_newPassphraseEdit);
    securityLayout->addRow(QString(), securityButtons);

    m_importBox = new QGroupBox(QStringLiteral("Import Private Key"), this);
    auto *importLayout = new QFormLayout(m_importBox);
    importLayout->setHorizontalSpacing(12);
    importLayout->setVerticalSpacing(8);
    m_importLabelEdit = new QLineEdit(this);
    m_importKeyEdit = new QLineEdit(this);
    m_importPassphraseEdit = new QLineEdit(this);
    m_importPassphraseEdit->setEchoMode(QLineEdit::Password);
    m_importButton = new QPushButton(QStringLiteral("Import Key"), this);
    auto *importButtons = new QHBoxLayout();
    importButtons->addWidget(m_importButton);
    importButtons->addStretch(1);
    importLayout->addRow(QStringLiteral("Label"), m_importLabelEdit);
    importLayout->addRow(QStringLiteral("Private Key Hex"), m_importKeyEdit);
    importLayout->addRow(QStringLiteral("Passphrase"), m_importPassphraseEdit);
    importLayout->addRow(QString(), importButtons);

    m_backupBox = new QGroupBox(QStringLiteral("Archived Backups"), this);
    auto *backupLayout = new QVBoxLayout(m_backupBox);
    m_backupsList = new QListWidget(this);
    backupLayout->addWidget(m_backupsList);

    m_logView = new QTextEdit(this);
    m_logView->setReadOnly(true);

    contentLayout->addWidget(m_statusBox, 0, 0);
    contentLayout->addWidget(m_appearanceBox, 0, 1);
    contentLayout->addWidget(m_upstreamBox, 1, 0);
    contentLayout->addWidget(m_backendBox, 1, 1);
    contentLayout->addWidget(m_processBox, 2, 0);
    contentLayout->addWidget(m_securityBox, 2, 1);
    contentLayout->addWidget(m_importBox, 3, 0);
    contentLayout->addWidget(m_backupBox, 3, 1);
    contentLayout->addWidget(m_logView, 4, 0, 1, 2);

    scrollArea->setWidget(content);
    layout->addWidget(scrollArea, 1);

    connect(m_applyUrlButton, &QPushButton::clicked, this, [this]() {
        emit backendUrlChanged(m_urlEdit->text());
    });
    connect(m_startBackendButton, &QPushButton::clicked, this, [this]() {
        emit startBackendRequested(m_programEdit->text(), m_argumentsEdit->text().split(' ', Qt::SkipEmptyParts));
    });
    connect(m_stopBackendButton, &QPushButton::clicked, this, [this]() {
        emit stopBackendRequested();
    });
    connect(m_activateUpstreamButton, &QPushButton::clicked, this, [this]() {
        const QString id = m_upstreamCombo->currentData().toString();
        if (!id.isEmpty()) {
            emit selectUpstreamRequested(id);
        }
    });
    connect(m_addUpstreamButton, &QPushButton::clicked, this, [this]() {
        emit addUpstreamRequested(m_upstreamNameEdit->text(), m_upstreamUrlEdit->text(), m_makeActiveCheck->isChecked());
    });
    connect(m_encryptButton, &QPushButton::clicked, this, [this]() {
        emit encryptWalletRequested(m_encryptPassphraseEdit->text());
    });
    connect(m_changePassphraseButton, &QPushButton::clicked, this, [this]() {
        emit changePassphraseRequested(m_oldPassphraseEdit->text(), m_newPassphraseEdit->text());
    });
    connect(m_importButton, &QPushButton::clicked, this, [this]() {
        emit importPrivateKeyRequested(m_importLabelEdit->text(), m_importKeyEdit->text(), m_importPassphraseEdit->text());
    });
    connect(m_openWalletPathButton, &QPushButton::clicked, this, [this]() {
        if (!m_walletPath.isEmpty()) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(m_walletPath));
        }
    });
    connect(m_openBackupPathButton, &QPushButton::clicked, this, [this]() {
        if (!m_backupDir.isEmpty()) {
            QDesktopServices::openUrl(QUrl::fromLocalFile(m_backupDir));
        }
    });
    connect(m_languageCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        const QString code = m_languageCombo->itemData(index).toString();
        if (!code.isEmpty() && code != m_languageCode) {
            m_languageCode = code;
            emit appLanguageChanged(code);
        }
    });
    retranslateUi();
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
    m_overview = overview;
    m_hasOverview = true;
    m_walletPath = overview.wallet.path;
    m_backupDir = overview.wallet.backupDir;
    m_walletPathLabel->setText(overview.wallet.path);
    m_walletStateLabel->setText(overview.wallet.exists
            ? (overview.wallet.encrypted ? l10n::text(QStringLiteral("Encrypted")) : l10n::text(QStringLiteral("Ready")))
            : l10n::text(QStringLiteral("No wallet created yet")));

    QString nodeText = overview.node.online
        ? l10n::text(QStringLiteral("%1 at height %2 (%3 peers, mempool %4)"))
              .arg(overview.node.network, QString::number(overview.node.bestHeight), QString::number(overview.node.peerCount), QString::number(overview.node.mempoolSize))
        : l10n::text(QStringLiteral("Offline"));
    if (!overview.node.error.isEmpty()) {
        nodeText += l10n::text(QStringLiteral(" - %1")).arg(overview.node.error);
    }
    m_nodeStatusLabel->setText(nodeText);
    m_activeUpstreamLabel->setText(QStringLiteral("%1 [%2]").arg(overview.upstream.activeUrl, overview.upstream.activeId));

    m_upstreamCombo->blockSignals(true);
    m_upstreamCombo->clear();
    for (const UpstreamProfile &profile : overview.upstream.profiles) {
        QString label = l10n::text(QStringLiteral("%1  (%2)")).arg(profile.name, profile.url);
        if (profile.id == overview.upstream.activeId) {
            label += l10n::text(QStringLiteral("  [active]"));
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
        m_backupsList->addItem(l10n::text(QStringLiteral("%1  (%2 bytes)")).arg(backup.name, QString::number(backup.sizeBytes)));
    }
}

void SettingsPage::setCurrentLanguageCode(const QString &code)
{
    const QString normalized = code.startsWith(QStringLiteral("zh")) ? QStringLiteral("zh_CN") : QStringLiteral("en");
    m_languageCode = normalized;
    m_languageCombo->blockSignals(true);
    int index = m_languageCombo->findData(normalized);
    if (index < 0) {
        index = 0;
    }
    m_languageCombo->setCurrentIndex(index);
    m_languageCombo->blockSignals(false);
}

void SettingsPage::retranslateUi()
{
    m_statusBox->setTitle(l10n::text(QStringLiteral("Wallet Status")));
    m_appearanceBox->setTitle(l10n::text(QStringLiteral("Appearance")));
    m_upstreamBox->setTitle(l10n::text(QStringLiteral("RPC Upstreams")));
    m_backendBox->setTitle(l10n::text(QStringLiteral("Backend Connection")));
    m_processBox->setTitle(l10n::text(QStringLiteral("Local pacwallet Service")));
    m_securityBox->setTitle(l10n::text(QStringLiteral("Wallet Security")));
    m_importBox->setTitle(l10n::text(QStringLiteral("Import Private Key")));
    m_backupBox->setTitle(l10n::text(QStringLiteral("Archived Backups")));

    if (auto *form = qobject_cast<QFormLayout *>(m_statusBox->layout())) {
        const QStringList labels{
            l10n::text(QStringLiteral("Wallet Path")),
            l10n::text(QStringLiteral("Wallet State")),
            l10n::text(QStringLiteral("Node")),
            l10n::text(QStringLiteral("Active Upstream"))
        };
        for (int row = 0; row < labels.size(); ++row) {
            if (auto *item = form->itemAt(row, QFormLayout::LabelRole)) {
                if (auto *label = qobject_cast<QLabel *>(item->widget())) {
                    label->setText(labels.at(row));
                }
            }
        }
    }
    if (auto *form = qobject_cast<QFormLayout *>(m_appearanceBox->layout())) {
        if (auto *item = form->itemAt(1, QFormLayout::LabelRole)) {
            if (auto *label = qobject_cast<QLabel *>(item->widget())) {
                label->setText(l10n::text(QStringLiteral("Language")));
            }
        }
        if (auto *hint = qobject_cast<QLabel *>(form->itemAt(0, QFormLayout::SpanningRole)->widget())) {
            hint->setText(l10n::text(QStringLiteral("Choose how the wallet interface is displayed on this Mac.")));
        }
    }
    if (auto *form = qobject_cast<QFormLayout *>(m_upstreamBox->layout())) {
        const QStringList labels{
            l10n::text(QStringLiteral("Known Profiles")),
            QString(),
            l10n::text(QStringLiteral("Name")),
            l10n::text(QStringLiteral("URL")),
            QString(),
            QString()
        };
        for (int row = 0; row < labels.size(); ++row) {
            if (auto *item = form->itemAt(row, QFormLayout::LabelRole)) {
                if (auto *label = qobject_cast<QLabel *>(item->widget())) {
                    label->setText(labels.at(row));
                }
            }
        }
    }
    if (auto *form = qobject_cast<QFormLayout *>(m_backendBox->layout())) {
        if (auto *item = form->itemAt(0, QFormLayout::LabelRole)) {
            if (auto *label = qobject_cast<QLabel *>(item->widget())) {
                label->setText(l10n::text(QStringLiteral("Wallet API URL")));
            }
        }
    }
    if (auto *form = qobject_cast<QFormLayout *>(m_processBox->layout())) {
        const QStringList labels{
            l10n::text(QStringLiteral("Program")),
            l10n::text(QStringLiteral("Arguments"))
        };
        for (int row = 0; row < labels.size(); ++row) {
            if (auto *item = form->itemAt(row, QFormLayout::LabelRole)) {
                if (auto *label = qobject_cast<QLabel *>(item->widget())) {
                    label->setText(labels.at(row));
                }
            }
        }
    }
    if (auto *form = qobject_cast<QFormLayout *>(m_securityBox->layout())) {
        const QStringList labels{
            l10n::text(QStringLiteral("New Passphrase")),
            QString(),
            l10n::text(QStringLiteral("Current Passphrase")),
            l10n::text(QStringLiteral("Replacement Passphrase"))
        };
        for (int row = 0; row < labels.size(); ++row) {
            if (auto *item = form->itemAt(row, QFormLayout::LabelRole)) {
                if (auto *label = qobject_cast<QLabel *>(item->widget())) {
                    label->setText(labels.at(row));
                }
            }
        }
    }
    if (auto *form = qobject_cast<QFormLayout *>(m_importBox->layout())) {
        const QStringList labels{
            l10n::text(QStringLiteral("Label")),
            l10n::text(QStringLiteral("Private Key Hex")),
            l10n::text(QStringLiteral("Passphrase"))
        };
        for (int row = 0; row < labels.size(); ++row) {
            if (auto *item = form->itemAt(row, QFormLayout::LabelRole)) {
                if (auto *label = qobject_cast<QLabel *>(item->widget())) {
                    label->setText(labels.at(row));
                }
            }
        }
    }

    const int currentIndex = m_languageCombo->currentIndex();
    m_languageCombo->blockSignals(true);
    m_languageCombo->clear();
    m_languageCombo->addItem(l10n::languageDisplayName(QStringLiteral("en")), QStringLiteral("en"));
    m_languageCombo->addItem(l10n::languageDisplayName(QStringLiteral("zh_CN")), QStringLiteral("zh_CN"));
    int languageIndex = m_languageCombo->findData(m_languageCode.isEmpty() ? QStringLiteral("en") : m_languageCode);
    if (languageIndex < 0) {
        languageIndex = currentIndex >= 0 ? currentIndex : 0;
    }
    m_languageCombo->setCurrentIndex(languageIndex);
    m_languageCombo->blockSignals(false);

    m_openWalletPathButton->setText(l10n::text(QStringLiteral("Open Wallet Location")));
    m_openBackupPathButton->setText(l10n::text(QStringLiteral("Open Backup Folder")));
    m_activateUpstreamButton->setText(l10n::text(QStringLiteral("Use Selected Upstream")));
    m_makeActiveCheck->setText(l10n::text(QStringLiteral("Make active after adding")));
    m_addUpstreamButton->setText(l10n::text(QStringLiteral("Add Custom Upstream")));
    m_applyUrlButton->setText(l10n::text(QStringLiteral("Apply URL")));
    m_startBackendButton->setText(l10n::text(QStringLiteral("Start Backend")));
    m_stopBackendButton->setText(l10n::text(QStringLiteral("Stop Backend")));
    m_encryptButton->setText(l10n::text(QStringLiteral("Encrypt Wallet")));
    m_changePassphraseButton->setText(l10n::text(QStringLiteral("Change Passphrase")));
    m_importButton->setText(l10n::text(QStringLiteral("Import Key")));
    m_upstreamNameEdit->setPlaceholderText(l10n::text(QStringLiteral("Name")));
    m_upstreamUrlEdit->setPlaceholderText(QStringLiteral("https://rpc1.pingancoin.org"));
    m_encryptPassphraseEdit->setPlaceholderText(l10n::text(QStringLiteral("New Passphrase")));
    m_oldPassphraseEdit->setPlaceholderText(l10n::text(QStringLiteral("Current Passphrase")));
    m_newPassphraseEdit->setPlaceholderText(l10n::text(QStringLiteral("Replacement Passphrase")));
    m_importLabelEdit->setPlaceholderText(l10n::text(QStringLiteral("Label")));
    m_importKeyEdit->setPlaceholderText(QStringLiteral("hex..."));
    m_importPassphraseEdit->setPlaceholderText(l10n::text(QStringLiteral("Passphrase")));
    m_logView->setPlaceholderText(l10n::text(QStringLiteral("Service Logs")));
    if (m_hasOverview) {
        setOverview(m_overview);
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
