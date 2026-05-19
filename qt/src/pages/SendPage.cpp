#include "SendPage.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace pacqt {

SendPage::SendPage(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    auto *box = new QGroupBox(QStringLiteral("Send PAC"), this);
    auto *form = new QFormLayout(box);

    m_balanceLabel = new QLabel(QStringLiteral("Spendable balance will appear after wallet sync."), this);
    m_toEdit = new QLineEdit(this);
    m_amountEdit = new QLineEdit(this);
    m_feeEdit = new QLineEdit(QStringLiteral("0.0001"), this);
    m_changeCombo = new QComboBox(this);
    m_changeCombo->addItem(QStringLiteral("Automatic wallet change address"), QString());
    m_passphraseEdit = new QLineEdit(this);
    m_passphraseEdit->setEchoMode(QLineEdit::Password);
    auto *sendButton = new QPushButton(QStringLiteral("Send Transaction"), this);
    auto *maxButton = new QPushButton(QStringLiteral("Use Max Spendable"), this);

    form->addRow(QStringLiteral("Spendable"), m_balanceLabel);
    form->addRow(QStringLiteral("Destination"), m_toEdit);
    form->addRow(QStringLiteral("Amount"), m_amountEdit);
    form->addRow(QStringLiteral("Fee"), m_feeEdit);
    form->addRow(QStringLiteral("Change Address"), m_changeCombo);
    form->addRow(QStringLiteral("Passphrase"), m_passphraseEdit);
    form->addRow(QString(), maxButton);
    form->addRow(QString(), sendButton);

    layout->addWidget(box);
    layout->addStretch(1);

    connect(maxButton, &QPushButton::clicked, this, [this]() {
        const QString spendableText = m_balanceLabel->property("spendable_text").toString();
        if (!spendableText.isEmpty()) {
            m_amountEdit->setText(spendableText);
        }
    });
    connect(sendButton, &QPushButton::clicked, this, [this]() {
        const QString to = m_toEdit->text().trimmed();
        const QString amount = m_amountEdit->text().trimmed();
        const QString fee = m_feeEdit->text().trimmed();
        const QString passphrase = m_passphraseEdit->text();
        if (to.isEmpty() || amount.isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("Pingancoin Wallet"), QStringLiteral("Destination and amount are required."));
            return;
        }
        const QString confirmText = QStringLiteral(
            "Send %1 PAC\n\nTo: %2\nFee: %3 PAC\nChange: %4")
                .arg(amount,
                     to,
                     fee.isEmpty() ? QStringLiteral("0.0001") : fee,
                     m_changeCombo->currentData().toString().isEmpty() ? QStringLiteral("Automatic") : m_changeCombo->currentData().toString());
        const auto confirm = QMessageBox::question(this, QStringLiteral("Confirm Transaction"), confirmText);
        if (confirm != QMessageBox::Yes) {
            return;
        }
        emit sendRequested(to, amount, fee, m_changeCombo->currentData().toString(), passphrase);
    });
}

void SendPage::setOverview(const pacqt::Overview &overview)
{
    m_keys = overview.wallet.keys;
    const QString spendable = formatPac(overview.balance.spendable);
    m_balanceLabel->setText(QStringLiteral("%1 PAC").arg(spendable));
    m_balanceLabel->setProperty("spendable_text", spendable);

    const QString current = m_changeCombo->currentData().toString();
    m_changeCombo->blockSignals(true);
    m_changeCombo->clear();
    m_changeCombo->addItem(QStringLiteral("Automatic wallet change address"), QString());
    for (const KeySummary &key : m_keys) {
        m_changeCombo->addItem(QStringLiteral("%1 - %2").arg(key.label, key.address), key.address);
    }
    const int index = m_changeCombo->findData(current);
    if (index >= 0) {
        m_changeCombo->setCurrentIndex(index);
    }
    m_changeCombo->blockSignals(false);
}

} // namespace pacqt
