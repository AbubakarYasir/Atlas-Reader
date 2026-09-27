#include "app/diagnostics/ShellMetrics.h"
#include "app/library/LibraryController.h"
#include "app/logging/Logging.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QGuiApplication>
#include <QDir>
#include <QImage>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickWindow>
#include <QStyleHints>
#include <QStandardPaths>
#include <QTimer>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <algorithm>

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

std::filesystem::path filesystemPath(const QString& value) {
#ifdef _WIN32
    return std::filesystem::path{value.toStdWString()};
#else
    return std::filesystem::path{value.toStdString()};
#endif
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
        QStringLiteral("Atlas Reader Native library and index beta"));
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
    const QCommandLineOption profileDirectoryOption(
        QStringLiteral("profile-directory"),
        QStringLiteral("Use an explicit Atlas profile directory."),
        QStringLiteral("path"));
    const QCommandLineOption screenshotFileOption(
        QStringLiteral("screenshot-file"),
        QStringLiteral("Save a first-frame PNG for visual QA."),
        QStringLiteral("path"));
    const QCommandLineOption screenshotDelayOption(
        QStringLiteral("screenshot-delay-ms"),
        QStringLiteral("Delay first-frame screenshot capture for asynchronous UI QA."),
        QStringLiteral("milliseconds"),
        QStringLiteral("100"));
    const QCommandLineOption addLibraryRootOption(
        QStringLiteral("add-library-root"),
        QStringLiteral("Add and scan one library root for QA."),
        QStringLiteral("path"));
    const QCommandLineOption windowWidthOption(
        QStringLiteral("window-width"),
        QStringLiteral("Set the initial window width for visual QA."),
        QStringLiteral("pixels"));
    const QCommandLineOption windowHeightOption(
        QStringLiteral("window-height"),
        QStringLiteral("Set the initial window height for visual QA."),
        QStringLiteral("pixels"));

    parser.addOption(languageOption);
    parser.addOption(themeOption);
    parser.addOption(quitAfterOption);
    parser.addOption(metricsFileOption);
    parser.addOption(benchmarkShellOption);
    parser.addOption(profileDirectoryOption);
    parser.addOption(screenshotFileOption);
    parser.addOption(screenshotDelayOption);
    parser.addOption(addLibraryRootOption);
    parser.addOption(windowWidthOption);
    parser.addOption(windowHeightOption);
    parser.process(app);
    const auto argumentsReady = MetricsClock::now();

    const QString language = normalizedLanguage(parser.value(languageOption));
    const QString theme = normalizedTheme(parser.value(themeOption));
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
        << QStringLiteral("Starting %1 with Qt %2; language=%3 theme=%4")
               .arg(QGuiApplication::applicationVersion(), QString::fromLatin1(qVersion()), language, theme);

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

    const QString profileDirectory = parser.isSet(profileDirectoryOption)
        ? QDir::cleanPath(parser.value(profileDirectoryOption))
        : QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    atlas::app::LibraryController libraryController(
        filesystemPath(QDir{profileDirectory}.filePath(QStringLiteral("library.sqlite3"))));
    for (const auto& rootPath : parser.values(addLibraryRootOption)) {
        libraryController.addRoot(QUrl::fromLocalFile(QDir::cleanPath(rootPath)));
    }

    engine.rootContext()->setContextProperty(
        QStringLiteral("atlasVersion"), QGuiApplication::applicationVersion());
    engine.rootContext()->setContextProperty(QStringLiteral("atlasInitialArabic"), isArabic);
    engine.rootContext()->setContextProperty(QStringLiteral("atlasInitialDark"), initialDark);
    engine.rootContext()->setContextProperty(QStringLiteral("libraryController"), &libraryController);

    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        [] { QCoreApplication::exit(EXIT_FAILURE); },
        Qt::QueuedConnection);

    metrics.recordStage(QStringLiteral("startup.before_qml_load_ms"));
    engine.loadFromModule(QStringLiteral("AtlasReader"), QStringLiteral("Main"));
    metrics.recordQmlLoaded();

    if (engine.rootObjects().isEmpty()) {
        qCCritical(atlas::logging::startup) << "QML root object was not created";
        return EXIT_FAILURE;
    }

    if (auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().constFirst())) {
        bool widthValid = false;
        bool heightValid = false;
        const int requestedWidth = parser.value(windowWidthOption).toInt(&widthValid);
        const int requestedHeight = parser.value(windowHeightOption).toInt(&heightValid);
        if (widthValid && heightValid && requestedWidth > 0 && requestedHeight > 0) {
            window->resize(requestedWidth, requestedHeight);
        }
        metrics.attach(window);
        const QString screenshotPath = parser.value(screenshotFileOption);
        if (!screenshotPath.isEmpty()) {
            bool screenshotDelayValid = false;
            const int requestedDelay = parser.value(screenshotDelayOption).toInt(&screenshotDelayValid);
            const int screenshotDelay = screenshotDelayValid ? std::max(0, requestedDelay) : 100;
            QObject::connect(window, &QQuickWindow::frameSwapped, window, [window, screenshotPath, screenshotDelay] {
                QTimer::singleShot(screenshotDelay, window, [window, screenshotPath] {
                    const bool saved = window->grabWindow().save(screenshotPath, "PNG");
                    if (!saved) qCWarning(atlas::logging::startup) << "Could not save UI screenshot";
                });
            }, Qt::SingleShotConnection);
        }
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
