#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName("Atlas Reader Native");
    QGuiApplication::setOrganizationName("Atlas Reader");

    QQuickStyle::setStyle("Fusion");

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        [] { QCoreApplication::exit(EXIT_FAILURE); },
        Qt::QueuedConnection);

    engine.loadFromModule("AtlasReader", "Main");
    return app.exec();
}
