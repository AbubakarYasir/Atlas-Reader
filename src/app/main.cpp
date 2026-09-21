#include "app/diagnostics/ShellMetrics.h"
#include "app/logging/Logging.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QStyleHints>
#include <QTimer>

#include <chrono>
#include <cstdlib>

namespace {

QString normalizedLanguage(const QString& value) {
    const QString normalized = value.trimmed().toLower();
    return normalized == QStringLiteral("ar") ? QStringLiteral("ar") : QStringLiteral("en");
}

QString normalizedTheme(const QString& value) {
    const QString normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("light") || normalized == QStringLiteral("dark")) {
        return normalized;
    }
    return QStringLiteral("system");
}

QString normalizedShellProfile(const QString& value) {
    const QString normalized = value.trimmed().toLower();
    if (normalized == QStringLiteral("bare")
        || normalized == QStringLiteral("text")
        || normalized == QStringLiteral("controls")) {
        return normalized;
    }
    return QStringLiteral("full");
}

QString qmlTypeForProfile(const QString& profile) {
    if (profile == QStringLiteral("bare")) {
        return QStringLiteral("BareProbe");
    }
    if (profile == QStringLiteral("text")) {
        return QStringLiteral("TextProbe");
    }
    if (profile == QStringLiteral("controls")) {
        return QStringLiteral("ControlsProbe");
    }
    return QStringLiteral("Main");
}

} // namespace

int main(int argc, char* argv[]) {
    using MetricsClock = atlas::diagnostics::ShellMetrics::Clock;
    const auto processStart = MetricsClock::now();

    QGuiApplication app(argc, argv);
    const auto applicationReady = MetricsClock::now();

    QGuiApplication::setApplicationName(QStringLiteral("Atlas Reader Native"));
    QGuiApplication::setOrganizationName(QStringLiteral("Atlas Reader"));
    QGuiApplication::setApplicationVersion(QStringLiteral(ATLAS_VERSION_STRING));

    atlas::logging::configureLogging();

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QStringLiteral("Atlas Reader Native N1 empty-shell baseline"));
    parser.addHelpOption();
    parser.addVersionOption();

    const QCommandLineOption languageOption(
        {QStringLiteral("language"), QStringLiteral("lang")},
        QStringLiteral("Initial shell language/direction: en or ar."),
        QStringLiteral("language"),
        QStringLiteral("en"));
    const QCommandLineOption themeOption(
        QStringLiteral("theme"),
        QStringLiteral("Initial shell theme: system, light, or dark."),
        QStringLiteral("theme"),
        QStringLiteral("system"));
    const QCommandLineOption shellProfileOption(
        QStringLiteral("shell-profile"),
        QStringLiteral("N1 diagnostic shell profile: full, bare, text, or controls."),
        QStringLiteral("profile"),
        QStringLiteral("full"));
    const QCommandLineOption quitAfterOption(
        QStringLiteral("quit-after-ms"),
        QStringLiteral("Quit automatically after N milliseconds; intended for lifecycle tests."),
        QStringLiteral("milliseconds"),
        QStringLiteral("0"));
    const QCommandLineOption metricsFileOption(
        QStringLiteral("metrics-file"),
        QStringLiteral("Write privacy-safe numeric shell metrics as JSON Lines to this path."),
        QStringLiteral("path"));
    const QCommandLineOption benchmarkShellOption(
        QStringLiteral("benchmark-shell"),
        QStringLiteral("Run the deterministic empty-shell resize exercise after first frame."));

    parser.addOption(languageOption);
    parser.addOption(themeOption);
    parser.addOption(shellProfileOption);
    parser.addOption(quitAfterOption);
    parser.addOption(metricsFileOption);
    parser.addOption(benchmarkShellOption);
    parser.process(app);
    const auto argumentsReady = MetricsClock::now();

    const QString language = normalizedLanguage(parser.value(languageOption));
    const QString theme = normalizedTheme(parser.value(themeOption));
    const QString shellProfile = normalizedShellProfile(parser.value(shellProfileOption));
    const bool isArabic = language == QStringLiteral("ar");

    QGuiApplication::setLayoutDirection(isArabic ? Qt::RightToLeft : Qt::LeftToRight);

    bool initialDark = theme == QStringLiteral("dark");
    if (theme == QStringLiteral("system")) {
        initialDark = QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
    }

    bool quitDelayValid = false;
    const int quitAfterMs = parser.value(quitAfterOption).toInt(&quitDelayValid);
    const int effectiveQuitAfterMs = quitDelayValid && quitAfterMs > 0 ? quitAfterMs : 0;

    qCInfo(atlas::logging::startup).noquote()
        << QStringLiteral("Starting %1 with Qt %2; language=%3 theme=%4 profile=%5")
               .arg(QGuiApplication::applicationVersion(),
                    QString::fromLatin1(qVersion()),
                    language,
                    theme,
                    shellProfile);

    atlas::diagnostics::ShellMetrics metrics(
        processStart,
        parser.value(metricsFileOption),
        parser.isSet(benchmarkShellOption),
        &app);
    metrics.recordCheckpoint(QStringLiteral("startup.qgui_application_ready_ms"), applicationReady);
    metrics.recordCheckpoint(QStringLiteral("startup.arguments_ready_ms"), argumentsReady);
    metrics.recordStage(QStringLiteral("startup.metrics_ready_ms"));

    QQmlApplicationEngine engine;
    metrics.recordStage(QStringLiteral("startup.qml_engine_ready_ms"));

    engine.rootContext()->setContextProperty(
        QStringLiteral("atlasVersion"), QGuiApplication::applicationVersion());
    engine.rootContext()->setContextProperty(QStringLiteral("atlasInitialArabic"), isArabic);
    engine.rootContext()->setContextProperty(QStringLiteral("atlasInitialDark"), initialDark);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        [] { QCoreApplication::exit(EXIT_FAILURE); },
        Qt::QueuedConnection);

    metrics.recordStage(QStringLiteral("startup.before_qml_load_ms"));
    engine.loadFromModule(QStringLiteral("AtlasReader"), qmlTypeForProfile(shellProfile));
    metrics.recordQmlLoaded();

    if (engine.rootObjects().isEmpty()) {
        qCCritical(atlas::logging::startup) << "QML root object was not created";
        return EXIT_FAILURE;
    }

    if (auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().constFirst())) {
        metrics.attach(window);
    } else {
        qCCritical(atlas::logging::startup) << "QML root object is not a QQuickWindow";
        return EXIT_FAILURE;
    }

    QObject::connect(&app, &QCoreApplication::aboutToQuit, &app, [&metrics] {
        metrics.recordShutdown();
        qCInfo(atlas::logging::startup) << "Atlas Reader Native shutdown cleanly";
    });

    if (effectiveQuitAfterMs > 0) {
        QTimer::singleShot(effectiveQuitAfterMs, &app, &QCoreApplication::quit);
    }

    return app.exec();
}
