#pragma once

#include "Models.h"

#include <QNetworkAccessManager>
#include <QObject>
#include <QUrl>
#include <functional>

namespace pacqt {

class ApiClient : public QObject
{
    Q_OBJECT

public:
    explicit ApiClient(QObject *parent = nullptr);

    void setBaseUrl(const QUrl &url);
    QUrl baseUrl() const;

    void fetchOverview();
    void fetchTransaction(const QString &txHash);
    void fetchReceiveQr(const QString &address, int size = 240);
    void createWallet(const QString &passphrase);
    void encryptWallet(const QString &passphrase);
    void changePassphrase(const QString &oldPassphrase, const QString &newPassphrase);
    void restoreWallet(const QString &walletJson, bool overwrite);
    void createAddress(const QString &label, const QString &passphrase);
    void importPrivateKey(const QString &label, const QString &privateKeyHex, const QString &passphrase);
    void addUpstream(const QString &name, const QString &url, bool makeActive);
    void selectUpstream(const QString &id);
    void sendTransaction(const QString &to, const QString &amount, const QString &fee, const QString &change, const QString &passphrase);
    void previewMultisig(int required, const QStringList &pubKeys);

signals:
    void overviewReady(const pacqt::Overview &overview);
    void transactionReady(const pacqt::TransactionDetail &detail);
    void receiveQrReady(const QString &address, const QByteArray &png);
    void walletCreated();
    void walletEncrypted();
    void walletPassphraseChanged();
    void walletRestored();
    void addressCreated();
    void privateKeyImported();
    void upstreamsUpdated();
    void transactionSubmitted(const QString &txid);
    void multisigPreviewReady(const pacqt::MultiSigPreviewResult &result);
    void requestFailed(const QString &operation, const QString &message);

private:
    QUrl apiUrl(const QString &path) const;
    void handleJsonPost(const QString &operation, const QString &path, const QJsonObject &payload, const std::function<void(const QByteArray &)> &onSuccess);

    QNetworkAccessManager m_network;
    QUrl m_baseUrl;
};

} // namespace pacqt
