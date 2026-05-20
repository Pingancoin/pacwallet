#include "ReceivePage.h"
#include "../Localization.h"

#include <QApplication>
#include <QClipboard>
#include <QFile>
#include <QFileDialog>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QMessageBox>
#include <QPixmap>
#include <QHBoxLayout>
#include <QVBoxLayout>

namespace pacqt {

ReceivePage::ReceivePage(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    m_topBox = new QGroupBox(QStringLiteral("Receive"), this);
    auto *topLayout = new QGridLayout(m_topBox);
    topLayout->setHorizontalSpacing(12);
    topLayout->setVerticalSpacing(8);

    m_addressTitleLabel = new QLabel(this);
    m_addressCombo = new QComboBox(this);
    m_qrLabel = new QLabel(this);
    m_qrLabel->setMinimumSize(160, 160);
    m_qrLabel->setMaximumSize(176, 176);
    m_qrLabel->setAlignment(Qt::AlignCenter);
    m_qrLabel->setFrameShape(QFrame::StyledPanel);
    m_pubKeyTitleLabel = new QLabel(this);
    m_pubKeyLabel = new QLabel(this);
    m_pubKeyLabel->setWordWrap(true);
    m_uriTitleLabel = new QLabel(this);
    m_uriLabel = new QLabel(this);
    m_uriLabel->setWordWrap(true);
    m_copyAddressButton = new QPushButton(this);
    m_copyPubKeyButton = new QPushButton(this);
    m_copyUriButton = new QPushButton(this);
    m_saveQrButton = new QPushButton(this);
    auto *buttonRow = new QHBoxLayout();
    buttonRow->setSpacing(8);
    buttonRow->addWidget(m_copyAddressButton);
    buttonRow->addWidget(m_copyPubKeyButton);
    buttonRow->addWidget(m_copyUriButton);
    buttonRow->addWidget(m_saveQrButton);

    topLayout->addWidget(m_addressTitleLabel, 0, 0);
    topLayout->addWidget(m_addressCombo, 0, 1);
    topLayout->addWidget(m_qrLabel, 1, 0, 3, 1);
    topLayout->addWidget(m_pubKeyTitleLabel, 1, 1);
    topLayout->addWidget(m_pubKeyLabel, 2, 1);
    topLayout->addWidget(m_uriTitleLabel, 3, 1);
    topLayout->addWidget(m_uriLabel, 4, 1);
    topLayout->addLayout(buttonRow, 5, 0, 1, 2);
    topLayout->setColumnStretch(0, 0);
    topLayout->setColumnStretch(1, 1);

    m_createBox = new QGroupBox(QStringLiteral("Create New Address"), this);
    auto *form = new QFormLayout(m_createBox);
    m_labelEdit = new QLineEdit(this);
    m_passphraseEdit = new QLineEdit(this);
    m_passphraseEdit->setEchoMode(QLineEdit::Password);
    m_createButton = new QPushButton(this);
    form->addRow(QStringLiteral("Label"), m_labelEdit);
    form->addRow(QStringLiteral("Passphrase"), m_passphraseEdit);
    form->addRow(QString(), m_createButton);

    layout->addWidget(m_topBox, 0);
    layout->addWidget(m_createBox, 0);
    layout->addStretch(1);

    connect(m_addressCombo, &QComboBox::currentTextChanged, this, [this]() {
        updateSelectedKey();
        emit qrRequested(currentAddress());
    });
    connect(m_createButton, &QPushButton::clicked, this, [this]() {
        emit createAddressRequested(m_labelEdit->text(), m_passphraseEdit->text());
    });
    connect(m_copyAddressButton, &QPushButton::clicked, this, [this]() {
        if (!currentAddress().isEmpty()) {
            QApplication::clipboard()->setText(currentAddress());
        }
    });
    connect(m_copyPubKeyButton, &QPushButton::clicked, this, [this]() {
        if (!m_pubKeyLabel->text().isEmpty()) {
            QApplication::clipboard()->setText(m_pubKeyLabel->text());
        }
    });
    connect(m_copyUriButton, &QPushButton::clicked, this, [this]() {
        if (!m_uriLabel->text().isEmpty()) {
            QApplication::clipboard()->setText(m_uriLabel->text());
        }
    });
    connect(m_saveQrButton, &QPushButton::clicked, this, [this]() {
        if (m_qrPngData.isEmpty() || currentAddress().isEmpty()) {
            QMessageBox::information(this, l10n::text(QStringLiteral("Pingancoin Wallet")), l10n::text(QStringLiteral("No QR image is available yet for the selected address.")));
            return;
        }
        const QString path = QFileDialog::getSaveFileName(this,
            l10n::text(QStringLiteral("Save Receive QR")),
            QStringLiteral("%1.png").arg(currentAddress()),
            QStringLiteral("PNG Files (*.png)"));
        if (path.isEmpty()) {
            return;
        }
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) {
            QMessageBox::warning(this, l10n::text(QStringLiteral("Pingancoin Wallet")), l10n::text(QStringLiteral("Could not write QR image to %1")).arg(path));
            return;
        }
        file.write(m_qrPngData);
    });
    retranslateUi();
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
    m_qrPngData = pngData;
    QPixmap pixmap;
    pixmap.loadFromData(pngData, "PNG");
    m_qrLabel->setPixmap(pixmap.scaled(156, 156, Qt::KeepAspectRatio, Qt::SmoothTransformation));
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

void ReceivePage::retranslateUi()
{
    m_topBox->setTitle(l10n::text(QStringLiteral("Receive")));
    m_createBox->setTitle(l10n::text(QStringLiteral("Create New Address")));
    m_addressTitleLabel->setText(l10n::text(QStringLiteral("Address")));
    m_pubKeyTitleLabel->setText(l10n::text(QStringLiteral("Public Key")));
    m_uriTitleLabel->setText(l10n::text(QStringLiteral("Receive URI")));
    m_copyAddressButton->setText(l10n::text(QStringLiteral("Copy Address")));
    m_copyPubKeyButton->setText(l10n::text(QStringLiteral("Copy Public Key")));
    m_copyUriButton->setText(l10n::text(QStringLiteral("Copy URI")));
    m_saveQrButton->setText(l10n::text(QStringLiteral("Save QR PNG")));
    m_labelEdit->setPlaceholderText(l10n::text(QStringLiteral("Label")));
    m_passphraseEdit->setPlaceholderText(l10n::text(QStringLiteral("Passphrase")));
    m_createButton->setText(l10n::text(QStringLiteral("Create Address")));
}

} // namespace pacqt
