#pragma once

#include <QGroupBox>
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QWidget>

namespace pacqt {

class WelcomePage : public QWidget
{
    Q_OBJECT

public:
    explicit WelcomePage(QWidget *parent = nullptr);
    void retranslateUi();
    void setWalletExists(bool exists, const QString &path = QString());

signals:
    void createWalletRequested(const QString &passphrase);
    void restoreWalletRequested(const QString &walletJson, bool overwrite);

private:
    void refreshCreateState();

    QLabel *m_heroLabel;
    QLabel *m_subLabel;
    QGroupBox *m_createBox;
    QGroupBox *m_restoreBox;
    QLineEdit *m_passphraseEdit;
    QTextEdit *m_restoreEdit;
    QCheckBox *m_overwriteCheck;
    QPushButton *m_createButton;
    QPushButton *m_browseButton;
    QPushButton *m_restoreButton;
    bool m_walletExists = false;
    QString m_walletPath;
};

} // namespace pacqt
