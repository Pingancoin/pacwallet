#pragma once

#include "../Models.h"

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

private:
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
