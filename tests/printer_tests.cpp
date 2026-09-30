#include "escpos.h"
#include "printercontroller.h"

#include <QTcpServer>
#include <QtTest>
#include <memory>

class PrinterTests : public QObject
{
    Q_OBJECT
private slots:
    void formatting_data()
    {
        QTest::addColumn<int>("size");
        QTest::addColumn<int>("alignment");
        for (int size = 0; size < 3; ++size)
            for (int alignment = 0; alignment < 3; ++alignment)
                QTest::newRow(qPrintable(QStringLiteral("%1-%2").arg(size).arg(alignment)))
                    << size << alignment;
    }

    void formatting()
    {
        QFETCH(int, size);
        QFETCH(int, alignment);
        QString error;
        const auto actual = EscPos::encode(QStringLiteral("Hello"), size, alignment, &error);
        QByteArray expected = QByteArray::fromHex("1b401c2e1b74021b4d001d21");
        expected.append(static_cast<char>(size * 0x11));
        expected += QByteArray::fromHex("1b61");
        expected.append(static_cast<char>(alignment));
        expected += "Hello\n";
        expected += QByteArray::fromHex("1d21001b64031d5601");
        QCOMPARE(error, QString());
        QCOMPARE(actual, expected);
    }

    void accentsAndNewlines()
    {
        QString error;
        const auto encoded = EscPos::encode(QString::fromUtf8("ação, café, maçã, São Paulo\r\nnext\rlast\n"),
                                            0, 0, &error);
        QVERIFY(error.isEmpty());
        const QByteArray expectedText = QByteArray::fromHex(
            "6187c66f2c20636166822c206d6187c62c2053c66f205061756c6f0a6e6578740a6c6173740a");
        QCOMPARE(encoded.mid(16, expectedText.size()), expectedText);
        QCOMPARE(EscPos::encode(QString::fromUtf8("cafe\u0301"), 0, 0, &error),
                 EscPos::encode(QString::fromUtf8("café"), 0, 0, &error));
    }

    void invalidInput_data()
    {
        QTest::addColumn<QString>("text");
        QTest::addColumn<int>("size");
        QTest::addColumn<int>("alignment");
        QTest::newRow("blank") << QStringLiteral(" \n\r ") << 0 << 0;
        QTest::newRow("emoji") << QString::fromUtf8("Hello 😀") << 0 << 0;
        QTest::newRow("escape") << QStringLiteral("Hello\x1b@") << 0 << 0;
        QTest::newRow("nul") << (QStringLiteral("Hello") + QChar(0)) << 0 << 0;
        QTest::newRow("tab") << QStringLiteral("Hello\tthere") << 0 << 0;
        QTest::newRow("invalid-size") << QStringLiteral("Hello") << 3 << 0;
        QTest::newRow("invalid-alignment") << QStringLiteral("Hello") << 0 << -1;
    }

    void invalidInput()
    {
        QFETCH(QString, text);
        QFETCH(int, size);
        QFETCH(int, alignment);
        QString error;
        QVERIFY(EscPos::encode(text, size, alignment, &error).isEmpty());
        QVERIFY(!error.isEmpty());
    }

    void networkDeliveryAndDuplicateClick()
    {
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost, 0));
        PrinterController printer(QStringLiteral("127.0.0.1"), server.serverPort(), 10000);
        // A large job also checks transmission across multiple TCP packets.
        const QString text = QString(200000, QChar('A')) + QString::fromUtf8("ação");
        QString error;
        const QByteArray expected = EscPos::encode(text, 2, 1, &error);
        printer.printText(text, 2, 1);
        QVERIFY(printer.busy());
        printer.printText(QStringLiteral("Duplicate"), 0, 0);
        QTRY_VERIFY(server.hasPendingConnections());
        std::unique_ptr<QTcpSocket> peer(server.nextPendingConnection());
        QByteArray received;
        connect(peer.get(), &QTcpSocket::readyRead, this, [&] { received += peer->readAll(); });
        received += peer->readAll();
        QTRY_COMPARE_WITH_TIMEOUT(received.size(), expected.size(), 10000);
        QCOMPARE(received, expected);
        QTRY_VERIFY(!printer.busy());
        QCOMPARE(printer.status(), QStringLiteral("Sent to printer"));
        QVERIFY(!server.hasPendingConnections());

        // A second intentional click must create a fresh job and connection.
        printer.printText(QStringLiteral("Second"), 0, 2);
        QTRY_VERIFY(server.hasPendingConnections());
        std::unique_ptr<QTcpSocket> second(server.nextPendingConnection());
        QByteArray secondReceived;
        connect(second.get(), &QTcpSocket::readyRead, this,
                [&] { secondReceived += second->readAll(); });
        secondReceived += second->readAll();
        const auto secondExpected = EscPos::encode(QStringLiteral("Second"), 0, 2, &error);
        QTRY_COMPARE(secondReceived, secondExpected);
        QTRY_VERIFY(!printer.busy());
    }

    void validationDoesNotConnect()
    {
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost, 0));
        PrinterController printer(QStringLiteral("127.0.0.1"), server.serverPort(), 1000);
        printer.printText(QString::fromUtf8("😀"), 0, 0);
        QVERIFY(!printer.busy());
        QVERIFY(printer.status().contains(QStringLiteral("CP850")));
        QVERIFY(!server.hasPendingConnections());
    }

    void connectionFailure()
    {
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost, 0));
        const auto port = server.serverPort();
        server.close();
        PrinterController printer(QStringLiteral("127.0.0.1"), port, 1000);
        printer.printText(QStringLiteral("Hello"), 0, 0);
        QTRY_VERIFY(!printer.busy());
        QVERIFY(printer.status().startsWith(QStringLiteral("Print failed:")));
    }

    void timeout()
    {
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost, 0));
        // Do not consume data on the server: the job exceeds TCP buffering and
        // must time out while sending even if the connection succeeds first.
        PrinterController printer(QStringLiteral("127.0.0.1"), server.serverPort(), 5);
        printer.printText(QString(8 * 1024 * 1024, QChar('A')), 0, 0);
        QTRY_VERIFY(!printer.busy());
        QVERIFY(printer.status().contains(QStringLiteral("timed out")));
    }

    void disconnectDuringTransfer()
    {
        QTcpServer server;
        QVERIFY(server.listen(QHostAddress::LocalHost, 0));
        connect(&server, &QTcpServer::newConnection, this, [&] {
            auto *peer = server.nextPendingConnection();
            peer->abort();
            peer->deleteLater();
        });
        PrinterController printer(QStringLiteral("127.0.0.1"), server.serverPort(), 10000);
        printer.printText(QString(8 * 1024 * 1024, QChar('A')), 0, 0);
        QTRY_VERIFY_WITH_TIMEOUT(!printer.busy(), 10000);
        QVERIFY(printer.status().startsWith(QStringLiteral("Print failed:"))
                || printer.status().contains(QStringLiteral("disconnected")));
    }
};

QTEST_GUILESS_MAIN(PrinterTests)
#include "printer_tests.moc"
