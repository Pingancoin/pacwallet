#pragma once

#include "../Models.h"

#include <QCheckBox>
#include <QComboBox>
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
    QString backendUrl() const;
    QString backendProgram() const;
    QStringList backendArguments() const;
    void appendLog(const QString &line);

signals:
    void backendUrlChanged(const QString &url);
    void startBackendRequested(const QString &program, const QStringList &arguments);
    void stopBackendRequested();
    void encryptWalletRequested(const QString &passphrase);
    void changePassphraseRequested(const QString &oldPassphrase, const QString &newPassphrase);
    void importPrivateKeyRequested(const QString &label, const QString &privateKeyHex, const QString &passphrase);
    void addUpstreamRequested(const QString &name, const QString &url, bool makeActive);
    void selectUpstreamRequested(const QString &id);

private:
    QString m_walletPath;
    QString m_backupDir;
    QLabel *m_walletPathLabel;
    QLabel *m_walletStateLabel;
    QLabel *m_nodeStatusLabel;
    QLabel *m_activeUpstreamLabel;
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
};

} // namespace pacqt
