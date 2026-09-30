#include "printercontroller.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Impressora Custom"));
    QQuickStyle::setStyle(QStringLiteral("Fusion"));

    PrinterController printer;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("printerController"), &printer);
    engine.load(QUrl(QStringLiteral("qrc:/ImpressoraCustom/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return 1;
    return app.exec();
}
