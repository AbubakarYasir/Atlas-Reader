#pragma once

// Interface preferences shared by every view (language, theme, touch mode,
// reading colours). Stored locally in the profile as preferences.ini; command
// line overrides apply to the session only and are never written back.

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>

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
    Q_PROPERTY(QString rightPanel READ rightPanel WRITE setRightPanel NOTIFY rightPanelChanged)
    Q_PROPERTY(int defaultPageDisplay READ defaultPageDisplay WRITE setDefaultPageDisplay NOTIFY readingDefaultsChanged)
    Q_PROPERTY(int defaultZoom READ defaultZoom WRITE setDefaultZoom NOTIFY readingDefaultsChanged)
    Q_PROPERTY(bool rememberPosition READ rememberPosition WRITE setRememberPosition NOTIFY readingDefaultsChanged)
    Q_PROPERTY(bool searchMatchCase READ searchMatchCase WRITE setSearchMatchCase NOTIFY searchOptionsChanged)
    Q_PROPERTY(bool searchWholeWords READ searchWholeWords WRITE setSearchWholeWords NOTIFY searchOptionsChanged)
    Q_PROPERTY(QStringList pinnedFiles READ pinnedFiles NOTIFY recentFilesChanged)
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
    // Right panel: "" hidden or "search".
    [[nodiscard]] QString rightPanel() const { return rightPanel_; }
    void setRightPanel(const QString& panel);
    // Page Display defaults (Preferences → Page Display): 0 single … 3 continuous facing;
    // zoom 0 fit width, 1 fit page, 2 actual size.
    [[nodiscard]] int defaultPageDisplay() const { return defaultPageDisplay_; }
    void setDefaultPageDisplay(int value);
    [[nodiscard]] int defaultZoom() const { return defaultZoom_; }
    void setDefaultZoom(int value);
    [[nodiscard]] bool rememberPosition() const { return rememberPosition_; }
    void setRememberPosition(bool value);
    [[nodiscard]] bool searchMatchCase() const { return searchMatchCase_; }
    void setSearchMatchCase(bool value);
    [[nodiscard]] bool searchWholeWords() const { return searchWholeWords_; }
    void setSearchWholeWords(bool value);
    [[nodiscard]] QStringList pinnedFiles() const { return pinnedFiles_; }
    Q_INVOKABLE void setPinned(const QString& path, bool pinned);
    // Last reading position per file (History): page, zoom, fit, display.
    Q_INVOKABLE QVariantMap positionFor(const QString& path) const;
    Q_INVOKABLE void savePosition(const QString& path, int page, double zoom, int fitMode, int display);
    // Most recent first, at most kMaxRecentFiles local paths.
    [[nodiscard]] QStringList recentFiles() const { return recentFiles_; }
    Q_INVOKABLE void addRecentFile(const QString& path);
    Q_INVOKABLE void removeRecentFile(const QString& path);
    // Preferences → History: forget unpinned recent files and saved reading positions.
    Q_INVOKABLE void clearHistory();

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
    void rightPanelChanged();
    void readingDefaultsChanged();
    void searchOptionsChanged();

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
    QString rightPanel_;
    int defaultPageDisplay_{1};
    int defaultZoom_{};
    bool rememberPosition_{true};
    bool searchMatchCase_{};
    bool searchWholeWords_{};
    QStringList pinnedFiles_;
    bool persist_{true};
};

} // namespace atlas::app
