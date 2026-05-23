#pragma once

#include "../Models.h"

#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QWidget>

namespace pacqt {

class SettingsPage : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPage(QWidget *parent = nullptr);

    void setOverview(const pacqt::Overview &overview);
    void setCurrentLanguageCode(const QString &code);
    void retranslateUi();
    void appendLog(const QString &line);

signals:
    void appLanguageChanged(const QString &code);
    void encryptWalletRequested(const QString &passphrase);
    void changePassphraseRequested(const QString &oldPassphrase, const QString &newPassphrase);
    void importPrivateKeyRequested(const QString &label, const QString &privateKeyHex, const QString &passphrase);

private:
    bool m_hasOverview = false;
    pacqt::Overview m_overview;
    QString m_languageCode;
    QString m_walletPath;
    QString m_backupDir;
    QGroupBox *m_statusBox;
    QGroupBox *m_appearanceBox;
    QGroupBox *m_securityBox;
    QGroupBox *m_importBox;
    QGroupBox *m_backupBox;
    QLabel *m_versionLabel;
    QLabel *m_walletPathLabel;
    QLabel *m_walletStateLabel;
    QLabel *m_nodeStatusLabel;
    QComboBox *m_languageCombo;
    QLineEdit *m_encryptPassphraseEdit;
    QLineEdit *m_oldPassphraseEdit;
    QLineEdit *m_newPassphraseEdit;
    QLineEdit *m_importLabelEdit;
    QLineEdit *m_importKeyEdit;
    QLineEdit *m_importPassphraseEdit;
    QListWidget *m_backupsList;
    QPushButton *m_openWalletPathButton;
    QPushButton *m_openBackupPathButton;
    QPushButton *m_encryptButton;
    QPushButton *m_changePassphraseButton;
    QPushButton *m_importButton;
};

} // namespace pacqt
