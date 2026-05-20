#pragma once

#include <QObject>
#include <QProcess>
#include <QStringList>

namespace pacqt {

class ServiceController : public QObject
{
    Q_OBJECT

public:
    explicit ServiceController(QObject *parent = nullptr);

    void setProgram(const QString &program);
    void setArguments(const QStringList &arguments);
    QString program() const;
    QStringList arguments() const;
    bool isRunning() const;

public slots:
    void start();
    void stop();

signals:
    void serviceStarted();
    void serviceStopped();
    void serviceLog(const QString &line);
    void serviceError(const QString &message);

private:
    QProcess m_process;
    QString m_program;
    QStringList m_arguments;
    bool m_stopping = false;
};

} // namespace pacqt
