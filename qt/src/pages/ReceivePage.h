#pragma once

#include "../Models.h"

#include <QComboBox>
#include <QByteArray>
#include <QGroupBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QWidget>

namespace pacqt {

class ReceivePage : public QWidget
{
    Q_OBJECT

public:
    explicit ReceivePage(QWidget *parent = nullptr);
    void setOverview(const pacqt::Overview &overview);
    void setQrImage(const QString &address, const QByteArray &pngData);
    void retranslateUi();

signals:
    void createAddressRequested(const QString &label, const QString &passphrase);
    void qrRequested(const QString &address);

private:
    void updateSelectedKey();
    QString currentAddress() const;

    QVector<KeySummary> m_keys;
    QByteArray m_qrPngData;
    QLabel *m_addressTitleLabel;
    QComboBox *m_addressCombo;
    QLabel *m_qrLabel;
    QLabel *m_pubKeyTitleLabel;
    QLabel *m_pubKeyLabel;
    QLabel *m_uriTitleLabel;
    QLabel *m_uriLabel;
    QLineEdit *m_labelEdit;
    QLineEdit *m_passphraseEdit;
    QPushButton *m_createButton;
    QPushButton *m_copyAddressButton;
    QPushButton *m_copyPubKeyButton;
    QPushButton *m_copyUriButton;
    QPushButton *m_saveQrButton;
    QGroupBox *m_topBox;
    QGroupBox *m_createBox;
};

} // namespace pacqt
