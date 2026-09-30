#pragma once

#include "core/reader/ReaderSessionModel.h"

#include <QAbstractListModel>
#include <QFutureWatcher>
#include <QHash>
#include <QUrl>

#include <cstdint>
#include <filesystem>
#include <memory>
#include <unordered_map>

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
    Q_INVOKABLE void closeAt(int index);
    Q_INVOKABLE void retryAt(int index);
    Q_INVOKABLE void submitPassword(int index, const QString& password);
    Q_INVOKABLE void closeActive();
    Q_INVOKABLE void showLibrary();

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
    void refreshModel(int previousActiveIndex, int previousCount);
    void restoreSessions();
    void saveSessions() const;
    [[nodiscard]] QString settingsPath() const;
    [[nodiscard]] static QString normalizedSourceKey(const QString& sourcePath);
    [[nodiscard]] static std::filesystem::path toPath(const QString& sourcePath);

    atlas::reader::ReaderSessionModel model_;
    std::filesystem::path profileDirectory_;
    std::unordered_map<std::uint64_t, std::unique_ptr<PendingOpen>> pending_;
    bool restoreEnabled_{};
    bool libraryVisible_{true};
};

} // namespace atlas::app
