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
}

void AppPreferences::applyLayoutDirection() const {
    const bool rtl = language_ == QStringLiteral("ar") || language_ == QStringLiteral("ur");
    QGuiApplication::setLayoutDirection(rtl ? Qt::RightToLeft : Qt::LeftToRight);
}

} // namespace atlas::app
