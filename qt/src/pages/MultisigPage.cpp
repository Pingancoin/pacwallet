#include "MultisigPage.h"
#include "../Localization.h"

#include <QApplication>
#include <QClipboard>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace pacqt {

MultisigPage::MultisigPage(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    m_localBox = new QGroupBox(QStringLiteral("Local Signer Export"), this);
    auto *localLayout = new QVBoxLayout(m_localBox);
    m_localExport = new QTextEdit(this);
    m_localExport->setReadOnly(true);
    m_localExport->setMinimumHeight(160);
    auto *localButtons = new QHBoxLayout();
    auto *copyLocalButton = new QPushButton(QStringLiteral("Copy Export"), this);
    copyLocalButton->setObjectName(QStringLiteral("copyExport"));
    auto *saveLocalButton = new QPushButton(QStringLiteral("Save Export"), this);
    saveLocalButton->setObjectName(QStringLiteral("saveExport"));
    auto *useLocalKeysButton = new QPushButton(QStringLiteral("Use Local Pubkeys In Preview"), this);
    useLocalKeysButton->setObjectName(QStringLiteral("useLocalPubkeys"));
    localButtons->addWidget(copyLocalButton);
    localButtons->addWidget(saveLocalButton);
    localButtons->addWidget(useLocalKeysButton);
    auto *localHint = new QLabel(QStringLiteral("Export local signer data and preview the final 3-of-5 address before mainnet launch."), this);
    localHint->setObjectName(QStringLiteral("multisigLocalHint"));
    localHint->setWordWrap(true);
    localHint->setStyleSheet(QStringLiteral("color: #475569;"));
    localLayout->addWidget(localHint);
    localLayout->addWidget(m_localExport);
    localLayout->addLayout(localButtons);

    m_previewBox = new QGroupBox(QStringLiteral("3-of-5 Preview"), this);
    auto *previewLayout = new QFormLayout(m_previewBox);
    previewLayout->setHorizontalSpacing(12);
    previewLayout->setVerticalSpacing(8);
    m_requiredSpin = new QSpinBox(this);
    m_requiredSpin->setRange(1, 16);
    m_requiredSpin->setValue(3);
    m_pubKeysEdit = new QTextEdit(this);
    m_pubKeysEdit->setMinimumHeight(104);
    m_pubKeysEdit->setMaximumHeight(136);
    auto *button = new QPushButton(QStringLiteral("Preview Multisig Address"), this);
    button->setObjectName(QStringLiteral("previewMultisig"));
    m_addressLabel = new QLabel(this);
    m_addressLabel->setWordWrap(true);
    m_scriptHashLabel = new QLabel(this);
    m_scriptHashLabel->setWordWrap(true);
    m_redeemLabel = new QLabel(this);
    m_redeemLabel->setWordWrap(true);
    m_p2shScriptLabel = new QLabel(this);
    m_p2shScriptLabel->setWordWrap(true);
    auto *resultButtons = new QHBoxLayout();
    auto *copyAddressButton = new QPushButton(QStringLiteral("Copy Address"), this);
    copyAddressButton->setObjectName(QStringLiteral("copyAddress"));
    auto *copyScriptsButton = new QPushButton(QStringLiteral("Copy Scripts"), this);
    copyScriptsButton->setObjectName(QStringLiteral("copyScripts"));
    auto *saveResultButton = new QPushButton(QStringLiteral("Save Result"), this);
    saveResultButton->setObjectName(QStringLiteral("saveResult"));
    copyAddressButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    copyScriptsButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    saveResultButton->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    resultButtons->setSpacing(6);
    resultButtons->addWidget(copyAddressButton);
    resultButtons->addWidget(copyScriptsButton);
    resultButtons->addWidget(saveResultButton);
    resultButtons->addStretch(1);

    previewLayout->addRow(QStringLiteral("Required"), m_requiredSpin);
    previewLayout->addRow(QStringLiteral("Public Keys"), m_pubKeysEdit);
    previewLayout->addRow(QString(), button);
    previewLayout->addRow(QStringLiteral("Address"), m_addressLabel);
    previewLayout->addRow(QStringLiteral("Script Hash"), m_scriptHashLabel);
    previewLayout->addRow(QStringLiteral("Redeem Script"), m_redeemLabel);
    previewLayout->addRow(QStringLiteral("P2SH Script"), m_p2shScriptLabel);
    previewLayout->addRow(QString(), resultButtons);

    layout->addWidget(m_localBox, 1);
    layout->addWidget(m_previewBox, 0);

    connect(button, &QPushButton::clicked, this, [this]() {
        emit previewRequested(m_requiredSpin->value(), m_pubKeysEdit->toPlainText().split('\n'));
    });
    connect(copyLocalButton, &QPushButton::clicked, this, [this]() {
        QApplication::clipboard()->setText(m_localExport->toPlainText());
    });
    connect(saveLocalButton, &QPushButton::clicked, this, [this]() {
        const QString path = QFileDialog::getSaveFileName(this,
            l10n::text(QStringLiteral("Save Local Multisig Export")),
            QStringLiteral("pingancoin-local-pubkeys.txt"),
            QStringLiteral("Text Files (*.txt)"));
        if (path.isEmpty()) {
            return;
        }
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::warning(this, l10n::text(QStringLiteral("Pingancoin Wallet")), l10n::text(QStringLiteral("Could not write signer export to %1")).arg(path));
            return;
        }
        file.write(m_localExport->toPlainText().toUtf8());
    });
    connect(useLocalKeysButton, &QPushButton::clicked, this, [this]() {
        QStringList keys;
        const QStringList lines = m_localExport->toPlainText().split('\n', Qt::SkipEmptyParts);
        for (const QString &line : lines) {
            const QStringList parts = line.split(' ', Qt::SkipEmptyParts);
            if (!parts.isEmpty()) {
                keys.push_back(parts.last());
            }
        }
        m_pubKeysEdit->setPlainText(keys.join('\n'));
    });
    connect(copyAddressButton, &QPushButton::clicked, this, [this]() {
        if (!m_addressLabel->text().isEmpty()) {
            QApplication::clipboard()->setText(m_addressLabel->text());
        }
    });
    connect(copyScriptsButton, &QPushButton::clicked, this, [this]() {
        QString text;
        text += QStringLiteral("address=%1\n").arg(m_addressLabel->text());
        text += QStringLiteral("script_hash=%1\n").arg(m_scriptHashLabel->text());
        text += QStringLiteral("redeem_script=%1\n").arg(m_redeemLabel->text());
        text += QStringLiteral("p2sh_script=%1\n").arg(m_p2shScriptLabel->text());
        QApplication::clipboard()->setText(text);
    });
    connect(saveResultButton, &QPushButton::clicked, this, [this]() {
        if (m_addressLabel->text().isEmpty()) {
            QMessageBox::information(this, l10n::text(QStringLiteral("Pingancoin Wallet")), l10n::text(QStringLiteral("Generate a multisig preview first.")));
            return;
        }
        const QString path = QFileDialog::getSaveFileName(this,
            l10n::text(QStringLiteral("Save Multisig Preview")),
            QStringLiteral("pingancoin-multisig-preview.txt"),
            QStringLiteral("Text Files (*.txt)"));
        if (path.isEmpty()) {
            return;
        }
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::warning(this, l10n::text(QStringLiteral("Pingancoin Wallet")), l10n::text(QStringLiteral("Could not write multisig preview to %1")).arg(path));
            return;
        }
        QString text;
        text += QStringLiteral("address=%1\n").arg(m_addressLabel->text());
        text += QStringLiteral("script_hash=%1\n").arg(m_scriptHashLabel->text());
        text += QStringLiteral("redeem_script=%1\n").arg(m_redeemLabel->text());
        text += QStringLiteral("p2sh_script=%1\n").arg(m_p2shScriptLabel->text());
        file.write(text.toUtf8());
    });
    retranslateUi();
}

void MultisigPage::setOverview(const pacqt::Overview &overview)
{
    QString text;
    for (const KeySummary &key : overview.wallet.keys) {
        text += QStringLiteral("%1 %2 %3\n").arg(key.label, key.address, key.pubKeyHex);
    }
    m_localExport->setPlainText(text);
}

void MultisigPage::setPreviewResult(const pacqt::MultiSigPreviewResult &result)
{
    m_result = result;
    m_hasResult = true;
    m_addressLabel->setText(result.address);
    m_scriptHashLabel->setText(result.scriptHash);
    m_redeemLabel->setText(result.redeemScript);
    m_p2shScriptLabel->setText(result.p2shScript);
}

void MultisigPage::retranslateUi()
{
    m_localBox->setTitle(l10n::text(QStringLiteral("Local Signer Export")));
    m_previewBox->setTitle(l10n::text(QStringLiteral("3-of-5 Preview")));
    if (auto *hint = findChild<QLabel *>(QStringLiteral("multisigLocalHint"))) {
        hint->setText(l10n::text(QStringLiteral("Export local signer data and preview the final 3-of-5 address before mainnet launch.")));
    }
    if (auto *form = qobject_cast<QFormLayout *>(m_previewBox->layout())) {
        const QStringList rowLabels{
            l10n::text(QStringLiteral("Required")),
            l10n::text(QStringLiteral("Public Keys")),
            QString(),
            l10n::text(QStringLiteral("Address")),
            l10n::text(QStringLiteral("Script Hash")),
            l10n::text(QStringLiteral("Redeem Script")),
            l10n::text(QStringLiteral("P2SH Script"))
        };
        for (int row = 0; row < rowLabels.size(); ++row) {
            if (auto *item = form->itemAt(row, QFormLayout::LabelRole)) {
                if (auto *label = qobject_cast<QLabel *>(item->widget())) {
                    label->setText(rowLabels.at(row));
                }
            }
        }
    }
    const auto buttons = findChildren<QPushButton *>();
    for (QPushButton *button : buttons) {
        if (button->text() == QStringLiteral("Copy Export") || button->objectName() == QStringLiteral("copyExport")) {
            button->setText(l10n::text(QStringLiteral("Copy Export")));
            button->setObjectName(QStringLiteral("copyExport"));
        } else if (button->text() == QStringLiteral("Save Export") || button->objectName() == QStringLiteral("saveExport")) {
            button->setText(l10n::text(QStringLiteral("Save Export")));
            button->setObjectName(QStringLiteral("saveExport"));
        } else if (button->text() == QStringLiteral("Use Local Pubkeys In Preview") || button->objectName() == QStringLiteral("useLocalPubkeys")) {
            button->setText(l10n::text(QStringLiteral("Use Local Pubkeys In Preview")));
            button->setObjectName(QStringLiteral("useLocalPubkeys"));
        } else if (button->text() == QStringLiteral("Preview Multisig Address") || button->objectName() == QStringLiteral("previewMultisig")) {
            button->setText(l10n::text(QStringLiteral("Preview Multisig Address")));
            button->setObjectName(QStringLiteral("previewMultisig"));
        } else if (button->text() == QStringLiteral("Copy Address") || button->objectName() == QStringLiteral("copyAddress")) {
            button->setText(l10n::text(QStringLiteral("Copy Address")));
            button->setObjectName(QStringLiteral("copyAddress"));
        } else if (button->text() == QStringLiteral("Copy Scripts") || button->objectName() == QStringLiteral("copyScripts")) {
            button->setText(l10n::text(QStringLiteral("Copy Scripts")));
            button->setObjectName(QStringLiteral("copyScripts"));
        } else if (button->text() == QStringLiteral("Save Result") || button->objectName() == QStringLiteral("saveResult")) {
            button->setText(l10n::text(QStringLiteral("Save Result")));
            button->setObjectName(QStringLiteral("saveResult"));
        }
    }
    m_pubKeysEdit->setPlaceholderText(QStringLiteral("02...\n03...\n02..."));
    if (m_hasResult) {
        setPreviewResult(m_result);
    }
}

} // namespace pacqt
