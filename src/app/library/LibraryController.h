#pragma once

#include "app/index/LibraryIngestionService.h"
#include "app/index/NativeFileIdentityProvider.h"
#include "app/index/NativeLibraryEnumerator.h"
#include "app/index/QtPdfDocumentInspector.h"
#include "app/index/SqliteLibraryIndex.h"

#include <QAbstractListModel>
#include <QFutureWatcher>
#include <QThread>
#include <QUrl>
#include <QVariantList>

#include <atomic>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <set>
#include <vector>

namespace atlas::app {

class LibraryController final : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(QString query READ query WRITE setQuery NOTIFY queryChanged)
    Q_PROPERTY(int viewMode READ viewMode WRITE setViewMode NOTIFY viewModeChanged)
    Q_PROPERTY(bool scanning READ scanning NOTIFY scanningChanged)
    Q_PROPERTY(QString statusKey READ statusKey NOTIFY statusChanged)
    Q_PROPERTY(int lastAddedCount READ lastAddedCount NOTIFY statusChanged)
    Q_PROPERTY(int rootCount READ rootCount NOTIFY rootCountChanged)
    Q_PROPERTY(QVariantList roots READ roots NOTIFY rootsChanged)
    Q_PROPERTY(QString selectedRootId READ selectedRootId WRITE setSelectedRootId NOTIFY selectedRootChanged)
    Q_PROPERTY(QString selectedRootName READ selectedRootName NOTIFY selectedRootChanged)
    Q_PROPERTY(int pendingCount READ pendingCount NOTIFY pendingProposalsChanged)
    Q_PROPERTY(QVariantList pendingProposals READ pendingProposals NOTIFY pendingProposalsChanged)

public:
    enum Role {
        DocumentIdRole = Qt::UserRole + 1,
        TitleRole,
        AuthorRole,
        SourceRole,
        FileNameRole,
        FileStemRole,
        FileExtensionRole,
        AvailabilityRole,
        FavoriteRole,
    };
    Q_ENUM(Role)

    enum ViewMode { AllBooks = 0, Favorites = 1, Recent = 2 };
    Q_ENUM(ViewMode)

    explicit LibraryController(std::filesystem::path databasePath, QObject* parent = nullptr);
    ~LibraryController() override;

    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] QString query() const;
    void setQuery(const QString& query);
    [[nodiscard]] int viewMode() const;
    void setViewMode(int mode);
    [[nodiscard]] bool scanning() const;
    [[nodiscard]] QString statusKey() const;
    [[nodiscard]] int lastAddedCount() const;
    [[nodiscard]] int rootCount() const;
    [[nodiscard]] QVariantList roots() const;
    [[nodiscard]] QString selectedRootId() const;
    void setSelectedRootId(const QString& rootId);
    [[nodiscard]] QString selectedRootName() const;
    [[nodiscard]] int pendingCount() const;
    [[nodiscard]] QVariantList pendingProposals() const;

    Q_INVOKABLE void addRoot(const QUrl& folder);
    Q_INVOKABLE void rescan();
    Q_INVOKABLE void setFavorite(const QString& documentId, bool favorite);
    Q_INVOKABLE void openInReader(const QString& documentId);
    Q_INVOKABLE void applyProposal(const QString& proposalId);
    Q_INVOKABLE void dismissProposal(const QString& proposalId);

signals:
    void queryChanged();
    void viewModeChanged();
    void scanningChanged();
    void statusChanged();
    void rootCountChanged();
    void rootsChanged();
    void selectedRootChanged();
    void pendingProposalsChanged();
    void operationError(const QString& message);
    void openDocumentRequested(const QUrl& source);

private:
    void requestRefresh();
    void runOnWorker(std::function<void()> task);
    void reportWorkerError(const QString& message);
    void startScan();
    void beginScan(std::vector<atlas::index::LibraryScanRoot> roots);
    void setScanPreparationFailed(const QString& message);
    [[nodiscard]] static QString availabilityName(atlas::document::Availability availability);
    [[nodiscard]] static QString pathToQString(const std::filesystem::path& path);

    atlas::index::SqliteLibraryIndex index_;
    std::shared_ptr<const atlas::index::NativeLibraryEnumerator> enumerator_;
    atlas::index::LibraryScanner scanner_;
    atlas::index::NativeFileIdentityProvider fileIdentityProvider_;
    atlas::index::LibraryScanService scanService_;
    atlas::index::QtPdfDocumentInspector documentInspector_;
    atlas::index::LibraryIngestionService ingestionService_;
    QFutureWatcher<atlas::index::LibraryIngestionSummary> scanWatcher_;
    QThread workerThread_;
    QObject* workerContext_{};
    std::atomic_bool cancelled_{};
    std::vector<atlas::index::LibraryRecord> records_;
    std::set<std::string> favoriteIds_;
    QVariantList pendingProposals_;
    QVariantList roots_;
    QString query_;
    QString selectedRootId_;
    QString selectedRootName_;
    int viewMode_{AllBooks};
    QString statusKey_{QStringLiteral("ready")};
    int lastAddedCount_{};
    int rootCount_{};
    std::uint64_t refreshRevision_{};
    bool scanPreparationPending_{};
};

} // namespace atlas::app
