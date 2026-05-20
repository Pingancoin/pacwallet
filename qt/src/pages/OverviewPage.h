#pragma once

#include "../Models.h"

#include <QGroupBox>
#include <QLabel>
#include <QTableWidget>
#include <QWidget>

namespace pacqt {

class OverviewPage : public QWidget
{
    Q_OBJECT

public:
    explicit OverviewPage(QWidget *parent = nullptr);
    void setOverview(const pacqt::Overview &overview);
    void retranslateUi();

private:
    bool m_hasOverview = false;
    pacqt::Overview m_overview;
    QGroupBox *m_summaryBox;
    QVector<QLabel *> m_metricNameLabels;
    QVector<QLabel *> m_metricValueLabels;
    QLabel *m_totalLabel;
    QLabel *m_spendableLabel;
    QLabel *m_immatureLabel;
    QLabel *m_pendingLabel;
    QLabel *m_nodeLabel;
    QLabel *m_heightLabel;
    QLabel *m_networkLabel;
    QLabel *m_walletStateLabel;
    QTableWidget *m_keysTable;
    QTableWidget *m_utxoTable;
};

} // namespace pacqt
