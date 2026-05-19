#pragma once

#include "../Models.h"

#include <QComboBox>
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

signals:
    void sendRequested(const QString &to, const QString &amount, const QString &fee, const QString &change, const QString &passphrase);

private:
    QVector<KeySummary> m_keys;
    QLabel *m_balanceLabel;
    QLineEdit *m_toEdit;
    QLineEdit *m_amountEdit;
    QLineEdit *m_feeEdit;
    QComboBox *m_changeCombo;
    QLineEdit *m_passphraseEdit;
};

} // namespace pacqt
