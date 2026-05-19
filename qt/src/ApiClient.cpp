#include "ApiClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <functional>

namespace pacqt {

ApiClient::ApiClient(QObject *parent)
    : QObject(parent)
    , m_baseUrl(QStringLiteral("http://127.0.0.1:19709"))
{
}

void ApiClient::setBaseUrl(const QUrl &url)
{
    m_baseUrl = url;
}

QUrl ApiClient::baseUrl() const
{
    return m_baseUrl;
}

QUrl ApiClient::apiUrl(const QString &path) const
{
    return m_baseUrl.resolved(QUrl(path));
}

void ApiClient::fetchOverview()
{
    QNetworkRequest request(apiUrl("/api/overview"));
    QNetworkReply *reply = m_network.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const QByteArray data = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            emit requestFailed(QStringLiteral("overview"), reply->errorString());
        } else {
            emit overviewReady(parseOverview(data));
        }
        reply->deleteLater();
    });
}

void ApiClient::fetchTransaction(const QString &txHash)
{
    QNetworkRequest request(apiUrl("/api/tx/" + txHash));
    QNetworkReply *reply = m_network.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        const QByteArray data = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            emit requestFailed(QStringLiteral("transaction"), reply->errorString());
        } else {
            emit transactionReady(parseTransactionDetail(data));
        }
        reply->deleteLater();
    });
}

void ApiClient::fetchReceiveQr(const QString &address, int size)
{
    QNetworkRequest request(apiUrl(QStringLiteral("/receive/qr/%1?size=%2").arg(address, QString::number(size))));
    QNetworkReply *reply = m_network.get(request);
    connect(reply, &QNetworkReply::finished, this, [this, reply, address]() {
        const QByteArray data = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            emit requestFailed(QStringLiteral("receive-qr"), reply->errorString());
        } else {
            emit receiveQrReady(address, data);
        }
        reply->deleteLater();
    });
}

void ApiClient::createWallet(const QString &passphrase)
{
    QJsonObject payload{
        {QStringLiteral("passphrase"), passphrase},
    };
    handleJsonPost(QStringLiteral("create-wallet"), QStringLiteral("/api/wallet/create"), payload, [this](const QByteArray &) {
        emit walletCreated();
    });
}

void ApiClient::encryptWallet(const QString &passphrase)
{
    QJsonObject payload{
        {QStringLiteral("passphrase"), passphrase},
    };
    handleJsonPost(QStringLiteral("encrypt-wallet"), QStringLiteral("/api/wallet/encrypt"), payload, [this](const QByteArray &) {
        emit walletEncrypted();
    });
}

void ApiClient::changePassphrase(const QString &oldPassphrase, const QString &newPassphrase)
{
    QJsonObject payload{
        {QStringLiteral("old_passphrase"), oldPassphrase},
        {QStringLiteral("new_passphrase"), newPassphrase},
    };
    handleJsonPost(QStringLiteral("change-passphrase"), QStringLiteral("/api/wallet/changepassphrase"), payload, [this](const QByteArray &) {
        emit walletPassphraseChanged();
    });
}

void ApiClient::restoreWallet(const QString &walletJson, bool overwrite)
{
    QJsonObject payload{
        {QStringLiteral("wallet_json"), walletJson},
        {QStringLiteral("overwrite"), overwrite},
    };
    handleJsonPost(QStringLiteral("restore-wallet"), QStringLiteral("/api/wallet/restore"), payload, [this](const QByteArray &) {
        emit walletRestored();
    });
}

void ApiClient::createAddress(const QString &label, const QString &passphrase)
{
    QJsonObject payload{
        {QStringLiteral("label"), label},
        {QStringLiteral("passphrase"), passphrase},
    };
    handleJsonPost(QStringLiteral("create-address"), QStringLiteral("/api/addresses"), payload, [this](const QByteArray &) {
        emit addressCreated();
    });
}

void ApiClient::importPrivateKey(const QString &label, const QString &privateKeyHex, const QString &passphrase)
{
    QJsonObject payload{
        {QStringLiteral("label"), label},
        {QStringLiteral("privkey_hex"), privateKeyHex},
        {QStringLiteral("passphrase"), passphrase},
    };
    handleJsonPost(QStringLiteral("import-private-key"), QStringLiteral("/api/keys/import"), payload, [this](const QByteArray &) {
        emit privateKeyImported();
    });
}

void ApiClient::addUpstream(const QString &name, const QString &url, bool makeActive)
{
    QJsonObject payload{
        {QStringLiteral("name"), name},
        {QStringLiteral("url"), url},
        {QStringLiteral("make_active"), makeActive},
    };
    handleJsonPost(QStringLiteral("add-upstream"), QStringLiteral("/api/upstreams"), payload, [this](const QByteArray &) {
        emit upstreamsUpdated();
    });
}

void ApiClient::selectUpstream(const QString &id)
{
    QJsonObject payload{
        {QStringLiteral("id"), id},
    };
    handleJsonPost(QStringLiteral("select-upstream"), QStringLiteral("/api/upstreams/select"), payload, [this](const QByteArray &) {
        emit upstreamsUpdated();
    });
}

void ApiClient::sendTransaction(const QString &to, const QString &amount, const QString &fee, const QString &change, const QString &passphrase)
{
    QJsonObject payload{
        {QStringLiteral("to"), to},
        {QStringLiteral("amount"), amount},
        {QStringLiteral("fee"), fee},
        {QStringLiteral("change"), change},
        {QStringLiteral("passphrase"), passphrase},
    };
    handleJsonPost(QStringLiteral("send"), QStringLiteral("/api/send"), payload, [this](const QByteArray &data) {
        const QJsonObject obj = QJsonDocument::fromJson(data).object();
        emit transactionSubmitted(obj.value(QStringLiteral("txid")).toString());
    });
}

void ApiClient::previewMultisig(int required, const QStringList &pubKeys)
{
    QJsonArray keys;
    for (const QString &key : pubKeys) {
        if (!key.trimmed().isEmpty()) {
            keys.push_back(key.trimmed());
        }
    }
    QJsonObject payload{
        {QStringLiteral("required"), required},
        {QStringLiteral("pubkeys"), keys},
    };
    handleJsonPost(QStringLiteral("multisig-preview"), QStringLiteral("/api/multisig/preview"), payload, [this](const QByteArray &data) {
        emit multisigPreviewReady(parseMultiSigPreview(data));
    });
}

void ApiClient::handleJsonPost(const QString &operation, const QString &path, const QJsonObject &payload, const std::function<void(const QByteArray &)> &onSuccess)
{
    QNetworkRequest request(apiUrl(path));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    QNetworkReply *reply = m_network.post(request, QJsonDocument(payload).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, operation, onSuccess]() {
        const QByteArray data = reply->readAll();
        if (reply->error() != QNetworkReply::NoError) {
            emit requestFailed(operation, reply->errorString());
        } else {
            onSuccess(data);
        }
        reply->deleteLater();
    });
}

} // namespace pacqt
