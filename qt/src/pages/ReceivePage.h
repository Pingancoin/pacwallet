#pragma once

#include "../Models.h"

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QWidget>

namespace pacqt {

class ReceivePage : public QWidget
{
    Q_OBJECT

public:
    explicit ReceivePage(QWidget *parent = nullptr);
    void setOverview(const pacqt::Overview &overview);
    void setQrImage(const QString &address, const QByteArray &pngData);

signals:
    void createAddressRequested(const QString &label, const QString &passphrase);
    void qrRequested(const QString &address);

private:
    void updateSelectedKey();
    QString currentAddress() const;

    QVector<KeySummary> m_keys;
    QComboBox *m_addressCombo;
    QLabel *m_qrLabel;
    QLabel *m_pubKeyLabel;
    QLabel *m_uriLabel;
    QLineEdit *m_labelEdit;
    QLineEdit *m_passphraseEdit;
    QPushButton *m_createButton;
};

} // namespace pacqt
