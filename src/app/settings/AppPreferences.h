#pragma once

// Interface preferences shared by every view (language, theme, touch mode,
// reading colours). Stored locally in the profile as preferences.ini; command
// line overrides apply to the session only and are never written back.

#include <QObject>
#include <QString>

namespace atlas::app {

class AppPreferences final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(QString themeMode READ themeMode WRITE setThemeMode NOTIFY themeChanged)
    Q_PROPERTY(bool dark READ dark NOTIFY themeChanged)
    Q_PROPERTY(bool touchMode READ touchMode WRITE setTouchMode NOTIFY touchModeChanged)
    Q_PROPERTY(int readingColors READ readingColors WRITE setReadingColors NOTIFY readingColorsChanged)

public:
    explicit AppPreferences(QString settingsFile, QObject* parent = nullptr);

    [[nodiscard]] QString language() const { return language_; }
    void setLanguage(const QString& language);
    [[nodiscard]] QString themeMode() const { return themeMode_; }
    void setThemeMode(const QString& mode);
    [[nodiscard]] bool dark() const;
    [[nodiscard]] bool touchMode() const { return touchMode_; }
    void setTouchMode(bool enabled);
    [[nodiscard]] int readingColors() const { return readingColors_; }
    void setReadingColors(int colors);

    // Session-only overrides from the command line (QA and support).
    void overrideLanguage(const QString& language);
    void overrideThemeMode(const QString& mode);

    [[nodiscard]] static QString normalizedLanguage(const QString& value);
    [[nodiscard]] static QString normalizedThemeMode(const QString& value);

signals:
    void languageChanged();
    void themeChanged();
    void touchModeChanged();
    void readingColorsChanged();

private:
    void save() const;
    void applyLayoutDirection() const;

    QString settingsFile_;
    QString language_{QStringLiteral("en")};
    QString themeMode_{QStringLiteral("system")};
    bool touchMode_{};
    int readingColors_{};
    bool persist_{true};
};

} // namespace atlas::app
