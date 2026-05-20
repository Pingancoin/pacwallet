#pragma once

#include "../Models.h"

#include <QComboBox>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QWidget>

namespace pacqt {

class SendPage : public QWidget
{
    Q_OBJECT

public:
    explicit SendPage(QWidget *parent = nullptr);
    void setOverview(const pacqt::Overview &overview);
    void retranslateUi();

signals:
    void sendRequested(const QString &to, const QString &amount, const QString &fee, const QString &change, const QString &passphrase);

private:
    QVector<KeySummary> m_keys;
    bool m_hasOverview = false;
    pacqt::Overview m_overview;
    QGroupBox *m_box;
    QLabel *m_balanceLabel;
    QLineEdit *m_toEdit;
    QLineEdit *m_amountEdit;
    QLineEdit *m_feeEdit;
    QComboBox *m_changeCombo;
    QLineEdit *m_passphraseEdit;
    QPushButton *m_sendButton;
    QPushButton *m_maxButton;
};

} // namespace pacqt
