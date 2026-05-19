#include "SendPage.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QPushButton>
#include <QVBoxLayout>

namespace pacqt {

SendPage::SendPage(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    auto *box = new QGroupBox(QStringLiteral("Send PAC"), this);
    auto *form = new QFormLayout(box);

    m_toEdit = new QLineEdit(this);
    m_amountEdit = new QLineEdit(this);
    m_feeEdit = new QLineEdit(QStringLiteral("0.0001"), this);
    m_changeEdit = new QLineEdit(this);
    m_passphraseEdit = new QLineEdit(this);
    m_passphraseEdit->setEchoMode(QLineEdit::Password);
    auto *sendButton = new QPushButton(QStringLiteral("Send Transaction"), this);

    form->addRow(QStringLiteral("Destination"), m_toEdit);
    form->addRow(QStringLiteral("Amount"), m_amountEdit);
    form->addRow(QStringLiteral("Fee"), m_feeEdit);
    form->addRow(QStringLiteral("Change Address"), m_changeEdit);
    form->addRow(QStringLiteral("Passphrase"), m_passphraseEdit);
    form->addRow(QString(), sendButton);

    layout->addWidget(box);
    layout->addStretch(1);

    connect(sendButton, &QPushButton::clicked, this, [this]() {
        emit sendRequested(m_toEdit->text(), m_amountEdit->text(), m_feeEdit->text(), m_changeEdit->text(), m_passphraseEdit->text());
    });
}

} // namespace pacqt
