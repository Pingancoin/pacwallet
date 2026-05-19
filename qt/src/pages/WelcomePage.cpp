#include "WelcomePage.h"

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

    auto *hero = new QLabel(QStringLiteral("Set up Pingancoin Wallet"), this);
    hero->setStyleSheet(QStringLiteral("font-size: 28px; font-weight: 700;"));
    auto *sub = new QLabel(QStringLiteral("Create a new wallet or restore an existing wallet.json before using the native desktop client."), this);
    sub->setWordWrap(true);

    auto *createBox = new QGroupBox(QStringLiteral("Create New Wallet"), this);
    auto *createLayout = new QFormLayout(createBox);
    m_passphraseEdit = new QLineEdit(this);
    m_passphraseEdit->setEchoMode(QLineEdit::Password);
    auto *createButton = new QPushButton(QStringLiteral("Create Wallet"), this);
    createLayout->addRow(QStringLiteral("Passphrase"), m_passphraseEdit);
    createLayout->addRow(QString(), createButton);

    auto *restoreBox = new QGroupBox(QStringLiteral("Restore Existing Wallet"), this);
    auto *restoreLayout = new QVBoxLayout(restoreBox);
    m_restoreEdit = new QTextEdit(this);
    m_restoreEdit->setPlaceholderText(QStringLiteral("Paste wallet.json contents here"));
    auto *browseButton = new QPushButton(QStringLiteral("Load wallet.json From File"), this);
    m_overwriteCheck = new QCheckBox(QStringLiteral("Overwrite any existing local wallet and archive it first"), this);
    auto *restoreButton = new QPushButton(QStringLiteral("Restore Wallet"), this);
    restoreLayout->addWidget(browseButton);
    restoreLayout->addWidget(m_restoreEdit);
    restoreLayout->addWidget(m_overwriteCheck);
    restoreLayout->addWidget(restoreButton);

    layout->addWidget(hero);
    layout->addWidget(sub);
    layout->addSpacing(8);
    layout->addWidget(createBox);
    layout->addWidget(restoreBox, 1);

    connect(createButton, &QPushButton::clicked, this, [this]() {
        emit createWalletRequested(m_passphraseEdit->text());
    });
    connect(browseButton, &QPushButton::clicked, this, [this]() {
        const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Open wallet.json"), QString(), QStringLiteral("JSON Files (*.json);;All Files (*)"));
        if (path.isEmpty()) {
            return;
        }
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
            m_restoreEdit->setPlainText(QString::fromUtf8(file.readAll()));
        }
    });
    connect(restoreButton, &QPushButton::clicked, this, [this]() {
        emit restoreWalletRequested(m_restoreEdit->toPlainText(), m_overwriteCheck->isChecked());
    });
}

} // namespace pacqt
