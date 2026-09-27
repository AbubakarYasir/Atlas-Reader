#include "app/library/LibraryController.h"

#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QFuture>
#include <QtConcurrentRun>
#include <QUuid>

#include <algorithm>
#include <stdexcept>

namespace atlas::app {
namespace {

[[nodiscard]] std::string toUtf8(const QString& value) {
    const auto bytes = value.toUtf8();
    return {bytes.constData(), static_cast<std::size_t>(bytes.size())};
}

[[nodiscard]] std::filesystem::path toPath(const QString& value) {
#ifdef _WIN32
    return std::filesystem::path{value.toStdWString()};
#else
    const auto bytes = value.toUtf8();
    return std::filesystem::path{std::string{bytes.constData(), static_cast<std::size_t>(bytes.size())}};
#endif
}

} // namespace

LibraryController::LibraryController(std::filesystem::path databasePath, QObject* parent)
    : QAbstractListModel{parent},
      index_{std::move(databasePath)},
      enumerator_{std::make_shared<atlas::index::NativeLibraryEnumerator>()},
      scanner_{enumerator_},
      scanService_{scanner_, index_, fileIdentityProvider_},
      ingestionService_{scanService_, index_, documentInspector_, [] {
          return toUtf8(QUuid::createUuid().toString(QUuid::WithoutBraces));
      }} {
    connect(&scanWatcher_, &QFutureWatcherBase::finished, this, [this] {
        try {
            const auto summary = scanWatcher_.result();
            lastAddedCount_ = static_cast<int>(summary.documentsAdded);
            statusKey_ = summary.inspectionFailures == 0
                ? QStringLiteral("complete")
                : QStringLiteral("completeWithErrors");
        } catch (const std::exception& error) {
            statusKey_ = QStringLiteral("error");
            emit operationError(QString::fromUtf8(error.what()));
        }
        refreshRoots();
        refreshRecords();
        refreshProposals();
        emit scanningChanged();
        emit statusChanged();
    });
    refreshRoots();
    refreshRecords();
    refreshProposals();
}

LibraryController::~LibraryController() {
    cancelled_.store(true);
    scanWatcher_.waitForFinished();
}

int LibraryController::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(records_.size());
}

QVariant LibraryController::data(const QModelIndex& modelIndex, int role) const {
    if (!modelIndex.isValid() || modelIndex.row() < 0
        || modelIndex.row() >= static_cast<int>(records_.size())) return {};
    const auto& record = records_.at(static_cast<std::size_t>(modelIndex.row()));
    switch (role) {
    case DocumentIdRole: return QString::fromUtf8(record.id);
    case TitleRole: return QString::fromUtf8(record.title);
    case AuthorRole: return QString::fromUtf8(record.author);
    case SourceRole: return pathToQString(record.source);
    case FileNameRole: return pathToQString(record.source.filename());
    case AvailabilityRole: return availabilityName(record.availability);
    case FavoriteRole: return favoriteIds_.contains(record.id);
    default: return {};
    }
}

QHash<int, QByteArray> LibraryController::roleNames() const {
    return {
        {DocumentIdRole, "documentId"}, {TitleRole, "bookTitle"}, {AuthorRole, "bookAuthor"},
        {SourceRole, "sourcePath"}, {FileNameRole, "fileName"}, {AvailabilityRole, "availability"},
        {FavoriteRole, "favorite"},
    };
}

QString LibraryController::query() const { return query_; }

void LibraryController::setQuery(const QString& query) {
    const auto normalized = query.trimmed();
    if (query_ == normalized) return;
    query_ = normalized;
    emit queryChanged();
    refreshRecords();
}

int LibraryController::viewMode() const { return viewMode_; }

void LibraryController::setViewMode(int mode) {
    if (mode < AllBooks || mode > Recent || viewMode_ == mode) return;
    viewMode_ = mode;
    if (mode != AllBooks && !selectedRootId_.isEmpty()) {
        selectedRootId_.clear();
        selectedRootName_.clear();
        emit selectedRootChanged();
    }
    emit viewModeChanged();
    refreshRecords();
}

bool LibraryController::scanning() const { return scanWatcher_.isRunning(); }
QString LibraryController::statusKey() const { return statusKey_; }
int LibraryController::lastAddedCount() const { return lastAddedCount_; }
int LibraryController::rootCount() const { return rootCount_; }
QVariantList LibraryController::roots() const { return roots_; }
QString LibraryController::selectedRootId() const { return selectedRootId_; }
QString LibraryController::selectedRootName() const { return selectedRootName_; }

void LibraryController::setSelectedRootId(const QString& rootId) {
    if (selectedRootId_ == rootId) return;
    selectedRootId_ = rootId;
    if (!rootId.isEmpty() && viewMode_ != AllBooks) {
        viewMode_ = AllBooks;
        emit viewModeChanged();
    }
    selectedRootName_.clear();
    for (const auto& value : roots_) {
        const auto root = value.toMap();
        if (root.value(QStringLiteral("id")).toString() == rootId) {
            selectedRootName_ = root.value(QStringLiteral("name")).toString();
            break;
        }
    }
    emit selectedRootChanged();
    refreshRecords();
}
int LibraryController::pendingCount() const { return pendingProposals_.size(); }
QVariantList LibraryController::pendingProposals() const { return pendingProposals_; }

void LibraryController::addRoot(const QUrl& folder) {
    if (scanWatcher_.isRunning()) return;
    const auto localPath = QDir::cleanPath(folder.toLocalFile());
    if (localPath.isEmpty() || !QFileInfo{localPath}.isDir()) {
        emit operationError(QStringLiteral("Choose an existing folder."));
        return;
    }
    try {
        const auto source = toPath(localPath);
        const auto roots = index_.roots();
        const bool exists = std::any_of(roots.begin(), roots.end(), [&](const auto& root) {
            return pathToQString(root.source).compare(localPath, Qt::CaseInsensitive) == 0;
        });
        if (!exists) {
            index_.registerRoot({
                toUtf8(QUuid::createUuid().toString(QUuid::WithoutBraces)),
                source,
                atlas::index::LibraryRootAvailability::available,
            });
            refreshRoots();
        }
        startScan();
    } catch (const std::exception& error) {
        statusKey_ = QStringLiteral("error");
        emit statusChanged();
        emit operationError(QString::fromUtf8(error.what()));
    }
}

void LibraryController::rescan() { startScan(); }

void LibraryController::setFavorite(const QString& documentId, bool favorite) {
    try {
        index_.setFavorite(toUtf8(documentId), favorite);
        refreshRecords();
    } catch (const std::exception& error) {
        emit operationError(QString::fromUtf8(error.what()));
    }
}

void LibraryController::openExternally(const QString& documentId) {
    const auto found = std::find_if(records_.begin(), records_.end(), [&](const auto& record) {
        return QString::fromUtf8(record.id) == documentId;
    });
    if (found == records_.end() || found->source.empty()) return;
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(pathToQString(found->source)))) {
        emit operationError(QStringLiteral("Windows could not open this PDF."));
        return;
    }
    try {
        index_.recordOpened(found->id, QDateTime::currentMSecsSinceEpoch());
        if (viewMode_ == Recent) refreshRecords();
    } catch (const std::exception& error) {
        emit operationError(QString::fromUtf8(error.what()));
    }
}

void LibraryController::applyProposal(const QString& proposalId) {
    try {
        index_.applyReconciliation(toUtf8(proposalId));
        refreshRecords();
        refreshProposals();
    } catch (const std::exception& error) {
        emit operationError(QString::fromUtf8(error.what()));
    }
}

void LibraryController::dismissProposal(const QString& proposalId) {
    try {
        index_.dismissReconciliation(toUtf8(proposalId));
        refreshProposals();
    } catch (const std::exception& error) {
        emit operationError(QString::fromUtf8(error.what()));
    }
}

void LibraryController::refreshRecords() {
    std::vector<atlas::index::LibraryRecord> next;
    try {
        if (!query_.isEmpty()) {
            next = index_.search(toUtf8(query_));
            if (!selectedRootId_.isEmpty()) {
                const auto rootRecords = index_.recordsForRoot(toUtf8(selectedRootId_));
                std::set<std::string> allowed;
                for (const auto& record : rootRecords) allowed.insert(record.id);
                std::erase_if(next, [&](const auto& record) { return !allowed.contains(record.id); });
            }
        }
        else if (viewMode_ == Favorites) next = index_.favorites();
        else if (viewMode_ == Recent) next = index_.recentlyOpened();
        else if (!selectedRootId_.isEmpty()) next = index_.recordsForRoot(toUtf8(selectedRootId_));
        else next = index_.allRecords();
        favoriteIds_.clear();
        for (const auto& favorite : index_.favorites()) favoriteIds_.insert(favorite.id);
        rootCount_ = static_cast<int>(index_.roots().size());
    } catch (const std::exception& error) {
        emit operationError(QString::fromUtf8(error.what()));
    }
    beginResetModel();
    records_ = std::move(next);
    endResetModel();
    emit rootCountChanged();
}

void LibraryController::refreshRoots() {
    QVariantList next;
    try {
        for (const auto& root : index_.roots()) {
            QVariantMap item;
            item.insert(QStringLiteral("id"), QString::fromUtf8(root.id));
            item.insert(QStringLiteral("path"), pathToQString(root.source));
            const auto name = pathToQString(root.source.filename());
            item.insert(QStringLiteral("name"), name.isEmpty() ? pathToQString(root.source) : name);
            item.insert(QStringLiteral("availability"),
                root.availability == atlas::index::LibraryRootAvailability::available
                    ? QStringLiteral("available")
                    : root.availability == atlas::index::LibraryRootAvailability::offline
                        ? QStringLiteral("offline") : QStringLiteral("partial"));
            next.push_back(item);
        }
    } catch (const std::exception& error) {
        emit operationError(QString::fromUtf8(error.what()));
    }
    roots_ = std::move(next);
    rootCount_ = roots_.size();
    emit rootsChanged();
    emit rootCountChanged();
}

void LibraryController::refreshProposals() {
    QVariantList next;
    try {
        for (const auto& proposal : index_.reconciliationProposals()) {
            if (proposal.state != atlas::index::ReconciliationProposalState::pending) continue;
            QVariantMap item;
            item.insert(QStringLiteral("id"), QString::fromUtf8(proposal.id));
            item.insert(QStringLiteral("beforePath"), pathToQString(proposal.previousSource));
            item.insert(QStringLiteral("candidatePath"), pathToQString(proposal.candidateSource));
            item.insert(QStringLiteral("canApply"), proposal.mayRelinkAutomatically);
            item.insert(QStringLiteral("decision"), proposal.mayRelinkAutomatically
                ? QStringLiteral("move") : QStringLiteral("ambiguous"));
            next.push_back(item);
        }
    } catch (const std::exception& error) {
        emit operationError(QString::fromUtf8(error.what()));
    }
    pendingProposals_ = std::move(next);
    emit pendingProposalsChanged();
}

void LibraryController::startScan() {
    if (scanWatcher_.isRunning()) return;
    const auto persistedRoots = index_.roots();
    if (persistedRoots.empty()) {
        statusKey_ = QStringLiteral("noRoots");
        emit statusChanged();
        return;
    }
    std::vector<atlas::index::LibraryScanRoot> roots;
    roots.reserve(persistedRoots.size());
    for (const auto& root : persistedRoots) roots.push_back({root.id, root.source});
    cancelled_.store(false);
    lastAddedCount_ = 0;
    statusKey_ = QStringLiteral("scanning");
    emit statusChanged();
    emit scanningChanged();
    scanWatcher_.setFuture(QtConcurrent::run([this, roots = std::move(roots)] {
        return ingestionService_.ingest(
            roots, cancelled_, QDateTime::currentMSecsSinceEpoch());
    }));
}

QString LibraryController::availabilityName(atlas::document::Availability availability) {
    switch (availability) {
    case atlas::document::Availability::available: return QStringLiteral("available");
    case atlas::document::Availability::locked: return QStringLiteral("locked");
    case atlas::document::Availability::readOnly: return QStringLiteral("readOnly");
    case atlas::document::Availability::offline: return QStringLiteral("offline");
    case atlas::document::Availability::missing: return QStringLiteral("missing");
    case atlas::document::Availability::cloudPlaceholder: return QStringLiteral("cloud");
    case atlas::document::Availability::unreadable: return QStringLiteral("unreadable");
    case atlas::document::Availability::unsupported: return QStringLiteral("unsupported");
    }
    return QStringLiteral("unknown");
}

QString LibraryController::pathToQString(const std::filesystem::path& path) {
#ifdef _WIN32
    return QString::fromStdWString(path.wstring());
#else
    const auto bytes = path.generic_u8string();
    return QString::fromUtf8(reinterpret_cast<const char*>(bytes.data()), static_cast<qsizetype>(bytes.size()));
#endif
}

} // namespace atlas::app
