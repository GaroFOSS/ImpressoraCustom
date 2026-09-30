#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QTimer>

class PrinterController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
public:
    explicit PrinterController(QObject *parent = nullptr);
    // Endpoint/timeout injection is for local integration tests; the UI uses the defaults.
    PrinterController(const QString &host, quint16 port, int timeoutMs,
                      QObject *parent = nullptr);
    bool busy() const { return m_busy; }
    QString status() const { return m_status; }
    Q_INVOKABLE void printText(const QString &text, int size, int alignment);
signals:
    void busyChanged();
    void statusChanged();
private:
    void sendPending();
    void finish(const QString &message, bool success);
    void setStatus(const QString &message);
    QTcpSocket m_socket;
    QTimer m_timer;
    QString m_host;
    quint16 m_port;
    int m_timeoutMs;
    QByteArray m_payload;
    qint64 m_queued = 0;
    bool m_busy = false;
    QString m_status = QStringLiteral("Ready");
};
