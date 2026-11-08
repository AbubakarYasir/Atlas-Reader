// Preferences that the reader relies on (N4.4): per-book position memory,
// pinned recent files and Clear History. Uses a temporary settings file.
#include "app/settings/AppPreferences.h"

#include <QGuiApplication>
#include <QTemporaryDir>

#include <iostream>
#include <stdexcept>
#include <string>

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

} // namespace

int main(int argc, char** argv) {
    QGuiApplication app{argc, argv};
    try {
        QTemporaryDir dir;
        require(dir.isValid(), "temporary directory");
        const QString file = dir.filePath(QStringLiteral("settings.ini"));
        const QString book = QStringLiteral("C:/Books/كتاب.pdf");
        {
            atlas::app::AppPreferences prefs{file};
            require(prefs.positionFor(book).isEmpty(), "A new book has no saved position.");
            prefs.savePosition(book, 42, 1.5, 0, 2);
            const QVariantMap saved = prefs.positionFor(book);
            require(saved.value(QStringLiteral("page")).toInt() == 42 &&
                        qFuzzyCompare(saved.value(QStringLiteral("zoom")).toDouble(), 1.5) &&
                        saved.value(QStringLiteral("display")).toInt() == 2,
                    "A saved position must come back.");

            for (int i = 0; i < 30; ++i) prefs.addRecentFile(QStringLiteral("C:/Books/%1.pdf").arg(i));
            prefs.setPinned(QStringLiteral("C:/Books/25.pdf"), true);
            for (int i = 30; i < 60; ++i) prefs.addRecentFile(QStringLiteral("C:/Books/%1.pdf").arg(i));
            require(prefs.recentFiles().contains(QStringLiteral("C:/Books/25.pdf")),
                    "A pinned file must survive the Recent list limit.");
        }
        {
            atlas::app::AppPreferences prefs{file};
            require(prefs.positionFor(book).value(QStringLiteral("page")).toInt() == 42,
                    "Positions must persist across restarts.");
            require(prefs.pinnedFiles() == QStringList{QStringLiteral("C:/Books/25.pdf")}, "Pins must persist.");
            prefs.clearHistory();
            require(prefs.recentFiles() == QStringList{QStringLiteral("C:/Books/25.pdf")},
                    "Clear History keeps pinned files only.");
            require(prefs.positionFor(book).isEmpty(), "Clear History forgets reading positions.");
            // N5.3 Identity and Security: exact text in any script, defaults on.
            require(prefs.recheckOnReturn() && prefs.showDocumentStatus(), "Security options default on.");
            prefs.setIdentityName(QStringLiteral("  أبو بكر ياسر  "));
            prefs.setIdentityOrganization(QStringLiteral("ڈیرک سول"));
            prefs.setIdentityEmail(QStringLiteral("reader@example.org"));
            prefs.setRecheckOnReturn(false);
            prefs.setNavPanel(QStringLiteral("nonsense"));
            require(prefs.navPanel().isEmpty(), "Unknown navigation panels are closed.");
            prefs.setNavPanel(QStringLiteral("signatures"));
            prefs.setIdentityName(QString(1000, QLatin1Char('x')));
            require(prefs.identityName().size() == 256, "Identity text is capped.");
            prefs.setIdentityName(QStringLiteral("  أبو بكر ياسر  "));
            prefs.setRememberPosition(false);
            prefs.savePosition(book, 7, 1.0, 1, 1);
            require(prefs.positionFor(book).isEmpty(), "With remembering off, nothing is saved.");
        }
        {
            atlas::app::AppPreferences prefs{file};
            require(prefs.identityName() == QStringLiteral("  أبو بكر ياسر  ") &&
                        prefs.identityOrganization() == QStringLiteral("ڈیرک سول") &&
                        prefs.identityEmail() == QStringLiteral("reader@example.org"),
                    "Identity persists exactly as typed.");
            require(!prefs.recheckOnReturn() && prefs.showDocumentStatus(), "Security options persist.");
            require(prefs.navPanel() == QStringLiteral("signatures"), "The Signatures panel stays open across restarts.");
        }
    } catch (const std::exception& error) {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
    std::cout << "app preferences tests passed\n";
    return 0;
}
