#pragma once

#include "../Models.h"

#include <QLabel>
#include <QSpinBox>
#include <QTextEdit>
#include <QWidget>

namespace pacqt {

class MultisigPage : public QWidget
{
    Q_OBJECT

public:
    explicit MultisigPage(QWidget *parent = nullptr);
    void setOverview(const pacqt::Overview &overview);
    void setPreviewResult(const pacqt::MultiSigPreviewResult &result);

signals:
    void previewRequested(int required, const QStringList &pubKeys);

private:
    QTextEdit *m_localExport;
    QTextEdit *m_pubKeysEdit;
    QSpinBox *m_requiredSpin;
    QLabel *m_addressLabel;
    QLabel *m_redeemLabel;
};

} // namespace pacqt
