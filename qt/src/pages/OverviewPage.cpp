#include "OverviewPage.h"

#include <QFormLayout>
#include <QGroupBox>
#include <QHeaderView>
#include <QSplitter>
#include <QVBoxLayout>

namespace pacqt {

OverviewPage::OverviewPage(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);

    auto *summaryBox = new QGroupBox(QStringLiteral("Wallet Overview"), this);
    auto *summaryLayout = new QFormLayout(summaryBox);
    m_totalLabel = new QLabel(this);
    m_spendableLabel = new QLabel(this);
    m_immatureLabel = new QLabel(this);
    m_pendingLabel = new QLabel(this);
    m_nodeLabel = new QLabel(this);
    m_heightLabel = new QLabel(this);
    m_networkLabel = new QLabel(this);
    m_walletStateLabel = new QLabel(this);

    summaryLayout->addRow(QStringLiteral("Total"), m_totalLabel);
    summaryLayout->addRow(QStringLiteral("Spendable"), m_spendableLabel);
    summaryLayout->addRow(QStringLiteral("Immature"), m_immatureLabel);
    summaryLayout->addRow(QStringLiteral("Pending"), m_pendingLabel);
    summaryLayout->addRow(QStringLiteral("Network"), m_networkLabel);
    summaryLayout->addRow(QStringLiteral("Node"), m_nodeLabel);
    summaryLayout->addRow(QStringLiteral("Best Height"), m_heightLabel);
    summaryLayout->addRow(QStringLiteral("Wallet"), m_walletStateLabel);

    auto *splitter = new QSplitter(Qt::Vertical, this);
    m_keysTable = new QTableWidget(this);
    m_keysTable->setColumnCount(3);
    m_keysTable->setHorizontalHeaderLabels({QStringLiteral("Label"), QStringLiteral("Address"), QStringLiteral("Public Key")});
    m_keysTable->horizontalHeader()->setStretchLastSection(true);
    m_keysTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_keysTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_keysTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    m_utxoTable = new QTableWidget(this);
    m_utxoTable->setColumnCount(5);
    m_utxoTable->setHorizontalHeaderLabels({QStringLiteral("Address"), QStringLiteral("Value"), QStringLiteral("Height"), QStringLiteral("Type"), QStringLiteral("Status")});
    m_utxoTable->horizontalHeader()->setStretchLastSection(true);
    m_utxoTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_utxoTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_utxoTable->setSelectionBehavior(QAbstractItemView::SelectRows);

    splitter->addWidget(m_keysTable);
    splitter->addWidget(m_utxoTable);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);

    layout->addWidget(summaryBox);
    layout->addWidget(splitter, 1);
}

void OverviewPage::setOverview(const pacqt::Overview &overview)
{
    m_totalLabel->setText(formatPac(overview.balance.total) + QStringLiteral(" PAC"));
    m_spendableLabel->setText(formatPac(overview.balance.spendable) + QStringLiteral(" PAC"));
    m_immatureLabel->setText(formatPac(overview.balance.immature) + QStringLiteral(" PAC"));
    m_pendingLabel->setText(formatPac(overview.balance.pending) + QStringLiteral(" PAC"));
    m_networkLabel->setText(overview.wallet.network);
    m_nodeLabel->setText(overview.node.online
            ? QStringLiteral("Online (%1 peers, mempool %2)").arg(QString::number(overview.node.peerCount), QString::number(overview.node.mempoolSize))
            : QStringLiteral("Offline"));
    m_heightLabel->setText(QString::number(overview.node.bestHeight));
    m_walletStateLabel->setText(overview.wallet.encrypted ? QStringLiteral("Encrypted") : QStringLiteral("Plaintext"));

    m_keysTable->setRowCount(overview.wallet.keys.size());
    for (int i = 0; i < overview.wallet.keys.size(); ++i) {
        const KeySummary &key = overview.wallet.keys.at(i);
        m_keysTable->setItem(i, 0, new QTableWidgetItem(key.label));
        m_keysTable->setItem(i, 1, new QTableWidgetItem(key.address));
        m_keysTable->setItem(i, 2, new QTableWidgetItem(key.pubKeyHex));
    }

    m_utxoTable->setRowCount(overview.balance.utxos.size());
    for (int i = 0; i < overview.balance.utxos.size(); ++i) {
        const Utxo &utxo = overview.balance.utxos.at(i);
        const QString type = utxo.coinbase ? QStringLiteral("Coinbase") : QStringLiteral("Transfer");
        QString status = utxo.pending ? QStringLiteral("Pending") : QStringLiteral("Spendable");
        if (!utxo.pending && !utxo.mature) {
            status = QStringLiteral("Immature");
        }
        m_utxoTable->setItem(i, 0, new QTableWidgetItem(utxo.address));
        m_utxoTable->setItem(i, 1, new QTableWidgetItem(formatPac(utxo.value) + QStringLiteral(" PAC")));
        m_utxoTable->setItem(i, 2, new QTableWidgetItem(QString::number(utxo.height)));
        m_utxoTable->setItem(i, 3, new QTableWidgetItem(type));
        m_utxoTable->setItem(i, 4, new QTableWidgetItem(status));
    }
}

} // namespace pacqt
