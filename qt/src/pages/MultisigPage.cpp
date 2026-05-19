#include "MultisigPage.h"

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

    auto *localBox = new QGroupBox(QStringLiteral("Local Signer Export"), this);
    auto *localLayout = new QVBoxLayout(localBox);
    m_localExport = new QTextEdit(this);
    m_localExport->setReadOnly(true);
    auto *localButtons = new QHBoxLayout();
    auto *copyLocalButton = new QPushButton(QStringLiteral("Copy Export"), this);
    auto *saveLocalButton = new QPushButton(QStringLiteral("Save Export"), this);
    auto *useLocalKeysButton = new QPushButton(QStringLiteral("Use Local Pubkeys In Preview"), this);
    localButtons->addWidget(copyLocalButton);
    localButtons->addWidget(saveLocalButton);
    localButtons->addWidget(useLocalKeysButton);
    localLayout->addWidget(m_localExport);
    localLayout->addLayout(localButtons);

    auto *previewBox = new QGroupBox(QStringLiteral("3-of-5 Preview"), this);
    auto *previewLayout = new QFormLayout(previewBox);
    m_requiredSpin = new QSpinBox(this);
    m_requiredSpin->setRange(1, 16);
    m_requiredSpin->setValue(3);
    m_pubKeysEdit = new QTextEdit(this);
    auto *button = new QPushButton(QStringLiteral("Preview Multisig Address"), this);
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
    auto *copyScriptsButton = new QPushButton(QStringLiteral("Copy Scripts"), this);
    auto *saveResultButton = new QPushButton(QStringLiteral("Save Result"), this);
    resultButtons->addWidget(copyAddressButton);
    resultButtons->addWidget(copyScriptsButton);
    resultButtons->addWidget(saveResultButton);

    previewLayout->addRow(QStringLiteral("Required"), m_requiredSpin);
    previewLayout->addRow(QStringLiteral("Public Keys"), m_pubKeysEdit);
    previewLayout->addRow(QString(), button);
    previewLayout->addRow(QStringLiteral("Address"), m_addressLabel);
    previewLayout->addRow(QStringLiteral("Script Hash"), m_scriptHashLabel);
    previewLayout->addRow(QStringLiteral("Redeem Script"), m_redeemLabel);
    previewLayout->addRow(QStringLiteral("P2SH Script"), m_p2shScriptLabel);
    previewLayout->addRow(QString(), resultButtons);

    layout->addWidget(localBox);
    layout->addWidget(previewBox);

    connect(button, &QPushButton::clicked, this, [this]() {
        emit previewRequested(m_requiredSpin->value(), m_pubKeysEdit->toPlainText().split('\n'));
    });
    connect(copyLocalButton, &QPushButton::clicked, this, [this]() {
        QApplication::clipboard()->setText(m_localExport->toPlainText());
    });
    connect(saveLocalButton, &QPushButton::clicked, this, [this]() {
        const QString path = QFileDialog::getSaveFileName(this,
            QStringLiteral("Save Local Multisig Export"),
            QStringLiteral("pingancoin-local-pubkeys.txt"),
            QStringLiteral("Text Files (*.txt)"));
        if (path.isEmpty()) {
            return;
        }
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::warning(this, QStringLiteral("Pingancoin Wallet"), QStringLiteral("Could not write signer export to %1").arg(path));
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
            QMessageBox::information(this, QStringLiteral("Pingancoin Wallet"), QStringLiteral("Generate a multisig preview first."));
            return;
        }
        const QString path = QFileDialog::getSaveFileName(this,
            QStringLiteral("Save Multisig Preview"),
            QStringLiteral("pingancoin-multisig-preview.txt"),
            QStringLiteral("Text Files (*.txt)"));
        if (path.isEmpty()) {
            return;
        }
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::warning(this, QStringLiteral("Pingancoin Wallet"), QStringLiteral("Could not write multisig preview to %1").arg(path));
            return;
        }
        QString text;
        text += QStringLiteral("address=%1\n").arg(m_addressLabel->text());
        text += QStringLiteral("script_hash=%1\n").arg(m_scriptHashLabel->text());
        text += QStringLiteral("redeem_script=%1\n").arg(m_redeemLabel->text());
        text += QStringLiteral("p2sh_script=%1\n").arg(m_p2shScriptLabel->text());
        file.write(text.toUtf8());
    });
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
    m_addressLabel->setText(result.address);
    m_scriptHashLabel->setText(result.scriptHash);
    m_redeemLabel->setText(result.redeemScript);
    m_p2shScriptLabel->setText(result.p2shScript);
}

} // namespace pacqt
