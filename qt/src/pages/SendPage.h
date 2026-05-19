#pragma once

#include <QLineEdit>
#include <QPushButton>
#include <QWidget>

namespace pacqt {

class SendPage : public QWidget
{
    Q_OBJECT

public:
    explicit SendPage(QWidget *parent = nullptr);

signals:
    void sendRequested(const QString &to, const QString &amount, const QString &fee, const QString &change, const QString &passphrase);

private:
    QLineEdit *m_toEdit;
    QLineEdit *m_amountEdit;
    QLineEdit *m_feeEdit;
    QLineEdit *m_changeEdit;
    QLineEdit *m_passphraseEdit;
};

} // namespace pacqt
