#include "MultisigPage.h"

#include <QFormLayout>
#include <QGroupBox>
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
    localLayout->addWidget(m_localExport);

    auto *previewBox = new QGroupBox(QStringLiteral("3-of-5 Preview"), this);
    auto *previewLayout = new QFormLayout(previewBox);
    m_requiredSpin = new QSpinBox(this);
    m_requiredSpin->setRange(1, 16);
    m_requiredSpin->setValue(3);
    m_pubKeysEdit = new QTextEdit(this);
    auto *button = new QPushButton(QStringLiteral("Preview Multisig Address"), this);
    m_addressLabel = new QLabel(this);
    m_addressLabel->setWordWrap(true);
    m_redeemLabel = new QLabel(this);
    m_redeemLabel->setWordWrap(true);

    previewLayout->addRow(QStringLiteral("Required"), m_requiredSpin);
    previewLayout->addRow(QStringLiteral("Public Keys"), m_pubKeysEdit);
    previewLayout->addRow(QString(), button);
    previewLayout->addRow(QStringLiteral("Address"), m_addressLabel);
    previewLayout->addRow(QStringLiteral("Redeem Script"), m_redeemLabel);

    layout->addWidget(localBox);
    layout->addWidget(previewBox);

    connect(button, &QPushButton::clicked, this, [this]() {
        emit previewRequested(m_requiredSpin->value(), m_pubKeysEdit->toPlainText().split('\n'));
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
    m_redeemLabel->setText(result.redeemScript);
}

} // namespace pacqt
