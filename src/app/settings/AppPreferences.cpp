#include "app/settings/AppPreferences.h"

#include <QGuiApplication>
#include <QCryptographicHash>
#include <QSettings>
#include <QStyleHints>

#include <algorithm>

namespace atlas::app {
namespace {
// Navigation pane panels (Foxit left rail); anything else means "closed".
QString knownNavPanel(const QString& panel) {
    return panel == QStringLiteral("pages") || panel == QStringLiteral("bookmarks") || panel == QStringLiteral("signatures")
               ? panel
               : QString{};
}
} // namespace

AppPreferences::AppPreferences(QString settingsFile, QObject* parent)
    : QObject{parent}, settingsFile_{std::move(settingsFile)} {
    const QSettings settings{settingsFile_, QSettings::IniFormat};
    language_ = normalizedLanguage(settings.value(QStringLiteral("language"), language_).toString());
    themeMode_ = normalizedThemeMode(settings.value(QStringLiteral("theme"), themeMode_).toString());
    touchMode_ = settings.value(QStringLiteral("touchMode"), false).toBool();
    readingColors_ = std::clamp(settings.value(QStringLiteral("readingColors"), 0).toInt(), 0, 3);
    skin_ = settings.value(QStringLiteral("skin"), skin_).toString() == QStringLiteral("classic")
        ? QStringLiteral("classic") : QStringLiteral("colorful");
    themeColor_ = std::clamp(settings.value(QStringLiteral("themeColor"), 0).toInt(), 0, kThemeColorCount - 1);
    ribbonCollapsed_ = settings.value(QStringLiteral("ribbonCollapsed"), false).toBool();
    statusBarMode_ = std::clamp(settings.value(QStringLiteral("statusBarMode"), 0).toInt(), 0, 2);
    recentFiles_ = settings.value(QStringLiteral("recentFiles")).toStringList();
    {
        navPanel_ = knownNavPanel(settings.value(QStringLiteral("navPanel")).toString());
    }
    rightPanel_ = settings.value(QStringLiteral("rightPanel")).toString() == QStringLiteral("search") ? QStringLiteral("search") : QString{};
    defaultPageDisplay_ = std::clamp(settings.value(QStringLiteral("defaultPageDisplay"), 1).toInt(), 0, 3);
    defaultZoom_ = std::clamp(settings.value(QStringLiteral("defaultZoom"), 0).toInt(), 0, 2);
    rememberPosition_ = settings.value(QStringLiteral("rememberPosition"), true).toBool();
    searchMatchCase_ = settings.value(QStringLiteral("searchMatchCase"), false).toBool();
    searchWholeWords_ = settings.value(QStringLiteral("searchWholeWords"), false).toBool();
    identityName_ = settings.value(QStringLiteral("identityName")).toString();
    identityOrganization_ = settings.value(QStringLiteral("identityOrganization")).toString();
    identityEmail_ = settings.value(QStringLiteral("identityEmail")).toString();
    recheckOnReturn_ = settings.value(QStringLiteral("recheckOnReturn"), true).toBool();
    showDocumentStatus_ = settings.value(QStringLiteral("showDocumentStatus"), true).toBool();
    pinnedFiles_ = settings.value(QStringLiteral("pinnedFiles")).toStringList();
    navPanelWidth_ = std::clamp(settings.value(QStringLiteral("navPanelWidth"), 240).toInt(), 160, 480);
    if (recentFiles_.size() > kMaxRecentFiles) recentFiles_.resize(kMaxRecentFiles);
    applyLayoutDirection();
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    connect(QGuiApplication::styleHints(), &QStyleHints::colorSchemeChanged, this, [this] {
        if (themeMode_ == QStringLiteral("system")) emit themeChanged();
    });
#endif
}

QString AppPreferences::normalizedLanguage(const QString& value) {
    const QString code = value.trimmed().toLower();
    if (code == QStringLiteral("ar") || code == QStringLiteral("ur")) return code;
    return QStringLiteral("en");
}

QString AppPreferences::normalizedThemeMode(const QString& value) {
    const QString mode = value.trimmed().toLower();
    if (mode == QStringLiteral("light") || mode == QStringLiteral("dark")) return mode;
    return QStringLiteral("system");
}

void AppPreferences::setLanguage(const QString& language) {
    const QString code = normalizedLanguage(language);
    if (code == language_) return;
    language_ = code;
    applyLayoutDirection();
    save();
    emit languageChanged();
}

void AppPreferences::setThemeMode(const QString& mode) {
    const QString normalized = normalizedThemeMode(mode);
    if (normalized == themeMode_) return;
    themeMode_ = normalized;
    save();
    emit themeChanged();
}

bool AppPreferences::dark() const {
    if (themeMode_ == QStringLiteral("dark")) return true;
    if (themeMode_ == QStringLiteral("light")) return false;
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
    return QGuiApplication::styleHints()->colorScheme() == Qt::ColorScheme::Dark;
#else
    return false;
#endif
}

void AppPreferences::setTouchMode(bool enabled) {
    if (enabled == touchMode_) return;
    touchMode_ = enabled;
    save();
    emit touchModeChanged();
}

void AppPreferences::setReadingColors(int colors) {
    colors = std::clamp(colors, 0, 3);
    if (colors == readingColors_) return;
    readingColors_ = colors;
    save();
    emit readingColorsChanged();
}

void AppPreferences::setSkin(const QString& skin) {
    const QString normalized = skin == QStringLiteral("classic") ? skin : QStringLiteral("colorful");
    if (normalized == skin_) return;
    skin_ = normalized;
    save();
    emit skinChanged();
}

void AppPreferences::setThemeColor(int index) {
    index = std::clamp(index, 0, kThemeColorCount - 1);
    if (index == themeColor_) return;
    themeColor_ = index;
    save();
    emit skinChanged();
}

void AppPreferences::setRibbonCollapsed(bool collapsed) {
    if (collapsed == ribbonCollapsed_) return;
    ribbonCollapsed_ = collapsed;
    save();
    emit ribbonCollapsedChanged();
}

void AppPreferences::setStatusBarMode(int mode) {
    mode = std::clamp(mode, 0, 2);
    if (mode == statusBarMode_) return;
    statusBarMode_ = mode;
    save();
    emit statusBarModeChanged();
}

void AppPreferences::setNavPanel(const QString& panel) {
    const QString normalized = knownNavPanel(panel);
    if (normalized == navPanel_) return;
    navPanel_ = normalized;
    save();
    emit navPanelChanged();
}

void AppPreferences::setNavPanelWidth(int width) {
    width = std::clamp(width, 160, 480);
    if (width == navPanelWidth_) return;
    navPanelWidth_ = width;
    save();
    emit navPanelChanged();
}

void AppPreferences::setRightPanel(const QString& panel) {
    const QString normalized = panel == QStringLiteral("search") ? panel : QString{};
    if (normalized == rightPanel_) return;
    rightPanel_ = normalized;
    save();
    emit rightPanelChanged();
}

void AppPreferences::setDefaultPageDisplay(int value) {
    value = std::clamp(value, 0, 3);
    if (value == defaultPageDisplay_) return;
    defaultPageDisplay_ = value;
    save();
    emit readingDefaultsChanged();
}

void AppPreferences::setDefaultZoom(int value) {
    value = std::clamp(value, 0, 2);
    if (value == defaultZoom_) return;
    defaultZoom_ = value;
    save();
    emit readingDefaultsChanged();
}

void AppPreferences::setRememberPosition(bool value) {
    if (value == rememberPosition_) return;
    rememberPosition_ = value;
    save();
    emit readingDefaultsChanged();
}

void AppPreferences::setSearchMatchCase(bool value) {
    if (value == searchMatchCase_) return;
    searchMatchCase_ = value;
    save();
    emit searchOptionsChanged();
}

void AppPreferences::setSearchWholeWords(bool value) {
    if (value == searchWholeWords_) return;
    searchWholeWords_ = value;
    save();
    emit searchOptionsChanged();
}

// Identity text is kept exactly as typed (any script; nothing trimmed), but
// capped so a pasted document cannot bloat the settings file.
void AppPreferences::setIdentityName(const QString& value) {
    const QString capped = value.left(256);
    if (capped == identityName_) return;
    identityName_ = capped;
    save();
    emit identityChanged();
}

void AppPreferences::setIdentityOrganization(const QString& value) {
    const QString capped = value.left(256);
    if (capped == identityOrganization_) return;
    identityOrganization_ = capped;
    save();
    emit identityChanged();
}

void AppPreferences::setIdentityEmail(const QString& value) {
    const QString capped = value.left(256);
    if (capped == identityEmail_) return;
    identityEmail_ = capped;
    save();
    emit identityChanged();
}

void AppPreferences::setRecheckOnReturn(bool value) {
    if (value == recheckOnReturn_) return;
    recheckOnReturn_ = value;
    save();
    emit securityChanged();
}

void AppPreferences::setShowDocumentStatus(bool value) {
    if (value == showDocumentStatus_) return;
    showDocumentStatus_ = value;
    save();
    emit securityChanged();
}

void AppPreferences::setPinned(const QString& path, bool pinned) {
    if (pinned == pinnedFiles_.contains(path)) return;
    if (pinned) pinnedFiles_.prepend(path);
    else pinnedFiles_.removeAll(path);
    save();
    emit recentFilesChanged();
}

QVariantMap AppPreferences::positionFor(const QString& path) const {
    if (!rememberPosition_ || path.isEmpty()) return {};
    const QSettings settings{settingsFile_, QSettings::IniFormat};
    const QString key = QStringLiteral("positions/") +
                        QString::fromLatin1(QCryptographicHash::hash(path.toUtf8(), QCryptographicHash::Sha1).toHex());
    const QVariantList values = settings.value(key).toList();
    if (values.size() != 4) return {};
    return {{QStringLiteral("page"), values.at(0)}, {QStringLiteral("zoom"), values.at(1)},
            {QStringLiteral("fit"), values.at(2)}, {QStringLiteral("display"), values.at(3)}};
}

void AppPreferences::savePosition(const QString& path, int page, double zoom, int fitMode, int display) {
    if (!rememberPosition_ || !persist_ || path.isEmpty()) return;
    QSettings settings{settingsFile_, QSettings::IniFormat};
    const QString key = QStringLiteral("positions/") +
                        QString::fromLatin1(QCryptographicHash::hash(path.toUtf8(), QCryptographicHash::Sha1).toHex());
    settings.setValue(key, QVariantList{page, zoom, fitMode, display});
}

void AppPreferences::addRecentFile(const QString& path) {
    if (path.isEmpty()) return;
    QStringList next = recentFiles_;
    next.removeAll(path);
    next.prepend(path);
    // Trim the oldest unpinned entries; pinned files stay in the list.
    for (qsizetype i = next.size() - 1; i >= 0 && next.size() > kMaxRecentFiles; --i) {
        if (!pinnedFiles_.contains(next.at(i))) next.removeAt(i);
    }
    if (next == recentFiles_) return;
    recentFiles_ = next;
    save();
    emit recentFilesChanged();
}

void AppPreferences::removeRecentFile(const QString& path) {
    pinnedFiles_.removeAll(path);
    if (recentFiles_.removeAll(path) == 0) return;
    save();
    emit recentFilesChanged();
}

void AppPreferences::clearHistory() {
    QStringList kept;
    for (const QString& path : std::as_const(recentFiles_)) {
        if (pinnedFiles_.contains(path)) kept.append(path);
    }
    recentFiles_ = kept;
    if (persist_) {
        QSettings settings{settingsFile_, QSettings::IniFormat};
        settings.remove(QStringLiteral("positions"));
    }
    save();
    emit recentFilesChanged();
}

void AppPreferences::overrideLanguage(const QString& language) {
    language_ = normalizedLanguage(language);
    applyLayoutDirection();
    persist_ = false; // QA/session override: never rewrite the user's choice
    emit languageChanged();
}

void AppPreferences::overrideThemeMode(const QString& mode) {
    themeMode_ = normalizedThemeMode(mode);
    persist_ = false;
    emit themeChanged();
}

void AppPreferences::save() const {
    if (!persist_) return;
    QSettings settings{settingsFile_, QSettings::IniFormat};
    settings.setValue(QStringLiteral("language"), language_);
    settings.setValue(QStringLiteral("theme"), themeMode_);
    settings.setValue(QStringLiteral("touchMode"), touchMode_);
    settings.setValue(QStringLiteral("readingColors"), readingColors_);
    settings.setValue(QStringLiteral("skin"), skin_);
    settings.setValue(QStringLiteral("themeColor"), themeColor_);
    settings.setValue(QStringLiteral("ribbonCollapsed"), ribbonCollapsed_);
    settings.setValue(QStringLiteral("statusBarMode"), statusBarMode_);
    settings.setValue(QStringLiteral("recentFiles"), recentFiles_);
    settings.setValue(QStringLiteral("navPanel"), navPanel_);
    settings.setValue(QStringLiteral("navPanelWidth"), navPanelWidth_);
    settings.setValue(QStringLiteral("rightPanel"), rightPanel_);
    settings.setValue(QStringLiteral("defaultPageDisplay"), defaultPageDisplay_);
    settings.setValue(QStringLiteral("defaultZoom"), defaultZoom_);
    settings.setValue(QStringLiteral("rememberPosition"), rememberPosition_);
    settings.setValue(QStringLiteral("searchMatchCase"), searchMatchCase_);
    settings.setValue(QStringLiteral("searchWholeWords"), searchWholeWords_);
    settings.setValue(QStringLiteral("identityName"), identityName_);
    settings.setValue(QStringLiteral("identityOrganization"), identityOrganization_);
    settings.setValue(QStringLiteral("identityEmail"), identityEmail_);
    settings.setValue(QStringLiteral("recheckOnReturn"), recheckOnReturn_);
    settings.setValue(QStringLiteral("showDocumentStatus"), showDocumentStatus_);
    settings.setValue(QStringLiteral("pinnedFiles"), pinnedFiles_);
}

void AppPreferences::applyLayoutDirection() const {
    const bool rtl = language_ == QStringLiteral("ar") || language_ == QStringLiteral("ur");
    QGuiApplication::setLayoutDirection(rtl ? Qt::RightToLeft : Qt::LeftToRight);
}

} // namespace atlas::app
