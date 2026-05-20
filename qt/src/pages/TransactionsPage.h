#pragma once

#include "../Models.h"

#include <QComboBox>
#include <QLabel>
#include <QLineEdit>
#include <QTableWidget>
#include <QTextEdit>
#include <QWidget>

namespace pacqt {

class TransactionsPage : public QWidget
{
    Q_OBJECT

public:
    explicit TransactionsPage(QWidget *parent = nullptr);
    void setOverview(const pacqt::Overview &overview);
    void setTransactionDetail(const pacqt::TransactionDetail &detail);
    void retranslateUi();

signals:
    void transactionSelected(const QString &txHash);

private:
    void refreshDisplayedHistory();

    QVector<HistoryEntry> m_history;
    bool m_hasDetail = false;
    pacqt::TransactionDetail m_detail;
    QComboBox *m_filterCombo;
    QLineEdit *m_searchEdit;
    QTableWidget *m_table;
    QTextEdit *m_detailView;
};

} // namespace pacqt
