#pragma once

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVector>

namespace pacqt {

struct KeySummary {
    QString label;
    QString address;
    QString pubKeyHex;
    QDateTime createdAt;
};

struct BackupInfo {
    QString name;
    QString path;
    qint64 sizeBytes = 0;
    QDateTime createdAt;
};

struct WalletSummary {
    bool exists = false;
    QString path;
    QString backupDir;
    QString network;
    bool encrypted = false;
    int keyCount = 0;
    QString addressHint;
    QDateTime createdAt;
    QVector<KeySummary> keys;
    QVector<BackupInfo> backups;
};

struct Utxo {
    QString address;
    QString txHash;
    int vout = 0;
    qint64 value = 0;
    int height = 0;
    bool coinbase = false;
    bool mature = false;
    bool pending = false;
};

struct Balance {
    qint64 total = 0;
    qint64 spendable = 0;
    qint64 immature = 0;
    qint64 pending = 0;
    int utxoCount = 0;
    int bestHeight = 0;
    QString bestHash;
    QVector<Utxo> utxos;
};

struct HistoryEntry {
    QString txHash;
    int height = 0;
    bool pending = false;
    bool coinbase = false;
    qint64 received = 0;
    qint64 sent = 0;
    qint64 net = 0;
    QStringList addresses;
};

struct UpstreamProfile {
    QString id;
    QString name;
    QString url;
    QString source;
};

struct UpstreamSettings {
    QString configPath;
    QString activeId;
    QString activeUrl;
    QVector<UpstreamProfile> profiles;
};

struct NodeStatus {
    bool online = false;
    QString endpoint;
    QString network;
    int bestHeight = 0;
    QString bestHash;
    int mempoolSize = 0;
    int peerCount = 0;
    QString error;
};

struct Overview {
    WalletSummary wallet;
    Balance balance;
    QVector<HistoryEntry> history;
    QString rpcUrl;
    UpstreamSettings upstream;
    NodeStatus node;
};

struct TxInputDetail {
    QString prevTxHash;
    int prevVout = 0;
    QString address;
    qint64 value = 0;
    bool walletOwned = false;
};

struct TxOutputDetail {
    int index = 0;
    QString address;
    qint64 value = 0;
    bool walletOwned = false;
    bool spent = false;
};

struct TransactionDetail {
    QString txHash;
    int height = 0;
    bool pending = false;
    bool coinbase = false;
    qint64 received = 0;
    qint64 sent = 0;
    qint64 net = 0;
    int confirmations = 0;
    int bestHeight = 0;
    QStringList addresses;
    QVector<TxInputDetail> inputs;
    QVector<TxOutputDetail> outputs;
};

struct MultiSigPreviewResult {
    int required = 0;
    int participants = 0;
    QStringList pubKeys;
    QString address;
    QString scriptHash;
    QString redeemScript;
    QString p2shScript;
};

inline QString formatPac(qint64 atoms)
{
    const qint64 whole = atoms / 100000000;
    const qint64 frac = qAbs(atoms % 100000000);
    return QString("%1.%2")
        .arg(whole)
        .arg(frac, 8, 10, QChar('0'));
}

inline QDateTime parseTimestamp(const QJsonValue &value)
{
    return QDateTime::fromString(value.toString(), Qt::ISODate);
}

inline KeySummary parseKeySummary(const QJsonObject &obj)
{
    KeySummary key;
    key.label = obj.value("label").toString();
    key.address = obj.value("address").toString();
    key.pubKeyHex = obj.value("pubkey_hex").toString();
    key.createdAt = parseTimestamp(obj.value("created_at"));
    return key;
}

inline BackupInfo parseBackupInfo(const QJsonObject &obj)
{
    BackupInfo info;
    info.name = obj.value("name").toString();
    info.path = obj.value("path").toString();
    info.sizeBytes = obj.value("size_bytes").toInteger();
    info.createdAt = parseTimestamp(obj.value("created_at"));
    return info;
}

inline WalletSummary parseWalletSummary(const QJsonObject &obj)
{
    WalletSummary wallet;
    wallet.exists = obj.value("exists").toBool();
    wallet.path = obj.value("path").toString();
    wallet.backupDir = obj.value("backup_dir").toString();
    wallet.network = obj.value("network").toString();
    wallet.encrypted = obj.value("encrypted").toBool();
    wallet.keyCount = obj.value("key_count").toInt();
    wallet.addressHint = obj.value("address_hint").toString();
    wallet.createdAt = parseTimestamp(obj.value("created_at"));

    for (const QJsonValue &item : obj.value("keys").toArray()) {
        wallet.keys.push_back(parseKeySummary(item.toObject()));
    }
    for (const QJsonValue &item : obj.value("backups").toArray()) {
        wallet.backups.push_back(parseBackupInfo(item.toObject()));
    }
    return wallet;
}

inline Utxo parseUtxo(const QJsonObject &obj)
{
    Utxo utxo;
    utxo.address = obj.value("address").toString();
    utxo.txHash = obj.value("tx_hash").toString();
    utxo.vout = obj.value("vout").toInt();
    utxo.value = obj.value("value").toInteger();
    utxo.height = obj.value("height").toInt();
    utxo.coinbase = obj.value("coinbase").toBool();
    utxo.mature = obj.value("mature").toBool();
    utxo.pending = obj.value("pending").toBool();
    return utxo;
}

inline Balance parseBalance(const QJsonObject &obj)
{
    Balance balance;
    balance.total = obj.value("total").toInteger();
    balance.spendable = obj.value("spendable").toInteger();
    balance.immature = obj.value("immature").toInteger();
    balance.pending = obj.value("pending").toInteger();
    balance.utxoCount = obj.value("utxo_count").toInt();
    balance.bestHeight = obj.value("best_height").toInt();
    balance.bestHash = obj.value("best_hash").toString();
    for (const QJsonValue &item : obj.value("utxos").toArray()) {
        balance.utxos.push_back(parseUtxo(item.toObject()));
    }
    return balance;
}

inline HistoryEntry parseHistoryEntry(const QJsonObject &obj)
{
    HistoryEntry entry;
    entry.txHash = obj.value("tx_hash").toString();
    entry.height = obj.value("height").toInt();
    entry.pending = obj.value("pending").toBool();
    entry.coinbase = obj.value("coinbase").toBool();
    entry.received = obj.value("received").toInteger();
    entry.sent = obj.value("sent").toInteger();
    entry.net = obj.value("net").toInteger();
    for (const QJsonValue &value : obj.value("addresses").toArray()) {
        entry.addresses.push_back(value.toString());
    }
    return entry;
}

inline UpstreamProfile parseUpstreamProfile(const QJsonObject &obj)
{
    UpstreamProfile profile;
    profile.id = obj.value("id").toString();
    profile.name = obj.value("name").toString();
    profile.url = obj.value("url").toString();
    profile.source = obj.value("source").toString();
    return profile;
}

inline UpstreamSettings parseUpstreamSettings(const QJsonObject &obj)
{
    UpstreamSettings settings;
    settings.configPath = obj.value("config_path").toString();
    settings.activeId = obj.value("active_id").toString();
    settings.activeUrl = obj.value("active_url").toString();
    for (const QJsonValue &value : obj.value("profiles").toArray()) {
        settings.profiles.push_back(parseUpstreamProfile(value.toObject()));
    }
    return settings;
}

inline NodeStatus parseNodeStatus(const QJsonObject &obj)
{
    NodeStatus node;
    node.online = obj.value("online").toBool();
    node.endpoint = obj.value("endpoint").toString();
    node.network = obj.value("network").toString();
    node.bestHeight = obj.value("best_height").toInt();
    node.bestHash = obj.value("best_hash").toString();
    node.mempoolSize = obj.value("mempool_size").toInt();
    node.peerCount = obj.value("peer_count").toInt();
    node.error = obj.value("error").toString();
    return node;
}

inline Overview parseOverview(const QByteArray &json)
{
    const QJsonObject obj = QJsonDocument::fromJson(json).object();
    Overview overview;
    overview.wallet = parseWalletSummary(obj.value("wallet").toObject());
    overview.balance = parseBalance(obj.value("balance").toObject());
    for (const QJsonValue &value : obj.value("history").toArray()) {
        overview.history.push_back(parseHistoryEntry(value.toObject()));
    }
    overview.rpcUrl = obj.value("rpc_url").toString();
    overview.upstream = parseUpstreamSettings(obj.value("upstream").toObject());
    overview.node = parseNodeStatus(obj.value("node").toObject());
    return overview;
}

inline TransactionDetail parseTransactionDetail(const QByteArray &json)
{
    const QJsonObject obj = QJsonDocument::fromJson(json).object();
    TransactionDetail detail;
    detail.txHash = obj.value("tx_hash").toString();
    detail.height = obj.value("height").toInt();
    detail.pending = obj.value("pending").toBool();
    detail.coinbase = obj.value("coinbase").toBool();
    detail.received = obj.value("received").toInteger();
    detail.sent = obj.value("sent").toInteger();
    detail.net = obj.value("net").toInteger();
    detail.confirmations = obj.value("confirmations").toInt();
    detail.bestHeight = obj.value("best_height").toInt();
    for (const QJsonValue &value : obj.value("addresses").toArray()) {
        detail.addresses.push_back(value.toString());
    }
    for (const QJsonValue &value : obj.value("inputs").toArray()) {
        const QJsonObject item = value.toObject();
        TxInputDetail input;
        input.prevTxHash = item.value("prev_tx_hash").toString();
        input.prevVout = item.value("prev_vout").toInt();
        input.address = item.value("address").toString();
        input.value = item.value("value").toInteger();
        input.walletOwned = item.value("wallet_owned").toBool();
        detail.inputs.push_back(input);
    }
    for (const QJsonValue &value : obj.value("outputs").toArray()) {
        const QJsonObject item = value.toObject();
        TxOutputDetail output;
        output.index = item.value("index").toInt();
        output.address = item.value("address").toString();
        output.value = item.value("value").toInteger();
        output.walletOwned = item.value("wallet_owned").toBool();
        output.spent = item.value("spent").toBool();
        detail.outputs.push_back(output);
    }
    return detail;
}

inline MultiSigPreviewResult parseMultiSigPreview(const QByteArray &json)
{
    const QJsonObject obj = QJsonDocument::fromJson(json).object();
    MultiSigPreviewResult result;
    result.required = obj.value("required").toInt();
    result.participants = obj.value("participants").toInt();
    result.address = obj.value("address").toString();
    result.scriptHash = obj.value("script_hash").toString();
    result.redeemScript = obj.value("redeem_script").toString();
    result.p2shScript = obj.value("p2sh_script").toString();
    for (const QJsonValue &value : obj.value("pubkeys").toArray()) {
        result.pubKeys.push_back(value.toString());
    }
    return result;
}

} // namespace pacqt
