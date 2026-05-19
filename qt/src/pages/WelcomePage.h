#pragma once

#include <QCheckBox>
#include <QLineEdit>
#include <QTextEdit>
#include <QWidget>

namespace pacqt {

class WelcomePage : public QWidget
{
    Q_OBJECT

public:
    explicit WelcomePage(QWidget *parent = nullptr);

signals:
    void createWalletRequested(const QString &passphrase);
    void restoreWalletRequested(const QString &walletJson, bool overwrite);

private:
    QLineEdit *m_passphraseEdit;
    QTextEdit *m_restoreEdit;
    QCheckBox *m_overwriteCheck;
};

} // namespace pacqt
