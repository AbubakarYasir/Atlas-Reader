#include "app/settings/AppPreferences.h"

#include <QGuiApplication>
#include <QSettings>
#include <QStyleHints>

#include <algorithm>

namespace atlas::app {

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

void AppPreferences::addRecentFile(const QString& path) {
    if (path.isEmpty()) return;
    QStringList next = recentFiles_;
    next.removeAll(path);
    next.prepend(path);
    if (next.size() > kMaxRecentFiles) next.resize(kMaxRecentFiles);
    if (next == recentFiles_) return;
    recentFiles_ = next;
    save();
    emit recentFilesChanged();
}

void AppPreferences::removeRecentFile(const QString& path) {
    if (recentFiles_.removeAll(path) == 0) return;
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
}

void AppPreferences::applyLayoutDirection() const {
    const bool rtl = language_ == QStringLiteral("ar") || language_ == QStringLiteral("ur");
    QGuiApplication::setLayoutDirection(rtl ? Qt::RightToLeft : Qt::LeftToRight);
}

} // namespace atlas::app
