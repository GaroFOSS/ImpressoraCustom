#include "printercontroller.h"
#include "escpos.h"

PrinterController::PrinterController(QObject *parent)
    : PrinterController(QStringLiteral("192.168.0.113"), 9100, 10000, parent)
{
}

PrinterController::PrinterController(const QString &host, quint16 port, int timeoutMs,
                                     QObject *parent)
    : QObject(parent), m_host(host), m_port(port), m_timeoutMs(timeoutMs)
{
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout, this, [this] {
        if (m_busy)
            finish(QStringLiteral("Print timed out. Check the printer and network."), false);
    });
    connect(&m_socket, &QTcpSocket::connected, this, [this] {
        if (m_busy) {
            setStatus(QStringLiteral("Sending…"));
            sendPending();
        }
    });
    connect(&m_socket, &QTcpSocket::bytesWritten, this, [this](qint64) {
        sendPending();
    });
    connect(&m_socket, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        if (m_busy)
            finish(QStringLiteral("Print failed: %1").arg(m_socket.errorString()), false);
    });
    connect(&m_socket, &QTcpSocket::disconnected, this, [this] {
        if (m_busy)
            finish(QStringLiteral("Printer disconnected before transmission completed."), false);
    });
}

void PrinterController::setStatus(const QString &message)
{
    if (m_status != message) {
        m_status = message;
        emit statusChanged();
    }
}

void PrinterController::printText(const QString &text, int size, int alignment)
{
    if (m_busy)
        return;
    QString error;
    const QByteArray payload = EscPos::encode(text, size, alignment, &error);
    if (!error.isEmpty()) {
        setStatus(error);
        return;
    }
    m_socket.abort();
    m_payload = payload;
    m_queued = 0;
    m_busy = true;
    emit busyChanged();
    setStatus(QStringLiteral("Connecting…"));
    m_timer.start(m_timeoutMs);
    m_socket.connectToHost(m_host, m_port);
}

void PrinterController::sendPending()
{
    if (!m_busy || m_socket.state() != QAbstractSocket::ConnectedState)
        return;
    if (m_queued < m_payload.size()) {
        const qint64 accepted = m_socket.write(m_payload.constData() + m_queued,
                                               m_payload.size() - m_queued);
        if (accepted < 0) {
            finish(QStringLiteral("Print failed: %1").arg(m_socket.errorString()), false);
            return;
        }
        m_queued += accepted;
    }
    if (m_queued == m_payload.size() && m_socket.bytesToWrite() == 0)
        finish(QStringLiteral("Sent to printer"), true);
}

void PrinterController::finish(const QString &message, bool success)
{
    m_timer.stop();
    m_busy = false;
    m_payload.clear();
    m_queued = 0;
    if (success)
        m_socket.disconnectFromHost();
    else
        m_socket.abort();
    setStatus(message);
    emit busyChanged();
}
