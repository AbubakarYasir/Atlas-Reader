#pragma once

#include "app/document/DocumentCapabilityCheck.h"
#include "core/reader/ReaderSessionModel.h"

#include <QAbstractListModel>
#include <QFutureWatcher>
#include <QHash>
#include <QUrl>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

namespace atlas::app {

class ReaderController final : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int activeIndex READ activeIndex WRITE setActiveIndex NOTIFY activeIndexChanged)
    Q_PROPERTY(bool hasSessions READ hasSessions NOTIFY countChanged)
    Q_PROPERTY(bool restoreEnabled READ restoreEnabled WRITE setRestoreEnabled NOTIFY restoreEnabledChanged)

public:
    enum Role {
        SessionIdRole = Qt::UserRole + 1,
        TitleRole,
        SourceRole,
        StatusRole,
        PageCountRole,
        DetailRole,
        DocumentTitleRole,
        CapabilityRole, // N5.1: what Atlas may change in this PDF, and why not
    };
    Q_ENUM(Role)

    explicit ReaderController(std::filesystem::path profileDirectory, QObject* parent = nullptr);
    ~ReaderController() override;

    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
    [[nodiscard]] int activeIndex() const;
    void setActiveIndex(int index);
    [[nodiscard]] bool hasSessions() const;
    [[nodiscard]] bool restoreEnabled() const;
    void setRestoreEnabled(bool enabled);

    Q_INVOKABLE void openLocalFile(const QUrl& file);
    Q_INVOKABLE void openLocalPath(const QString& path);
    Q_INVOKABLE void closeAt(int index);
    Q_INVOKABLE void retryAt(int index);
    Q_INVOKABLE void submitPassword(int index, const QString& password);
    Q_INVOKABLE void closeActive();
    Q_INVOKABLE void closeAll();
    // Foxit tab context menu.
    Q_INVOKABLE void closeOthers(int index);
    Q_INVOKABLE void closeLeft(int index);
    Q_INVOKABLE void closeRight(int index);
    Q_INVOKABLE void copyPathAt(int index) const;
    // Copies plain text the user chose to copy (a link address, a word count).
    Q_INVOKABLE void copyText(const QString& text) const;
    Q_INVOKABLE void showPathInFolder(const QString& path) const;
    // Opens a web or mail link the user confirmed; other schemes are refused.
    Q_INVOKABLE bool openExternalLink(const QString& url) const;
    Q_INVOKABLE void showInFolderAt(int index) const;
    Q_INVOKABLE void showLibrary();
    // N5.1: re-run the capability check (also detects a change made outside Atlas).
    Q_INVOKABLE void recheckCapabilities(int index);
    // N5.4: Atlas itself saved this file; its new revision is the baseline
    // (not a change made outside Atlas). Re-runs the capability check.
    Q_INVOKABLE void acceptSavedFile(const QString& path);
    // For safe save on the bookmark worker; never exposed to QML.
    [[nodiscard]] QString passwordForPath(const QString& path) const;

    // For the in-process page renderer only; never exposed to QML. The password
    // lives in memory for the tab's lifetime and is never persisted or logged.
    struct RenderSource {
        QString path;
        QString password;
        quint64 revision{};
    };
    [[nodiscard]] std::optional<RenderSource> renderSource(qulonglong sessionId) const;

signals:
    void activeIndexChanged();
    void countChanged();
    void restoreEnabledChanged();

private:
    struct PendingOpen {
        std::uint64_t sessionId{};
        std::uint64_t revision{};
        std::unique_ptr<QFutureWatcher<atlas::reader::OpenResult>> watcher;
    };

    void beginOpen(const QString& sourcePath);
    void launchOpen(
        std::uint64_t sessionId,
        std::uint64_t revision,
        const QString& sourcePath,
        const QString& password = {});
    void cancelPending(std::uint64_t sessionId);
    void scheduleRetiredCleanup(QFutureWatcher<atlas::reader::OpenResult>* watcher);
    void refreshModel(int previousActiveIndex, int previousCount);
    void restoreSessions();
    void startCapabilityCheck(std::uint64_t sessionId);
    struct CapabilityState {
        QVariantMap map;
        RevisionStamp baseline;
        quint64 generation{};
    };
    std::unordered_map<std::uint64_t, CapabilityState> capabilities_;
    void saveSessions() const;
    [[nodiscard]] QString settingsPath() const;
    [[nodiscard]] static QString normalizedSourceKey(const QString& sourcePath);
    [[nodiscard]] static std::filesystem::path toPath(const QString& sourcePath);

    atlas::reader::ReaderSessionModel model_;
    std::filesystem::path profileDirectory_;
    std::unordered_map<std::uint64_t, std::unique_ptr<PendingOpen>> pending_;
    std::vector<std::unique_ptr<PendingOpen>> retired_;
    std::unordered_map<std::uint64_t, QString> sessionPasswords_;
    bool restoreEnabled_{};
    bool libraryVisible_{true};
};

} // namespace atlas::app
