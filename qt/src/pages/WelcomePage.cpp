#include "WelcomePage.h"
#include "../Localization.h"

#include <QFileDialog>
#include <QFile>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace pacqt {

WelcomePage::WelcomePage(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    m_heroLabel = new QLabel(QStringLiteral("Set up Pingancoin Wallet"), this);
    m_heroLabel->setStyleSheet(QStringLiteral("font-size: 26px; font-weight: 700;"));
    m_subLabel = new QLabel(QStringLiteral("Create a new wallet on this Mac, or restore an existing wallet.json to continue from another machine."), this);
    m_subLabel->setWordWrap(true);
    m_subLabel->setStyleSheet(QStringLiteral("font-size: 13px; color: #475569; line-height: 1.4;"));

    auto *cardsLayout = new QHBoxLayout();
    cardsLayout->setSpacing(12);

    m_createBox = new QGroupBox(QStringLiteral("Create New Wallet"), this);
    auto *createLayout = new QFormLayout(m_createBox);
    createLayout->setHorizontalSpacing(14);
    createLayout->setVerticalSpacing(10);
    m_passphraseEdit = new QLineEdit(this);
    m_passphraseEdit->setEchoMode(QLineEdit::Password);
    m_createButton = new QPushButton(QStringLiteral("Create Wallet"), this);
    auto *createHint = new QLabel(QStringLiteral("Create and restore"), this);
    createHint->setObjectName(QStringLiteral("welcomeCreateHint"));
    createHint->setWordWrap(true);
    createHint->setStyleSheet(QStringLiteral("color: #475569;"));
    createLayout->addRow(createHint);
    createLayout->addRow(QStringLiteral("Passphrase"), m_passphraseEdit);
    createLayout->addRow(QString(), m_createButton);

    m_restoreBox = new QGroupBox(QStringLiteral("Restore Existing Wallet"), this);
    auto *restoreLayout = new QVBoxLayout(m_restoreBox);
    restoreLayout->setSpacing(12);
    m_restoreEdit = new QTextEdit(this);
    m_restoreEdit->setPlaceholderText(QStringLiteral("Paste wallet.json contents here"));
    m_browseButton = new QPushButton(QStringLiteral("Load wallet.json From File"), this);
    m_overwriteCheck = new QCheckBox(QStringLiteral("Overwrite any existing local wallet and archive it first"), this);
    m_restoreButton = new QPushButton(QStringLiteral("Restore Wallet"), this);
    restoreLayout->addWidget(m_browseButton);
    restoreLayout->addWidget(m_restoreEdit);
    restoreLayout->addWidget(m_overwriteCheck);
    restoreLayout->addWidget(m_restoreButton);

    cardsLayout->addWidget(m_createBox, 1);
    cardsLayout->addWidget(m_restoreBox, 2);

    layout->addWidget(m_heroLabel);
    layout->addWidget(m_subLabel);
    layout->addLayout(cardsLayout, 1);

    connect(m_createButton, &QPushButton::clicked, this, [this]() {
        emit createWalletRequested(m_passphraseEdit->text());
    });
    connect(m_browseButton, &QPushButton::clicked, this, [this]() {
        const QString path = QFileDialog::getOpenFileName(this, l10n::text(QStringLiteral("Open wallet.json")), QString(), l10n::text(QStringLiteral("JSON Files (*.json);;All Files (*)")));
        if (path.isEmpty()) {
            return;
        }
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            m_restoreEdit->setPlainText(QString::fromUtf8(file.readAll()));
        }
    });
    connect(m_restoreButton, &QPushButton::clicked, this, [this]() {
        emit restoreWalletRequested(m_restoreEdit->toPlainText(), m_overwriteCheck->isChecked());
    });
    retranslateUi();
}

void WelcomePage::retranslateUi()
{
    m_heroLabel->setText(l10n::text(QStringLiteral("Set up Pingancoin Wallet")));
    m_subLabel->setText(l10n::text(QStringLiteral("Create a new wallet on this Mac, or restore an existing wallet.json to continue from another machine.")));
    m_createBox->setTitle(l10n::text(QStringLiteral("Create New Wallet")));
    m_restoreBox->setTitle(l10n::text(QStringLiteral("Restore Existing Wallet")));
    if (auto *form = qobject_cast<QFormLayout *>(m_createBox->layout())) {
        if (auto *item = form->itemAt(1, QFormLayout::LabelRole)) {
            if (auto *label = qobject_cast<QLabel *>(item->widget())) {
                label->setText(l10n::text(QStringLiteral("Passphrase")));
            }
        }
    }
    m_passphraseEdit->setPlaceholderText(l10n::text(QStringLiteral("Passphrase")));
    m_restoreEdit->setPlaceholderText(l10n::text(QStringLiteral("Paste wallet.json contents here")));
    if (auto *createHint = findChild<QLabel *>(QStringLiteral("welcomeCreateHint"))) {
        createHint->setText(l10n::text(QStringLiteral("Create and restore")));
    }
    m_overwriteCheck->setText(l10n::text(QStringLiteral("Overwrite any existing local wallet and archive it first")));
    m_createButton->setText(l10n::text(QStringLiteral("Create Wallet")));
    m_browseButton->setText(l10n::text(QStringLiteral("Load wallet.json From File")));
    m_restoreButton->setText(l10n::text(QStringLiteral("Restore Wallet")));
}

} // namespace pacqt
