#include "SendPage.h"
#include "../Localization.h"

#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace pacqt {

SendPage::SendPage(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    auto *contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(12);

    m_box = new QGroupBox(QStringLiteral("Send PAC"), this);
    auto *form = new QFormLayout(m_box);
    form->setHorizontalSpacing(12);
    form->setVerticalSpacing(8);

    m_balanceLabel = new QLabel(QStringLiteral("Spendable balance will appear after wallet sync."), this);
    m_toEdit = new QLineEdit(this);
    m_amountEdit = new QLineEdit(this);
    m_feeEdit = new QLineEdit(QStringLiteral("0.0001"), this);
    m_changeCombo = new QComboBox(this);
    m_changeCombo->addItem(QStringLiteral("Automatic wallet change address"), QString());
    m_passphraseEdit = new QLineEdit(this);
    m_passphraseEdit->setEchoMode(QLineEdit::Password);
    m_sendButton = new QPushButton(QStringLiteral("Send Transaction"), this);
    m_maxButton = new QPushButton(QStringLiteral("Use Max Spendable"), this);

    form->addRow(QStringLiteral("Spendable"), m_balanceLabel);
    form->addRow(QStringLiteral("Destination"), m_toEdit);
    form->addRow(QStringLiteral("Amount"), m_amountEdit);
    form->addRow(QStringLiteral("Fee"), m_feeEdit);
    form->addRow(QStringLiteral("Change Address"), m_changeCombo);
    form->addRow(QStringLiteral("Passphrase"), m_passphraseEdit);
    form->addRow(QString(), m_maxButton);
    form->addRow(QString(), m_sendButton);

    auto *noteBox = new QGroupBox(QStringLiteral("Need a cleaner send flow?"), this);
    noteBox->setObjectName(QStringLiteral("sendHintBox"));
    auto *noteLayout = new QVBoxLayout(noteBox);
    auto *noteTitle = new QLabel(QStringLiteral("Review the destination, fee, and change output before broadcasting."), this);
    noteTitle->setObjectName(QStringLiteral("sendHintTitle"));
    noteTitle->setWordWrap(true);
    auto *noteText = new QLabel(QStringLiteral("Coinbase rewards stay immature for a while before they become spendable."), this);
    noteText->setObjectName(QStringLiteral("sendHintText"));
    noteText->setWordWrap(true);
    noteText->setStyleSheet(QStringLiteral("color: #475569;"));
    noteLayout->addWidget(noteTitle);
    noteLayout->addWidget(noteText);
    noteLayout->addStretch(1);

    contentLayout->addWidget(m_box, 3);
    contentLayout->addWidget(noteBox, 2);
    layout->addLayout(contentLayout);

    connect(m_maxButton, &QPushButton::clicked, this, [this]() {
        const QString spendableText = m_balanceLabel->property("spendable_text").toString();
        if (!spendableText.isEmpty()) {
            m_amountEdit->setText(spendableText);
        }
    });
    connect(m_sendButton, &QPushButton::clicked, this, [this]() {
        const QString to = m_toEdit->text().trimmed();
        const QString amount = m_amountEdit->text().trimmed();
        const QString fee = m_feeEdit->text().trimmed();
        const QString passphrase = m_passphraseEdit->text();
        if (to.isEmpty() || amount.isEmpty()) {
            QMessageBox::warning(this, l10n::text(QStringLiteral("Pingancoin Wallet")), l10n::text(QStringLiteral("Destination and amount are required.")));
            return;
        }
        const QString changeLabel = m_changeCombo->currentData().toString().isEmpty()
            ? l10n::text(QStringLiteral("Automatic"))
            : m_changeCombo->currentData().toString();
        const QString confirmTemplate = l10n::text(QStringLiteral("Send %1 PAC\n\nTo: %2\nFee: %3 PAC\nChange: %4"));
        const QString confirmText = confirmTemplate.arg(
            amount,
            to,
            fee.isEmpty() ? QStringLiteral("0.0001") : fee,
            changeLabel);
        const auto confirm = QMessageBox::question(this, l10n::text(QStringLiteral("Confirm Transaction")), confirmText);
        if (confirm != QMessageBox::Yes) {
            return;
        }
        emit sendRequested(to, amount, fee, m_changeCombo->currentData().toString(), passphrase);
    });
    retranslateUi();
}

void SendPage::setOverview(const pacqt::Overview &overview)
{
    m_overview = overview;
    m_hasOverview = true;
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

void SendPage::retranslateUi()
{
    m_box->setTitle(l10n::text(QStringLiteral("Send PAC")));
    if (auto *hintBox = findChild<QGroupBox *>(QStringLiteral("sendHintBox"))) {
        hintBox->setTitle(l10n::text(QStringLiteral("Need a cleaner send flow?")));
    }
    if (auto *hintTitle = findChild<QLabel *>(QStringLiteral("sendHintTitle"))) {
        hintTitle->setText(l10n::text(QStringLiteral("Review the destination, fee, and change output before broadcasting.")));
    }
    if (auto *hintText = findChild<QLabel *>(QStringLiteral("sendHintText"))) {
        hintText->setText(l10n::text(QStringLiteral("Coinbase rewards stay immature for a while before they become spendable.")));
    }
    const QStringList rowLabels{
        l10n::text(QStringLiteral("Spendable")),
        l10n::text(QStringLiteral("Destination")),
        l10n::text(QStringLiteral("Amount")),
        l10n::text(QStringLiteral("Fee")),
        l10n::text(QStringLiteral("Change Address")),
        l10n::text(QStringLiteral("Passphrase"))
    };
    if (auto *form = qobject_cast<QFormLayout *>(m_box->layout())) {
        for (int row = 0; row < rowLabels.size(); ++row) {
            if (auto *item = form->itemAt(row, QFormLayout::LabelRole)) {
                if (auto *label = qobject_cast<QLabel *>(item->widget())) {
                    label->setText(rowLabels.at(row));
                }
            }
        }
    }
    m_balanceLabel->setToolTip(l10n::text(QStringLiteral("Coinbase rewards stay immature for a while before they become spendable.")));
    if (!m_hasOverview) {
        m_balanceLabel->setText(l10n::text(QStringLiteral("Spendable balance will appear after wallet sync.")));
    }
    m_toEdit->setPlaceholderText(QStringLiteral("P..."));
    m_amountEdit->setPlaceholderText(QStringLiteral("0.10000000"));
    m_feeEdit->setPlaceholderText(QStringLiteral("0.0001"));
    m_passphraseEdit->setPlaceholderText(l10n::text(QStringLiteral("Passphrase")));
    m_sendButton->setText(l10n::text(QStringLiteral("Send Transaction")));
    m_maxButton->setText(l10n::text(QStringLiteral("Use Max Spendable")));
    if (m_hasOverview) {
        setOverview(m_overview);
    }
}

} // namespace pacqt
