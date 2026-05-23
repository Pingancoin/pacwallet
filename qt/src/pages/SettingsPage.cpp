#include "SettingsPage.h"
#include "../Localization.h"

#include <QDesktopServices>
#include <QFormLayout>
#include <QGroupBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QScrollArea>
#include <QSizePolicy>
#include <QUrl>
#include <QVBoxLayout>

#ifndef PACWALLET_QT_VERSION
#define PACWALLET_QT_VERSION "dev"
#endif

namespace pacqt {

namespace {

QString displayVersion()
{
    const QString version = QString::fromLatin1(PACWALLET_QT_VERSION).trimmed();
    if (version.isEmpty()) {
        return QStringLiteral("dev");
    }
    return version.startsWith(QLatin1Char('v')) ? version : QStringLiteral("v%1").arg(version);
}

} // namespace

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
    statusLayout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    statusLayout->setRowWrapPolicy(QFormLayout::WrapLongRows);
    m_versionLabel = new QLabel(displayVersion(), this);
    m_walletPathLabel = new QLabel(this);
    m_walletPathLabel->setWordWrap(true);
    m_walletPathLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_walletStateLabel = new QLabel(this);
    m_nodeStatusLabel = new QLabel(this);
    m_nodeStatusLabel->setWordWrap(true);
    m_nodeStatusLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_openWalletPathButton = new QPushButton(QStringLiteral("Open Wallet Location"), this);
    m_openBackupPathButton = new QPushButton(QStringLiteral("Open Backup Folder"), this);
    auto *statusButtons = new QHBoxLayout();
    statusButtons->setSpacing(8);
    statusButtons->addWidget(m_openWalletPathButton);
    statusButtons->addWidget(m_openBackupPathButton);
    statusButtons->addStretch(1);
    statusLayout->addRow(QStringLiteral("Wallet Version"), m_versionLabel);
    statusLayout->addRow(QStringLiteral("Wallet Path"), m_walletPathLabel);
    statusLayout->addRow(QStringLiteral("Wallet State"), m_walletStateLabel);
    statusLayout->addRow(QStringLiteral("Node"), m_nodeStatusLabel);
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
    m_backupsList->setMinimumHeight(128);
    backupLayout->addWidget(m_backupsList);

    contentLayout->addWidget(m_statusBox, 0, 0);
    contentLayout->addWidget(m_appearanceBox, 0, 1);
    contentLayout->addWidget(m_securityBox, 1, 0);
    contentLayout->addWidget(m_importBox, 1, 1);
    contentLayout->addWidget(m_backupBox, 2, 0, 1, 2);
    contentLayout->setRowStretch(3, 1);

    scrollArea->setWidget(content);
    layout->addWidget(scrollArea, 1);

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
    m_statusBox->setTitle(l10n::text(QStringLiteral("Wallet Details")));
    m_appearanceBox->setTitle(l10n::text(QStringLiteral("Appearance")));
    m_securityBox->setTitle(l10n::text(QStringLiteral("Wallet Security")));
    m_importBox->setTitle(l10n::text(QStringLiteral("Import Private Key")));
    m_backupBox->setTitle(l10n::text(QStringLiteral("Archived Backups")));

    if (auto *form = qobject_cast<QFormLayout *>(m_statusBox->layout())) {
        const QStringList labels{
            l10n::text(QStringLiteral("Wallet Version")),
            l10n::text(QStringLiteral("Wallet Path")),
            l10n::text(QStringLiteral("Wallet State")),
            l10n::text(QStringLiteral("Node"))
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
    if (auto *form = qobject_cast<QFormLayout *>(m_securityBox->layout())) {
        const QStringList labels{
            l10n::text(QStringLiteral("New Passphrase")),
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
    m_encryptButton->setText(l10n::text(QStringLiteral("Encrypt Wallet")));
    m_changePassphraseButton->setText(l10n::text(QStringLiteral("Change Passphrase")));
    m_importButton->setText(l10n::text(QStringLiteral("Import Key")));
    m_encryptPassphraseEdit->setPlaceholderText(l10n::text(QStringLiteral("New Passphrase")));
    m_oldPassphraseEdit->setPlaceholderText(l10n::text(QStringLiteral("Current Passphrase")));
    m_newPassphraseEdit->setPlaceholderText(l10n::text(QStringLiteral("Replacement Passphrase")));
    m_importLabelEdit->setPlaceholderText(l10n::text(QStringLiteral("Label")));
    m_importKeyEdit->setPlaceholderText(QStringLiteral("hex..."));
    m_importPassphraseEdit->setPlaceholderText(l10n::text(QStringLiteral("Passphrase")));
    if (m_hasOverview) {
        setOverview(m_overview);
    }
}

void SettingsPage::appendLog(const QString &line)
{
    Q_UNUSED(line);
}

} // namespace pacqt
