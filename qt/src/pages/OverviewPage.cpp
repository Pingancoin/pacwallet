#include "OverviewPage.h"
#include "../Localization.h"

#include <QGroupBox>
#include <QHeaderView>
#include <QGridLayout>
#include <QSplitter>
#include <QVBoxLayout>

namespace pacqt {

OverviewPage::OverviewPage(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(12);

    m_summaryBox = new QGroupBox(QStringLiteral("Wallet Overview"), this);
    auto *summaryGrid = new QGridLayout(m_summaryBox);
    summaryGrid->setHorizontalSpacing(10);
    summaryGrid->setVerticalSpacing(10);
    m_totalLabel = new QLabel(this);
    m_spendableLabel = new QLabel(this);
    m_immatureLabel = new QLabel(this);
    m_pendingLabel = new QLabel(this);
    m_nodeLabel = new QLabel(this);
    m_heightLabel = new QLabel(this);
    m_networkLabel = new QLabel(this);
    m_walletStateLabel = new QLabel(this);

    const QStringList metricNames{
        QStringLiteral("Total"),
        QStringLiteral("Spendable"),
        QStringLiteral("Immature"),
        QStringLiteral("Pending"),
        QStringLiteral("Network"),
        QStringLiteral("Node"),
        QStringLiteral("Best Height"),
        QStringLiteral("Wallet")
    };
    const QList<QLabel *> metricValues{
        m_totalLabel,
        m_spendableLabel,
        m_immatureLabel,
        m_pendingLabel,
        m_networkLabel,
        m_nodeLabel,
        m_heightLabel,
        m_walletStateLabel
    };
    for (int i = 0; i < metricNames.size(); ++i) {
        auto *card = new QWidget(this);
        card->setObjectName(QStringLiteral("overviewMetricCard"));
        auto *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(12, 10, 12, 10);
        cardLayout->setSpacing(4);
        auto *nameLabel = new QLabel(metricNames.at(i), this);
        nameLabel->setObjectName(QStringLiteral("overviewMetricName"));
        auto *valueLabel = metricValues.at(i);
        valueLabel->setObjectName(QStringLiteral("overviewMetricValue"));
        valueLabel->setWordWrap(true);
        cardLayout->addWidget(nameLabel);
        cardLayout->addWidget(valueLabel);
        cardLayout->addStretch(1);
        m_metricNameLabels.push_back(nameLabel);
        m_metricValueLabels.push_back(valueLabel);
        summaryGrid->addWidget(card, i / 4, i % 4);
    }
    for (int column = 0; column < 4; ++column) {
        summaryGrid->setColumnStretch(column, 1);
    }
    m_summaryBox->setStyleSheet(QStringLiteral(
        "QWidget#overviewMetricCard { background: #f8fbff; border: 1px solid #dbe4f0; border-radius: 10px; }"
        "QLabel#overviewMetricName { color: #64748b; font-size: 12px; font-weight: 600; }"
        "QLabel#overviewMetricValue { color: #0f172a; font-size: 15px; font-weight: 700; }"));

    auto *splitter = new QSplitter(Qt::Vertical, this);
    m_keysTable = new QTableWidget(this);
    m_keysTable->setColumnCount(3);
    m_keysTable->setHorizontalHeaderLabels({QStringLiteral("Label"), QStringLiteral("Address"), QStringLiteral("Public Key")});
    m_keysTable->horizontalHeader()->setStretchLastSection(true);
    m_keysTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_keysTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_keysTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_keysTable->setAlternatingRowColors(true);

    m_utxoTable = new QTableWidget(this);
    m_utxoTable->setColumnCount(5);
    m_utxoTable->setHorizontalHeaderLabels({QStringLiteral("Address"), QStringLiteral("Value"), QStringLiteral("Height"), QStringLiteral("Type"), QStringLiteral("Status")});
    m_utxoTable->horizontalHeader()->setStretchLastSection(true);
    m_utxoTable->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_utxoTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_utxoTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_utxoTable->setAlternatingRowColors(true);
    m_keysTable->verticalHeader()->setDefaultSectionSize(32);
    m_utxoTable->verticalHeader()->setDefaultSectionSize(32);

    splitter->addWidget(m_keysTable);
    splitter->addWidget(m_utxoTable);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);

    layout->addWidget(m_summaryBox);
    layout->addWidget(splitter, 1);
    retranslateUi();
}

void OverviewPage::setOverview(const pacqt::Overview &overview)
{
    m_overview = overview;
    m_hasOverview = true;
    m_totalLabel->setText(formatPac(overview.balance.total) + QStringLiteral(" PAC"));
    m_spendableLabel->setText(formatPac(overview.balance.spendable) + QStringLiteral(" PAC"));
    m_immatureLabel->setText(formatPac(overview.balance.immature) + QStringLiteral(" PAC"));
    m_pendingLabel->setText(formatPac(overview.balance.pending) + QStringLiteral(" PAC"));
    m_networkLabel->setText(overview.wallet.network);
    m_nodeLabel->setText(overview.node.online
            ? l10n::text(QStringLiteral("Online (%1 peers, mempool %2)")).arg(QString::number(overview.node.peerCount), QString::number(overview.node.mempoolSize))
            : l10n::text(QStringLiteral("Offline")));
    m_heightLabel->setText(QString::number(overview.node.bestHeight));
    m_walletStateLabel->setText(overview.wallet.encrypted ? l10n::text(QStringLiteral("Encrypted")) : l10n::text(QStringLiteral("Plaintext")));

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
        const QString type = utxo.coinbase ? l10n::text(QStringLiteral("Coinbase")) : l10n::text(QStringLiteral("Transfer"));
        QString status = utxo.pending ? l10n::text(QStringLiteral("Pending")) : l10n::text(QStringLiteral("Spendable"));
        if (!utxo.pending && !utxo.mature) {
            status = l10n::text(QStringLiteral("Immature"));
        }
        m_utxoTable->setItem(i, 0, new QTableWidgetItem(utxo.address));
        m_utxoTable->setItem(i, 1, new QTableWidgetItem(formatPac(utxo.value) + QStringLiteral(" PAC")));
        m_utxoTable->setItem(i, 2, new QTableWidgetItem(QString::number(utxo.height)));
        m_utxoTable->setItem(i, 3, new QTableWidgetItem(type));
        m_utxoTable->setItem(i, 4, new QTableWidgetItem(status));
    }
}

void OverviewPage::retranslateUi()
{
    m_summaryBox->setTitle(l10n::text(QStringLiteral("Wallet Overview")));
    const QStringList rowLabels{
        l10n::text(QStringLiteral("Total")),
        l10n::text(QStringLiteral("Spendable")),
        l10n::text(QStringLiteral("Immature")),
        l10n::text(QStringLiteral("Pending")),
        l10n::text(QStringLiteral("Network")),
        l10n::text(QStringLiteral("Node")),
        l10n::text(QStringLiteral("Best Height")),
        l10n::text(QStringLiteral("Wallet"))
    };
    for (int row = 0; row < rowLabels.size() && row < m_metricNameLabels.size(); ++row) {
        m_metricNameLabels.at(row)->setText(rowLabels.at(row));
    }
    m_keysTable->setHorizontalHeaderLabels({
        l10n::text(QStringLiteral("Label")),
        l10n::text(QStringLiteral("Address")),
        l10n::text(QStringLiteral("Public Key"))
    });
    m_utxoTable->setHorizontalHeaderLabels({
        l10n::text(QStringLiteral("Address")),
        l10n::text(QStringLiteral("Value")),
        l10n::text(QStringLiteral("Height")),
        l10n::text(QStringLiteral("Type")),
        l10n::text(QStringLiteral("Status"))
    });
    if (m_hasOverview) {
        setOverview(m_overview);
    }
}

} // namespace pacqt
