#include "app/accessibility/TreeItemAccessible.h"
#include "app/bookmarks/BookmarkModel.h"
#include "app/bookmarks/BookmarkService.h"
#include "app/diagnostics/AccessibilityDump.h"
#include "app/diagnostics/ShellMetrics.h"
#include "app/fonts/InterfaceFonts.h"
#include "app/library/LibraryController.h"
#include "app/logging/Logging.h"
#include "app/reader/PageThumbnailProvider.h"
#include "app/reader/ReaderController.h"
#include "app/reader/ReaderViewport.h"
#include "app/settings/AppPreferences.h"

#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QGuiApplication>
#include <QFont>
#include <QFontDatabase>
#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QtQml/qqml.h>
#include <QQuickWindow>
#include <QStyleHints>
#include <QStandardPaths>
#include <QSvgRenderer>
#include <QTimer>

#include <algorithm>
#include <chrono>
#include <cstdlib>
#include <filesystem>

namespace {

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
    // Interface icons are SVG. Referencing Qt Svg keeps its image plugin in
    // the deployed package (windeployqt follows real module imports).
    static_cast<void>(QSvgRenderer::staticMetaObject.className());
    const auto applicationReady = MetricsClock::now();
    MetricsClock::time_point metrics_fontsRegistered{};

    // Bundled Arabic/Urdu fonts are registered once, before anything asks for
    // a font. Adding an application font later (QML FontLoader) invalidates
    // Qt's font database, which then enumerates every installed font again:
    // seconds on font-heavy Windows machines (N1-03).
    for (const QString& file : atlas::fonts::registerBundledFonts()) {
        qCWarning(atlas::logging::startup) << "Bundled font not registered:" << file;
    }
    // Weight 600 becomes an exact match for the system interface font; an
    // inexact match makes Qt read every installed font (see InterfaceFonts.h).
    const QString systemFontFamily = QGuiApplication::font().family();
    const bool semiboldAttached = atlas::fonts::attachSystemSemibold(systemFontFamily);
#ifdef Q_OS_WIN
    if (!semiboldAttached) {
        qCInfo(atlas::logging::startup) << "No semibold face for the system font" << systemFontFamily;
    }
#else
    static_cast<void>(semiboldAttached);
#endif

    // User content can need characters from other fonts; see InterfaceFonts.h.
    // ATLAS_FONT_WARMUP: unset = read the other families in idle slices after the
    // first frame; "0" = leave it to Qt; "hold" = never (CI font check).
    const QString fontWarmupSetting = qEnvironmentVariable("ATLAS_FONT_WARMUP");
    atlas::fonts::ContentFonts contentFonts(fontWarmupSetting == QLatin1String("0") ? atlas::fonts::ContentFonts::Mode::Off
                                            : fontWarmupSetting == QLatin1String("hold")
                                                ? atlas::fonts::ContentFonts::Mode::Hold
                                                : atlas::fonts::ContentFonts::Mode::Idle);
    atlas::accessibility::installTreeItemAccessibility(); // before any QML exists
    metrics_fontsRegistered = MetricsClock::now();

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
        QStringLiteral("Interface language for this session: en (default), ar or ur."),
        QStringLiteral("language"),
        QStringLiteral("en"));
    const QCommandLineOption themeOption(
        QStringLiteral("theme"),
        QStringLiteral("Theme for this session: system, light, or dark."),
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
    const QCommandLineOption showOption(
        QStringLiteral("show"),
        QStringLiteral("Open one surface at startup for visual QA: backstage, skins, preferences[:page], about, ribbon-<tab>, display:<0-3>[:rtl]."),
        QStringLiteral("surface"));
    const QCommandLineOption nativeFrameOption(
        QStringLiteral("native-frame"),
        QStringLiteral("Use the system window frame instead of Atlas's title bar."));
    const QCommandLineOption windowWidthOption(
        QStringLiteral("window-width"),
        QStringLiteral("Set the initial window width for visual QA."),
        QStringLiteral("pixels"));
    const QCommandLineOption windowHeightOption(
        QStringLiteral("window-height"),
        QStringLiteral("Set the initial window height for visual QA."),
        QStringLiteral("pixels"));

    parser.addPositionalArgument(
        QStringLiteral("files"), QStringLiteral("PDF files to open in the reader."), QStringLiteral("[files...]"));
    parser.addOption(languageOption);
    parser.addOption(nativeFrameOption);
    parser.addOption(showOption);
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

    bool quitDelayValid = false;
    const int quitAfterMs = parser.value(quitAfterOption).toInt(&quitDelayValid);
    const int effectiveQuitAfterMs = quitDelayValid && quitAfterMs > 0 ? quitAfterMs : 0;

    qCInfo(atlas::logging::startup).noquote()
        << QStringLiteral("Starting %1 with Qt %2")
               .arg(QGuiApplication::applicationVersion(), QString::fromLatin1(qVersion()));

    atlas::diagnostics::ShellMetrics metrics(
        processStart,
        parser.value(metricsFileOption),
        parser.isSet(benchmarkShellOption),
        &app);
    metrics.recordCheckpoint(QStringLiteral("startup.qgui_application_ready_ms"), applicationReady);
    metrics.recordCheckpoint(QStringLiteral("startup.fonts_registered_ms"), metrics_fontsRegistered);
    metrics.recordCheckpoint(QStringLiteral("startup.arguments_ready_ms"), argumentsReady);
    metrics.recordStage(QStringLiteral("startup.metrics_ready_ms"));

    qmlRegisterType<atlas::app::ReaderViewport>("Atlas.Reader", 1, 0, "ReaderViewport");
    qmlRegisterType<atlas::app::BookmarkModel>("Atlas.Reader", 1, 0, "BookmarkModel");
    qmlRegisterUncreatableType<atlas::app::BookmarkService>(
        "Atlas.Reader", 1, 0, "BookmarkService", QStringLiteral("Provided by the application"));
    qmlRegisterUncreatableType<atlas::app::ReaderController>(
        "Atlas.Reader", 1, 0, "ReaderController", QStringLiteral("Provided by the application"));

    const QString profileDirectory = parser.isSet(profileDirectoryOption)
        ? QDir::cleanPath(parser.value(profileDirectoryOption))
        : QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    atlas::app::LibraryController libraryController(
        filesystemPath(QDir{profileDirectory}.filePath(QStringLiteral("library.sqlite3"))));
    atlas::app::ReaderController readerController(filesystemPath(profileDirectory));
    // N5.3: bookmark storage on its own worker, in the same profile database.
    atlas::app::BookmarkService bookmarkService(
        filesystemPath(QDir{profileDirectory}.filePath(QStringLiteral("library.sqlite3"))));
    // Safe save asks the open reader session for a PDF's password (never QML).
    bookmarkService.setPasswordSource([&readerController](const QString& path) {
        return readerController.passwordForPath(path);
    });
    atlas::app::AppPreferences preferences(QDir{profileDirectory}.filePath(QStringLiteral("preferences.ini")));
    if (parser.isSet(languageOption)) preferences.overrideLanguage(parser.value(languageOption));
    if (parser.isSet(themeOption)) preferences.overrideThemeMode(parser.value(themeOption));

    // Declared after the objects it exposes so it is destroyed first: QML
    // bindings never see a destroyed context object during shutdown.
    // Before the engine, so it outlives every QML binding that reads it.
    atlas::accessibility::AccessibilityNotifier accessibilityNotifier;
    QQmlApplicationEngine engine;
    // An empty QML font family selects a legacy Windows bitmap font under GDI.
    // Preserve the actual platform interface font instead of clearing it.
    engine.rootContext()->setContextProperty(QStringLiteral("atlasSystemFontFamily"),
                                            systemFontFamily);
    engine.rootContext()->setContextProperty(QStringLiteral("atlasArabicFontFamily"),
                                            QString::fromLatin1(atlas::fonts::arabicFamily));
    engine.rootContext()->setContextProperty(QStringLiteral("atlasContentFonts"), &contentFonts);
    engine.rootContext()->setContextProperty(QStringLiteral("atlasUrduFontFamily"),
                                            QString::fromLatin1(atlas::fonts::urduFamily));
    metrics.recordStage(QStringLiteral("startup.qml_engine_ready_ms"));
    QObject::connect(
        &libraryController,
        &atlas::app::LibraryController::openDocumentRequested,
        &readerController,
        &atlas::app::ReaderController::openLocalFile);
    for (const auto& rootPath : parser.values(addLibraryRootOption)) {
        libraryController.addRoot(QUrl::fromLocalFile(QDir::cleanPath(rootPath)));
    }

    engine.rootContext()->setContextProperty(
        QStringLiteral("atlasVersion"), QGuiApplication::applicationVersion());
    engine.rootContext()->setContextProperty(QStringLiteral("preferences"), &preferences);
    engine.rootContext()->setContextProperty(QStringLiteral("nativeFrame"), parser.isSet(nativeFrameOption));
    engine.rootContext()->setContextProperty(QStringLiteral("qaShow"), parser.value(showOption));
    atlas::diagnostics::AccessibilityDump accessibilityDump;
    engine.rootContext()->setContextProperty(QStringLiteral("qaAccessibility"), &accessibilityDump);
    engine.rootContext()->setContextProperty(QStringLiteral("atlasAccessibility"), &accessibilityNotifier);
    engine.rootContext()->setContextProperty(QStringLiteral("libraryController"), &libraryController);
    engine.rootContext()->setContextProperty(QStringLiteral("readerController"), &readerController);
    engine.rootContext()->setContextProperty(QStringLiteral("bookmarkService"), &bookmarkService);
    // Pages panel thumbnails (engine owns the provider).
    auto* thumbnails = new atlas::app::PageThumbnailProvider;
    QObject thumbnailConnections; // destroyed before the engine deletes the provider
    engine.addImageProvider(QStringLiteral("atlasthumb"), thumbnails);
    const auto refreshThumbnails = [thumbnails, &readerController] { thumbnails->refresh(readerController); };
    QObject::connect(&readerController, &QAbstractItemModel::modelReset, &thumbnailConnections, refreshThumbnails);
    QObject::connect(&readerController, &QAbstractItemModel::dataChanged, &thumbnailConnections, refreshThumbnails);
    QObject::connect(&readerController, &QAbstractItemModel::rowsInserted, &thumbnailConnections, refreshThumbnails);
    QObject::connect(&readerController, &QAbstractItemModel::rowsRemoved, &thumbnailConnections, refreshThumbnails);

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

    // Files passed on the command line (Open with…, drag onto the icon).
    for (const auto& file : parser.positionalArguments()) {
        readerController.openLocalFile(QUrl::fromLocalFile(QFileInfo{file}.absoluteFilePath()));
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

    // Font diagnostics: when the idle read finished and (ATLAS_FONT_PROBE=1)
    // what characters from other scripts cost afterwards.
    const auto reportFonts = [&contentFonts, &metrics, systemFontFamily] {
        if (contentFonts.familiesRead() > 0) {
            metrics.recordStage(QStringLiteral("fonts.warmup_finished_ms"));
            metrics.recordValue(QStringLiteral("fonts.warmup_families"), contentFonts.familiesRead(), QStringLiteral("count"));
        }
        if (qEnvironmentVariable("ATLAS_FONT_PROBE") == QLatin1String("1")) {
            metrics.recordValue(QStringLiteral("fonts.fallback_probe_ms"),
                                atlas::fonts::measureFallbackLookupMs(systemFontFamily), QStringLiteral("ms"));
        }
    };
    QObject::connect(&contentFonts, &atlas::fonts::ContentFonts::readyChanged, &app, reportFonts);
    if (auto* window = qobject_cast<QQuickWindow*>(engine.rootObjects().constFirst())) {
        QObject::connect(window, &QQuickWindow::frameSwapped, &app, [&contentFonts, reportFonts] {
            if (contentFonts.ready()) {
                reportFonts(); // nothing to read (off, or not Windows)
            } else {
                contentFonts.start();
            }
        }, Qt::SingleShotConnection);
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
