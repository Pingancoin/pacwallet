#pragma once

#include "../Models.h"

#include <QGroupBox>
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
    void retranslateUi();

signals:
    void previewRequested(int required, const QStringList &pubKeys);

private:
    bool m_hasResult = false;
    pacqt::MultiSigPreviewResult m_result;
    QTextEdit *m_localExport;
    QTextEdit *m_pubKeysEdit;
    QSpinBox *m_requiredSpin;
    QLabel *m_addressLabel;
    QLabel *m_scriptHashLabel;
    QLabel *m_redeemLabel;
    QLabel *m_p2shScriptLabel;
    QGroupBox *m_localBox;
    QGroupBox *m_previewBox;
};

} // namespace pacqt
