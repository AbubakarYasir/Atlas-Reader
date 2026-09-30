#include "app/reader/ReaderController.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QTemporaryDir>
#include <QThread>
#include <QUrl>

#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace {

void require(bool condition, std::string_view message) {
    if (!condition) throw std::runtime_error(std::string{message});
}

std::filesystem::path toPath(const QString& path) {
#ifdef _WIN32
    return std::filesystem::path{path.toStdWString()};
#else
    const auto bytes = path.toUtf8();
    return std::filesystem::path{std::string{bytes.constData(), static_cast<std::size_t>(bytes.size())}};
#endif
}

QString statusAt(const atlas::app::ReaderController& controller, int row) {
    return controller.data(controller.index(row), atlas::app::ReaderController::StatusRole).toString();
}

QString detailAt(const atlas::app::ReaderController& controller, int row) {
    return controller.data(controller.index(row), atlas::app::ReaderController::DetailRole).toString();
}

int pageCountAt(const atlas::app::ReaderController& controller, int row) {
    return controller.data(controller.index(row), atlas::app::ReaderController::PageCountRole).toInt();
}

bool waitForStatus(
    const atlas::app::ReaderController& controller,
    int row,
    const QString& expected,
    int timeoutMs = 5000) {
    QElapsedTimer timer;
    timer.start();
    while (timer.elapsed() < timeoutMs) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, 25);
        if (row >= 0 && row < controller.rowCount() && statusAt(controller, row) == expected) return true;
        QThread::msleep(5);
    }
    QCoreApplication::processEvents(QEventLoop::AllEvents, 25);
    return row >= 0 && row < controller.rowCount() && statusAt(controller, row) == expected;
}

QString copyFixture(const QString& source, const QTemporaryDir& temp, const QString& name) {
    const QString destination = temp.filePath(name);
    require(QFileInfo::exists(source), "Required reader fixture is missing.");
    require(QFile::copy(source, destination), "Could not copy reader fixture into the disposable test directory.");
    return destination;
}

QByteArray readSettings(const QString& profileDirectory) {
    QFile file{profileDirectory + QStringLiteral("/reader-session.ini")};
    require(file.open(QIODevice::ReadOnly), "Reader restoration settings were not written.");
    return file.readAll();
}

void requirePasswordsAbsent(const QByteArray& settings) {
    require(!settings.contains("atlas-user"), "The correct fixture password must never be persisted.");
    require(!settings.contains("atlas-wrong"), "A rejected fixture password must never be persisted.");
}

} // namespace

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    if (argc != 3) {
        std::cerr << "Usage: atlas_reader_controller_tests <simple-pdf> <password-pdf>\n";
        return 2;
    }

    try {
        QTemporaryDir temp;
        require(temp.isValid(), "Could not create the disposable reader test directory.");
        const QString profileDirectory = temp.filePath(QStringLiteral("profile"));
        const QString simplePdf = copyFixture(
            QString::fromLocal8Bit(argv[1]), temp, QStringLiteral("simple.pdf"));
        const QString passwordPdf = copyFixture(
            QString::fromLocal8Bit(argv[2]), temp, QStringLiteral("password.pdf"));

        {
            atlas::app::ReaderController controller{toPath(profileDirectory)};
            require(!controller.restoreEnabled(), "Reader restoration must default to disabled.");
            require(controller.rowCount() == 0, "A new reader profile must begin without sessions.");

            controller.setRestoreEnabled(true);
            controller.openLocalFile(QUrl::fromLocalFile(simplePdf));
            require(waitForStatus(controller, 0, QStringLiteral("ready")),
                "A normal PDF must reach the ready state asynchronously.");
            require(pageCountAt(controller, 0) == 1, "The simple fixture must report one page.");

            const QString movedSimplePdf = temp.filePath(QStringLiteral("simple-renamed.pdf"));
            require(QFile::rename(simplePdf, movedSimplePdf),
                "A ready N4.1 session must not retain a file handle that blocks rename/share on Windows.");
            require(QFile::rename(movedSimplePdf, simplePdf),
                "The disposable sharing fixture could not be restored to its original path.");

            controller.openLocalFile(QUrl::fromLocalFile(passwordPdf));
            require(waitForStatus(controller, 1, QStringLiteral("passwordRequired")),
                "An encrypted PDF must expose the password-required state.");
            require(detailAt(controller, 1) == QStringLiteral("password-required"),
                "The first encrypted open must be distinguishable from a rejected password.");

            controller.submitPassword(1, QStringLiteral("atlas-wrong"));
            require(waitForStatus(controller, 1, QStringLiteral("passwordRequired")),
                "A rejected password must return to password-required rather than a generic failure.");
            require(detailAt(controller, 1) == QStringLiteral("incorrect-password"),
                "A rejected password must provide a specific retry state without logging the secret.");

            controller.submitPassword(1, QStringLiteral("atlas-user"));
            require(waitForStatus(controller, 1, QStringLiteral("ready")),
                "The deterministic test password must unlock the encrypted fixture.");
            require(pageCountAt(controller, 1) == 1, "The unlocked fixture must report one page.");

            const QByteArray settings = readSettings(profileDirectory);
            requirePasswordsAbsent(settings);
            require(settings.contains("simple.pdf") && settings.contains("password.pdf"),
                "Opt-in restoration must persist paths for open reader tabs.");
        }

        {
            atlas::app::ReaderController restored{toPath(profileDirectory)};
            require(restored.restoreEnabled(), "The opt-in restoration choice must survive restart.");
            require(restored.rowCount() == 2, "Restart must recreate both path-only reader sessions.");
            require(waitForStatus(restored, 0, QStringLiteral("ready")),
                "The unencrypted restored session must reopen successfully.");
            require(waitForStatus(restored, 1, QStringLiteral("passwordRequired")),
                "A protected restored session must ask for its password again after restart.");
            require(detailAt(restored, 1) == QStringLiteral("password-required"),
                "Restart must not silently reuse a previously supplied password.");
            requirePasswordsAbsent(readSettings(profileDirectory));
        }

        for (int iteration = 0; iteration < 12; ++iteration) {
            const QString cancellationProfile = temp.filePath(QStringLiteral("cancel-%1").arg(iteration));
            atlas::app::ReaderController controller{toPath(cancellationProfile)};
            controller.openLocalFile(QUrl::fromLocalFile(simplePdf));
            require(controller.rowCount() == 1, "Cancellation stress must create one pending session.");
            controller.closeAt(0);
            require(controller.rowCount() == 0, "Closing a pending session must release it immediately.");
        }

        QCoreApplication::processEvents(QEventLoop::AllEvents, 50);
    } catch (const std::exception& error) {
        std::cerr << "Reader controller test failed: " << error.what() << '\n';
        return 1;
    }

    std::cout << "N4.1 password, restoration, sharing and cancellation lifecycle checks passed.\n";
    return 0;
}
