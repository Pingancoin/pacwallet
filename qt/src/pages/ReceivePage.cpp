#include "ReceivePage.h"

#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QPixmap>
#include <QVBoxLayout>

namespace pacqt {

ReceivePage::ReceivePage(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);

    auto *topBox = new QGroupBox(QStringLiteral("Receive"), this);
    auto *topLayout = new QGridLayout(topBox);

    m_addressCombo = new QComboBox(this);
    m_qrLabel = new QLabel(this);
    m_qrLabel->setMinimumSize(220, 220);
    m_qrLabel->setAlignment(Qt::AlignCenter);
    m_qrLabel->setFrameShape(QFrame::StyledPanel);
    m_pubKeyLabel = new QLabel(this);
    m_pubKeyLabel->setWordWrap(true);
    m_uriLabel = new QLabel(this);
    m_uriLabel->setWordWrap(true);

    topLayout->addWidget(new QLabel(QStringLiteral("Address")), 0, 0);
    topLayout->addWidget(m_addressCombo, 0, 1);
    topLayout->addWidget(m_qrLabel, 1, 0, 3, 1);
    topLayout->addWidget(new QLabel(QStringLiteral("Public Key")), 1, 1);
    topLayout->addWidget(m_pubKeyLabel, 2, 1);
    topLayout->addWidget(new QLabel(QStringLiteral("Receive URI")), 3, 1);
    topLayout->addWidget(m_uriLabel, 4, 1);

    auto *createBox = new QGroupBox(QStringLiteral("Create New Address"), this);
    auto *form = new QFormLayout(createBox);
    m_labelEdit = new QLineEdit(this);
    m_passphraseEdit = new QLineEdit(this);
    m_passphraseEdit->setEchoMode(QLineEdit::Password);
    m_createButton = new QPushButton(QStringLiteral("Create Address"), this);
    form->addRow(QStringLiteral("Label"), m_labelEdit);
    form->addRow(QStringLiteral("Passphrase"), m_passphraseEdit);
    form->addRow(QString(), m_createButton);

    layout->addWidget(topBox);
    layout->addWidget(createBox);

    connect(m_addressCombo, &QComboBox::currentTextChanged, this, [this]() {
        updateSelectedKey();
        emit qrRequested(currentAddress());
    });
    connect(m_createButton, &QPushButton::clicked, this, [this]() {
        emit createAddressRequested(m_labelEdit->text(), m_passphraseEdit->text());
    });
}

QString ReceivePage::currentAddress() const
{
    return m_addressCombo->currentData().toString();
}

void ReceivePage::setOverview(const pacqt::Overview &overview)
{
    m_keys = overview.wallet.keys;
    m_addressCombo->blockSignals(true);
    m_addressCombo->clear();
    for (const KeySummary &key : overview.wallet.keys) {
        m_addressCombo->addItem(key.label + QStringLiteral(" - ") + key.address, key.address);
    }
    m_addressCombo->blockSignals(false);

    if (!overview.wallet.keys.isEmpty()) {
        m_addressCombo->setCurrentIndex(0);
        updateSelectedKey();
        emit qrRequested(currentAddress());
    } else {
        m_pubKeyLabel->clear();
        m_uriLabel->clear();
    }
}

void ReceivePage::setQrImage(const QString &address, const QByteArray &pngData)
{
    if (address != currentAddress()) {
        return;
    }
    QPixmap pixmap;
    pixmap.loadFromData(pngData, "PNG");
    m_qrLabel->setPixmap(pixmap.scaled(220, 220, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void ReceivePage::updateSelectedKey()
{
    const QString address = currentAddress();
    for (const KeySummary &key : m_keys) {
        if (key.address == address) {
            m_pubKeyLabel->setText(key.pubKeyHex);
            m_uriLabel->setText(QStringLiteral("pingancoin:%1").arg(key.address));
            return;
        }
    }
    m_pubKeyLabel->clear();
    m_uriLabel->clear();
}

} // namespace pacqt
