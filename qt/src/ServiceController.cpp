#include "ServiceController.h"

#include <QFileInfo>

namespace pacqt {

ServiceController::ServiceController(QObject *parent)
    : QObject(parent)
{
    connect(&m_process, &QProcess::started, this, &ServiceController::serviceStarted);
    connect(&m_process, &QProcess::readyReadStandardOutput, this, [this]() {
        emit serviceLog(QString::fromUtf8(m_process.readAllStandardOutput()));
    });
    connect(&m_process, &QProcess::readyReadStandardError, this, [this]() {
        emit serviceLog(QString::fromUtf8(m_process.readAllStandardError()));
    });
    connect(&m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError) {
        emit serviceError(m_process.errorString());
    });
    connect(&m_process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), this, [this]() {
        emit serviceStopped();
    });
}

void ServiceController::setProgram(const QString &program)
{
    m_program = program;
}

void ServiceController::setArguments(const QStringList &arguments)
{
    m_arguments = arguments;
}

QString ServiceController::program() const
{
    return m_program;
}

QStringList ServiceController::arguments() const
{
    return m_arguments;
}

bool ServiceController::isRunning() const
{
    return m_process.state() != QProcess::NotRunning;
}

void ServiceController::start()
{
    if (m_program.isEmpty()) {
        emit serviceError(QStringLiteral("No pacwallet backend program configured."));
        return;
    }
    if (isRunning()) {
        return;
    }
    const QFileInfo programInfo(m_program);
    if (programInfo.exists()) {
        m_process.setWorkingDirectory(programInfo.absolutePath());
    }
    m_process.start(m_program, m_arguments);
}

void ServiceController::stop()
{
    if (!isRunning()) {
        return;
    }
    m_process.terminate();
    if (!m_process.waitForFinished(3000)) {
        m_process.kill();
    }
}

} // namespace pacqt
