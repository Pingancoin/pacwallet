#include "Localization.h"

#include <QLocale>
#include <QMap>

namespace pacqt::l10n {

namespace {

QString g_languageCode = defaultLanguageCode();

QMap<QString, QString> zhTranslations()
{
    return {
        {QStringLiteral("Pingancoin Wallet"), QStringLiteral("Pingancoin 钱包")},
        {QStringLiteral("Welcome"), QStringLiteral("欢迎")},
        {QStringLiteral("Overview"), QStringLiteral("总览")},
        {QStringLiteral("Receive"), QStringLiteral("收款")},
        {QStringLiteral("Send"), QStringLiteral("发送")},
        {QStringLiteral("Transactions"), QStringLiteral("交易")},
        {QStringLiteral("Multisig"), QStringLiteral("多签")},
        {QStringLiteral("Settings"), QStringLiteral("设置")},
        {QStringLiteral("Native Qt wallet ready."), QStringLiteral("原生 Qt 钱包已就绪。")},
        {QStringLiteral("Native desktop wallet for balances, transfers, multisig, and node settings."), QStringLiteral("用于余额、转账、多签和节点设置的原生桌面钱包。")},
        {QStringLiteral("Wallet synced from %1"), QStringLiteral("钱包已从 %1 同步。")},
        {QStringLiteral("Wallet created."), QStringLiteral("钱包已创建。")},
        {QStringLiteral("Wallet encrypted."), QStringLiteral("钱包已加密。")},
        {QStringLiteral("Wallet passphrase changed."), QStringLiteral("钱包口令已更新。")},
        {QStringLiteral("Wallet restored."), QStringLiteral("钱包已恢复。")},
        {QStringLiteral("New receive address created."), QStringLiteral("新的收款地址已创建。")},
        {QStringLiteral("Private key imported."), QStringLiteral("私钥已导入。")},
        {QStringLiteral("RPC upstreams updated."), QStringLiteral("RPC 上游节点已更新。")},
        {QStringLiteral("Transaction submitted: %1"), QStringLiteral("交易已提交：%1")},
        {QStringLiteral("Backend URL updated."), QStringLiteral("后端地址已更新。")},
        {QStringLiteral("Local pacwallet service started."), QStringLiteral("本地 pacwallet 服务已启动。")},
        {QStringLiteral("Local pacwallet service stopped."), QStringLiteral("本地 pacwallet 服务已停止。")},
        {QStringLiteral("%1 failed: %2"), QStringLiteral("%1 失败：%2")},
        {QStringLiteral("%1 failed."), QStringLiteral("%1 失败。")},
        {QStringLiteral("Wallet Overview"), QStringLiteral("钱包概览")},
        {QStringLiteral("Total"), QStringLiteral("总额")},
        {QStringLiteral("Spendable"), QStringLiteral("可用")},
        {QStringLiteral("Immature"), QStringLiteral("未成熟")},
        {QStringLiteral("Pending"), QStringLiteral("待确认")},
        {QStringLiteral("Network"), QStringLiteral("网络")},
        {QStringLiteral("Node"), QStringLiteral("节点")},
        {QStringLiteral("Best Height"), QStringLiteral("最新高度")},
        {QStringLiteral("Wallet"), QStringLiteral("钱包状态")},
        {QStringLiteral("Label"), QStringLiteral("标签")},
        {QStringLiteral("Address"), QStringLiteral("地址")},
        {QStringLiteral("Public Key"), QStringLiteral("公钥")},
        {QStringLiteral("Value"), QStringLiteral("金额")},
        {QStringLiteral("Height"), QStringLiteral("高度")},
        {QStringLiteral("Type"), QStringLiteral("类型")},
        {QStringLiteral("Status"), QStringLiteral("状态")},
        {QStringLiteral("Online (%1 peers, mempool %2)"), QStringLiteral("在线（%1 个节点，内存池 %2）")},
        {QStringLiteral("Offline"), QStringLiteral("离线")},
        {QStringLiteral("Encrypted"), QStringLiteral("已加密")},
        {QStringLiteral("Plaintext"), QStringLiteral("明文")},
        {QStringLiteral("Coinbase"), QStringLiteral("Coinbase")},
        {QStringLiteral("Transfer"), QStringLiteral("转账")},
        {QStringLiteral("Receive"), QStringLiteral("收款")},
        {QStringLiteral("Receive URI"), QStringLiteral("收款 URI")},
        {QStringLiteral("Copy Address"), QStringLiteral("复制地址")},
        {QStringLiteral("Copy Public Key"), QStringLiteral("复制公钥")},
        {QStringLiteral("Copy URI"), QStringLiteral("复制 URI")},
        {QStringLiteral("Save QR PNG"), QStringLiteral("保存二维码 PNG")},
        {QStringLiteral("Create New Address"), QStringLiteral("创建新地址")},
        {QStringLiteral("Passphrase"), QStringLiteral("口令")},
        {QStringLiteral("Create Address"), QStringLiteral("创建地址")},
        {QStringLiteral("No QR image is available yet for the selected address."), QStringLiteral("当前地址还没有可保存的二维码。")},
        {QStringLiteral("Save Receive QR"), QStringLiteral("保存收款二维码")},
        {QStringLiteral("Could not write QR image to %1"), QStringLiteral("无法将二维码写入 %1")},
        {QStringLiteral("Send PAC"), QStringLiteral("发送 PAC")},
        {QStringLiteral("Spendable balance will appear after wallet sync."), QStringLiteral("钱包同步完成后会显示可用余额。")},
        {QStringLiteral("Destination"), QStringLiteral("目标地址")},
        {QStringLiteral("Amount"), QStringLiteral("数量")},
        {QStringLiteral("Fee"), QStringLiteral("手续费")},
        {QStringLiteral("Change Address"), QStringLiteral("找零地址")},
        {QStringLiteral("Send Transaction"), QStringLiteral("发送交易")},
        {QStringLiteral("Use Max Spendable"), QStringLiteral("使用全部可用余额")},
        {QStringLiteral("Automatic wallet change address"), QStringLiteral("自动使用钱包找零地址")},
        {QStringLiteral("Destination and amount are required."), QStringLiteral("目标地址和数量不能为空。")},
        {QStringLiteral("Confirm Transaction"), QStringLiteral("确认交易")},
        {QStringLiteral("Send %1 PAC\n\nTo: %2\nFee: %3 PAC\nChange: %4"), QStringLiteral("发送 %1 PAC\n\n目标：%2\n手续费：%3 PAC\n找零：%4")},
        {QStringLiteral("Automatic"), QStringLiteral("自动")},
        {QStringLiteral("Filter"), QStringLiteral("筛选")},
        {QStringLiteral("All"), QStringLiteral("全部")},
        {QStringLiteral("Received"), QStringLiteral("收入")},
        {QStringLiteral("Sent"), QStringLiteral("支出")},
        {QStringLiteral("Search txid or address"), QStringLiteral("搜索交易哈希或地址")},
        {QStringLiteral("Direction"), QStringLiteral("方向")},
        {QStringLiteral("Addresses"), QStringLiteral("涉及地址")},
        {QStringLiteral("Select a transaction to inspect details."), QStringLiteral("选择一笔交易以查看详情。")},
        {QStringLiteral("Confirmed"), QStringLiteral("已确认")},
        {QStringLiteral("Incoming"), QStringLiteral("转入")},
        {QStringLiteral("Outgoing"), QStringLiteral("转出")},
        {QStringLiteral("TxID: %1\n"), QStringLiteral("交易哈希：%1\n")},
        {QStringLiteral("Confirmations: %1\n"), QStringLiteral("确认数：%1\n")},
        {QStringLiteral("Net: %1 PAC\n\n"), QStringLiteral("净额：%1 PAC\n\n")},
        {QStringLiteral("Inputs:\n"), QStringLiteral("输入：\n")},
        {QStringLiteral("\nOutputs:\n"), QStringLiteral("\n输出：\n")},
        {QStringLiteral("spent"), QStringLiteral("已花费")},
        {QStringLiteral("unspent"), QStringLiteral("未花费")},
        {QStringLiteral("Local Signer Export"), QStringLiteral("本地签名人导出")},
        {QStringLiteral("Copy Export"), QStringLiteral("复制导出内容")},
        {QStringLiteral("Save Export"), QStringLiteral("保存导出内容")},
        {QStringLiteral("Use Local Pubkeys In Preview"), QStringLiteral("使用本地公钥预览")},
        {QStringLiteral("3-of-5 Preview"), QStringLiteral("3-of-5 预览")},
        {QStringLiteral("Required"), QStringLiteral("需要签名数")},
        {QStringLiteral("Public Keys"), QStringLiteral("公钥列表")},
        {QStringLiteral("Preview Multisig Address"), QStringLiteral("预览多签地址")},
        {QStringLiteral("Script Hash"), QStringLiteral("脚本哈希")},
        {QStringLiteral("Redeem Script"), QStringLiteral("赎回脚本")},
        {QStringLiteral("P2SH Script"), QStringLiteral("P2SH 脚本")},
        {QStringLiteral("Copy Scripts"), QStringLiteral("复制脚本")},
        {QStringLiteral("Save Result"), QStringLiteral("保存结果")},
        {QStringLiteral("Save Local Multisig Export"), QStringLiteral("保存本地多签导出")},
        {QStringLiteral("Could not write signer export to %1"), QStringLiteral("无法将签名人导出写入 %1")},
        {QStringLiteral("Generate a multisig preview first."), QStringLiteral("请先生成多签预览。")},
        {QStringLiteral("Save Multisig Preview"), QStringLiteral("保存多签预览")},
        {QStringLiteral("Could not write multisig preview to %1"), QStringLiteral("无法将多签预览写入 %1")},
        {QStringLiteral("Set up Pingancoin Wallet"), QStringLiteral("设置 Pingancoin 钱包")},
        {QStringLiteral("Create a new wallet or restore an existing wallet.json before using the native desktop client."), QStringLiteral("在使用原生桌面客户端前，请先创建新钱包或恢复已有的 wallet.json。")},
        {QStringLiteral("Create New Wallet"), QStringLiteral("创建新钱包")},
        {QStringLiteral("Create Wallet"), QStringLiteral("创建钱包")},
        {QStringLiteral("Restore Existing Wallet"), QStringLiteral("恢复已有钱包")},
        {QStringLiteral("Paste wallet.json contents here"), QStringLiteral("将 wallet.json 内容粘贴到这里")},
        {QStringLiteral("Load wallet.json From File"), QStringLiteral("从文件载入 wallet.json")},
        {QStringLiteral("Overwrite any existing local wallet and archive it first"), QStringLiteral("覆盖当前本地钱包，并先归档旧钱包")},
        {QStringLiteral("Restore Wallet"), QStringLiteral("恢复钱包")},
        {QStringLiteral("Open wallet.json"), QStringLiteral("打开 wallet.json")},
        {QStringLiteral("JSON Files (*.json);;All Files (*)"), QStringLiteral("JSON 文件 (*.json);;所有文件 (*)")},
        {QStringLiteral("Wallet Status"), QStringLiteral("钱包状态")},
        {QStringLiteral("Appearance"), QStringLiteral("外观")},
        {QStringLiteral("Wallet Path"), QStringLiteral("钱包路径")},
        {QStringLiteral("Wallet State"), QStringLiteral("钱包状态")},
        {QStringLiteral("Active Upstream"), QStringLiteral("当前上游节点")},
        {QStringLiteral("Open Wallet Location"), QStringLiteral("打开钱包位置")},
        {QStringLiteral("Open Backup Folder"), QStringLiteral("打开备份目录")},
        {QStringLiteral("Language"), QStringLiteral("语言")},
        {QStringLiteral("Choose how the wallet interface is displayed on this Mac."), QStringLiteral("选择这个 Mac 上钱包界面的显示语言。")},
        {QStringLiteral("Review wallet balances, node status, stored keys, and spendable outputs at a glance."), QStringLiteral("在一个页面里查看钱包余额、节点状态、已保存密钥和可花费输出。")},
        {QStringLiteral("Generate fresh receive addresses, inspect public keys, and export QR codes for payments."), QStringLiteral("生成新的收款地址、查看公钥，并导出收款二维码。")},
        {QStringLiteral("Prepare a payment, choose fee and change behavior, then confirm before broadcasting."), QStringLiteral("准备一笔付款，选择手续费和找零方式，再确认广播。")},
        {QStringLiteral("Search through transfers, coinbase rewards, and raw transaction details in one place."), QStringLiteral("在一个页面里搜索转账、coinbase 奖励和原始交易详情。")},
        {QStringLiteral("Preview the project multisig address, export signer data, and save the final scripts."), QStringLiteral("预览项目多签地址，导出签名人数据，并保存最终脚本。")},
        {QStringLiteral("Switch language, manage upstream nodes, tune the local service, and harden wallet security."), QStringLiteral("切换语言、管理上游节点、调整本地服务，并加强钱包安全。")},
        {QStringLiteral("RPC Upstreams"), QStringLiteral("RPC 上游节点")},
        {QStringLiteral("Known Profiles"), QStringLiteral("已知配置")},
        {QStringLiteral("Use Selected Upstream"), QStringLiteral("使用选中上游")},
        {QStringLiteral("Name"), QStringLiteral("名称")},
        {QStringLiteral("URL"), QStringLiteral("地址")},
        {QStringLiteral("Make active after adding"), QStringLiteral("添加后立即启用")},
        {QStringLiteral("Add Custom Upstream"), QStringLiteral("添加自定义上游")},
        {QStringLiteral("Backend Connection"), QStringLiteral("后端连接")},
        {QStringLiteral("Wallet API URL"), QStringLiteral("钱包 API 地址")},
        {QStringLiteral("Apply URL"), QStringLiteral("应用地址")},
        {QStringLiteral("Local pacwallet Service"), QStringLiteral("本地 pacwallet 服务")},
        {QStringLiteral("Program"), QStringLiteral("程序")},
        {QStringLiteral("Arguments"), QStringLiteral("参数")},
        {QStringLiteral("Start Backend"), QStringLiteral("启动后端")},
        {QStringLiteral("Stop Backend"), QStringLiteral("停止后端")},
        {QStringLiteral("Wallet Security"), QStringLiteral("钱包安全")},
        {QStringLiteral("New Passphrase"), QStringLiteral("新口令")},
        {QStringLiteral("Encrypt Wallet"), QStringLiteral("加密钱包")},
        {QStringLiteral("Current Passphrase"), QStringLiteral("当前口令")},
        {QStringLiteral("Replacement Passphrase"), QStringLiteral("替换口令")},
        {QStringLiteral("Change Passphrase"), QStringLiteral("修改口令")},
        {QStringLiteral("Import Private Key"), QStringLiteral("导入私钥")},
        {QStringLiteral("Private Key Hex"), QStringLiteral("私钥 Hex")},
        {QStringLiteral("Import Key"), QStringLiteral("导入私钥")},
        {QStringLiteral("Archived Backups"), QStringLiteral("归档备份")},
        {QStringLiteral("Service Logs"), QStringLiteral("服务日志")},
        {QStringLiteral("Ready"), QStringLiteral("可用")},
        {QStringLiteral("No wallet created yet"), QStringLiteral("尚未创建钱包")},
        {QStringLiteral("%1 at height %2 (%3 peers, mempool %4)"), QStringLiteral("%1，高度 %2（%3 个节点，内存池 %4）")},
        {QStringLiteral(" - %1"), QStringLiteral(" - %1")},
        {QStringLiteral("%1  (%2)"), QStringLiteral("%1（%2）")},
        {QStringLiteral("  [active]"), QStringLiteral(" [当前]")},
        {QStringLiteral("%1  (%2 bytes)"), QStringLiteral("%1（%2 字节）")},
        {QStringLiteral("Receive addresses, public keys, and QR codes stay in one place here."), QStringLiteral("这里集中展示收款地址、公钥和二维码。")},
        {QStringLiteral("Create a fresh address for a new customer, invoice, or bookkeeping label."), QStringLiteral("可以为新的客户、账单或记账标签生成一条全新的地址。")},
        {QStringLiteral("Review the destination, fee, and change output before broadcasting."), QStringLiteral("广播之前，先检查目标地址、手续费和找零输出。")},
        {QStringLiteral("Coinbase rewards stay immature for a while before they become spendable."), QStringLiteral("Coinbase 奖励需要经过一段成熟期后才能花费。")},
        {QStringLiteral("Search across txid and related addresses, then inspect the full inputs and outputs."), QStringLiteral("可以按交易哈希和相关地址搜索，再查看完整的输入输出详情。")},
        {QStringLiteral("Export local signer data and preview the final 3-of-5 address before mainnet launch."), QStringLiteral("导出本地签名人信息，并在主网上线前预览最终的 3-of-5 地址。")},
        {QStringLiteral("Create a new wallet on this Mac, or restore an existing wallet.json to continue from another machine."), QStringLiteral("在这台 Mac 上创建新钱包，或恢复已有的 wallet.json，从另一台设备继续使用。")},
        {QStringLiteral("Open"), QStringLiteral("打开")},
        {QStringLiteral("Wallet Health"), QStringLiteral("钱包健康状态")},
        {QStringLiteral("Need a cleaner send flow?"), QStringLiteral("发送前先确认一下？")},
        {QStringLiteral("Create and restore"), QStringLiteral("创建与恢复")},
        {QStringLiteral("Change language"), QStringLiteral("切换语言")},
        {QStringLiteral("English"), QStringLiteral("英文")},
        {QStringLiteral("Simplified Chinese"), QStringLiteral("简体中文")}
    };
}

} // namespace

QString defaultLanguageCode()
{
    return QLocale::system().name().startsWith(QStringLiteral("zh")) ? QStringLiteral("zh_CN") : QStringLiteral("en");
}

QString currentLanguageCode()
{
    return g_languageCode;
}

void setCurrentLanguageCode(const QString &code)
{
    g_languageCode = code.startsWith(QStringLiteral("zh")) ? QStringLiteral("zh_CN") : QStringLiteral("en");
}

QString text(const QString &source)
{
    if (g_languageCode != QStringLiteral("zh_CN")) {
        return source;
    }
    static const QMap<QString, QString> map = zhTranslations();
    return map.value(source, source);
}

QString languageDisplayName(const QString &code)
{
    if (code.startsWith(QStringLiteral("zh"))) {
        return text(QStringLiteral("Simplified Chinese"));
    }
    return text(QStringLiteral("English"));
}

} // namespace pacqt::l10n
