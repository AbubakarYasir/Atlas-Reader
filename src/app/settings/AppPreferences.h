#pragma once

// Interface preferences shared by every view (language, theme, touch mode,
// reading colours). Stored locally in the profile as preferences.ini; command
// line overrides apply to the session only and are never written back.

#include <QObject>
#include <QString>
#include <QStringList>

namespace atlas::app {

class AppPreferences final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString language READ language WRITE setLanguage NOTIFY languageChanged)
    Q_PROPERTY(QString themeMode READ themeMode WRITE setThemeMode NOTIFY themeChanged)
    Q_PROPERTY(bool dark READ dark NOTIFY themeChanged)
    Q_PROPERTY(bool touchMode READ touchMode WRITE setTouchMode NOTIFY touchModeChanged)
    Q_PROPERTY(int readingColors READ readingColors WRITE setReadingColors NOTIFY readingColorsChanged)
    Q_PROPERTY(QString skin READ skin WRITE setSkin NOTIFY skinChanged)
    Q_PROPERTY(int themeColor READ themeColor WRITE setThemeColor NOTIFY skinChanged)
    Q_PROPERTY(bool ribbonCollapsed READ ribbonCollapsed WRITE setRibbonCollapsed NOTIFY ribbonCollapsedChanged)
    Q_PROPERTY(int statusBarMode READ statusBarMode WRITE setStatusBarMode NOTIFY statusBarModeChanged)
    Q_PROPERTY(QString navPanel READ navPanel WRITE setNavPanel NOTIFY navPanelChanged)
    Q_PROPERTY(int navPanelWidth READ navPanelWidth WRITE setNavPanelWidth NOTIFY navPanelChanged)
    Q_PROPERTY(QStringList recentFiles READ recentFiles NOTIFY recentFilesChanged)

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

    // Skins (Foxit model): "classic" neutral title bar or "colorful" in the theme colour.
    [[nodiscard]] QString skin() const { return skin_; }
    void setSkin(const QString& skin);
    [[nodiscard]] int themeColor() const { return themeColor_; }
    void setThemeColor(int index);
    [[nodiscard]] bool ribbonCollapsed() const { return ribbonCollapsed_; }
    void setRibbonCollapsed(bool collapsed);
    // 0 show, 1 auto-hide, 2 hide (View > View Setting > Status Bar).
    [[nodiscard]] int statusBarMode() const { return statusBarMode_; }
    void setStatusBarMode(int mode);
    // Navigation pane: open panel ("" = hidden, "pages", …) and its width.
    [[nodiscard]] QString navPanel() const { return navPanel_; }
    void setNavPanel(const QString& panel);
    [[nodiscard]] int navPanelWidth() const { return navPanelWidth_; }
    void setNavPanelWidth(int width);
    // Most recent first, at most kMaxRecentFiles local paths.
    [[nodiscard]] QStringList recentFiles() const { return recentFiles_; }
    Q_INVOKABLE void addRecentFile(const QString& path);
    Q_INVOKABLE void removeRecentFile(const QString& path);

    static constexpr int kThemeColorCount = 6;
    static constexpr int kMaxRecentFiles = 20;

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
    void skinChanged();
    void ribbonCollapsedChanged();
    void statusBarModeChanged();
    void recentFilesChanged();
    void navPanelChanged();

private:
    void save() const;
    void applyLayoutDirection() const;

    QString settingsFile_;
    QString language_{QStringLiteral("en")};
    QString themeMode_{QStringLiteral("system")};
    bool touchMode_{};
    int readingColors_{};
    QString skin_{QStringLiteral("colorful")};
    int themeColor_{};
    bool ribbonCollapsed_{};
    int statusBarMode_{};
    QStringList recentFiles_;
    QString navPanel_;
    int navPanelWidth_{240};
    bool persist_{true};
};

} // namespace atlas::app
