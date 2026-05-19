#include "TransactionsPage.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QSplitter>
#include <QVBoxLayout>

namespace pacqt {

TransactionsPage::TransactionsPage(QWidget *parent)
    : QWidget(parent)
{
    auto *layout = new QVBoxLayout(this);
    auto *toolbar = new QHBoxLayout();
    m_filterCombo = new QComboBox(this);
    m_filterCombo->addItems({QStringLiteral("All"), QStringLiteral("Received"), QStringLiteral("Sent"), QStringLiteral("Coinbase"), QStringLiteral("Pending")});
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(QStringLiteral("Search txid or address"));
    toolbar->addWidget(new QLabel(QStringLiteral("Filter"), this));
    toolbar->addWidget(m_filterCombo);
    toolbar->addWidget(m_searchEdit, 1);

    auto *splitter = new QSplitter(Qt::Horizontal, this);

    m_table = new QTableWidget(this);
    m_table->setColumnCount(6);
    m_table->setHorizontalHeaderLabels({QStringLiteral("TxID"), QStringLiteral("Height"), QStringLiteral("Status"), QStringLiteral("Direction"), QStringLiteral("Net"), QStringLiteral("Addresses")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);

    m_detailView = new QTextEdit(this);
    m_detailView->setReadOnly(true);
    m_detailView->setPlainText(QStringLiteral("Select a transaction to inspect details."));

    splitter->addWidget(m_table);
    splitter->addWidget(m_detailView);
    splitter->setStretchFactor(0, 3);
    splitter->setStretchFactor(1, 2);

    layout->addLayout(toolbar);
    layout->addWidget(splitter);

    connect(m_filterCombo, &QComboBox::currentTextChanged, this, [this]() {
        refreshDisplayedHistory();
    });
    connect(m_searchEdit, &QLineEdit::textChanged, this, [this]() {
        refreshDisplayedHistory();
    });
    connect(m_table, &QTableWidget::cellClicked, this, [this](int row, int) {
        const QTableWidgetItem *item = m_table->item(row, 0);
        if (item) {
            emit transactionSelected(item->text());
        }
    });
}

void TransactionsPage::setOverview(const pacqt::Overview &overview)
{
    m_history = overview.history;
    refreshDisplayedHistory();
}

void TransactionsPage::setTransactionDetail(const pacqt::TransactionDetail &detail)
{
    QString text;
    text += QStringLiteral("TxID: %1\n").arg(detail.txHash);
    text += QStringLiteral("Confirmations: %1\n").arg(detail.confirmations);
    text += QStringLiteral("Net: %1 PAC\n\n").arg(formatPac(detail.net));
    text += QStringLiteral("Inputs:\n");
    for (const TxInputDetail &input : detail.inputs) {
        text += QStringLiteral("- %1:%2  %3  %4 PAC\n")
                    .arg(input.prevTxHash, QString::number(input.prevVout), input.address, formatPac(input.value));
    }
    text += QStringLiteral("\nOutputs:\n");
    for (const TxOutputDetail &output : detail.outputs) {
        text += QStringLiteral("- #%1  %2  %3 PAC  %4\n")
                    .arg(output.index)
                    .arg(output.address)
                    .arg(formatPac(output.value))
                    .arg(output.spent ? QStringLiteral("spent") : QStringLiteral("unspent"));
    }
    m_detailView->setPlainText(text);
}

void TransactionsPage::refreshDisplayedHistory()
{
    const QString filter = m_filterCombo->currentText();
    const QString needle = m_searchEdit->text().trimmed().toLower();

    QVector<HistoryEntry> displayed;
    for (const HistoryEntry &entry : m_history) {
        bool keep = true;
        if (filter == QStringLiteral("Received")) {
            keep = entry.net > 0;
        } else if (filter == QStringLiteral("Sent")) {
            keep = entry.net < 0;
        } else if (filter == QStringLiteral("Coinbase")) {
            keep = entry.coinbase;
        } else if (filter == QStringLiteral("Pending")) {
            keep = entry.pending;
        }

        if (keep && !needle.isEmpty()) {
            const QString haystack = (entry.txHash + QStringLiteral(" ") + entry.addresses.join(QStringLiteral(" "))).toLower();
            keep = haystack.contains(needle);
        }
        if (keep) {
            displayed.push_back(entry);
        }
    }

    m_table->setRowCount(displayed.size());
    for (int i = 0; i < displayed.size(); ++i) {
        const HistoryEntry &entry = displayed.at(i);
        const QString direction = entry.coinbase ? QStringLiteral("Coinbase") : (entry.net >= 0 ? QStringLiteral("Incoming") : QStringLiteral("Outgoing"));
        m_table->setItem(i, 0, new QTableWidgetItem(entry.txHash));
        m_table->setItem(i, 1, new QTableWidgetItem(QString::number(entry.height)));
        m_table->setItem(i, 2, new QTableWidgetItem(entry.pending ? QStringLiteral("Pending") : QStringLiteral("Confirmed")));
        m_table->setItem(i, 3, new QTableWidgetItem(direction));
        m_table->setItem(i, 4, new QTableWidgetItem(formatPac(entry.net) + QStringLiteral(" PAC")));
        m_table->setItem(i, 5, new QTableWidgetItem(entry.addresses.join(QStringLiteral(", "))));
    }
}

} // namespace pacqt
