#pragma once

#include "../Models.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QTextEdit>
#include <QWidget>

namespace pacqt {

class SettingsPage : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPage(QWidget *parent = nullptr);

    void setBackendUrl(const QString &url);
    void setBackendProgram(const QString &program);
    void setBackendArguments(const QStringList &arguments);
    void setOverview(const pacqt::Overview &overview);
    void setCurrentLanguageCode(const QString &code);
    void retranslateUi();
    QString backendUrl() const;
    QString backendProgram() const;
    QStringList backendArguments() const;
    void appendLog(const QString &line);

signals:
    void appLanguageChanged(const QString &code);
    void backendUrlChanged(const QString &url);
    void startBackendRequested(const QString &program, const QStringList &arguments);
    void stopBackendRequested();
    void encryptWalletRequested(const QString &passphrase);
    void changePassphraseRequested(const QString &oldPassphrase, const QString &newPassphrase);
    void importPrivateKeyRequested(const QString &label, const QString &privateKeyHex, const QString &passphrase);
    void addUpstreamRequested(const QString &name, const QString &url, bool makeActive);
    void selectUpstreamRequested(const QString &id);

private:
    bool m_hasOverview = false;
    pacqt::Overview m_overview;
    QString m_languageCode;
    QString m_walletPath;
    QString m_backupDir;
    QGroupBox *m_statusBox;
    QGroupBox *m_appearanceBox;
    QGroupBox *m_upstreamBox;
    QGroupBox *m_backendBox;
    QGroupBox *m_processBox;
    QGroupBox *m_securityBox;
    QGroupBox *m_importBox;
    QGroupBox *m_backupBox;
    QLabel *m_walletPathLabel;
    QLabel *m_walletStateLabel;
    QLabel *m_nodeStatusLabel;
    QLabel *m_activeUpstreamLabel;
    QComboBox *m_languageCombo;
    QComboBox *m_upstreamCombo;
    QLineEdit *m_upstreamNameEdit;
    QLineEdit *m_upstreamUrlEdit;
    QCheckBox *m_makeActiveCheck;
    QLineEdit *m_encryptPassphraseEdit;
    QLineEdit *m_oldPassphraseEdit;
    QLineEdit *m_newPassphraseEdit;
    QLineEdit *m_importLabelEdit;
    QLineEdit *m_importKeyEdit;
    QLineEdit *m_importPassphraseEdit;
    QListWidget *m_backupsList;
    QLineEdit *m_urlEdit;
    QLineEdit *m_programEdit;
    QLineEdit *m_argumentsEdit;
    QTextEdit *m_logView;
    QPushButton *m_openWalletPathButton;
    QPushButton *m_openBackupPathButton;
    QPushButton *m_activateUpstreamButton;
    QPushButton *m_addUpstreamButton;
    QPushButton *m_applyUrlButton;
    QPushButton *m_startBackendButton;
    QPushButton *m_stopBackendButton;
    QPushButton *m_encryptButton;
    QPushButton *m_changePassphraseButton;
    QPushButton *m_importButton;
};

} // namespace pacqt
