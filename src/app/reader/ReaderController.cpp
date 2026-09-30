#include "app/reader/ReaderController.h"

#include "app/index/LibraryMetadataPolicy.h"

#include <QDir>
#include <QFileInfo>
#include <QPdfDocument>
#include <QSettings>
#include <QTimer>
#include <QtConcurrentRun>

#include <algorithm>
#include <utility>

namespace atlas::app {
namespace {

[[nodiscard]] std::string toUtf8(const QString& value) {
    const auto bytes = value.toUtf8();
    return {bytes.constData(), static_cast<std::size_t>(bytes.size())};
}

[[nodiscard]] QString fromUtf8(const std::string& value) {
    return QString::fromUtf8(value.data(), static_cast<qsizetype>(value.size()));
}

[[nodiscard]] QString pathToQString(const std::filesystem::path& path) {
#ifdef _WIN32
    return QString::fromStdWString(path.wstring());
#else
    const auto utf8 = path.u8string();
    return QString::fromUtf8(reinterpret_cast<const char*>(utf8.data()), static_cast<qsizetype>(utf8.size()));
#endif
}

[[nodiscard]] atlas::reader::OpenResult inspectPdf(const QString& sourcePath) {
    using atlas::reader::OpenResult;
    using atlas::reader::OpenState;
    const QFileInfo fileInfo{sourcePath};
    if (!fileInfo.exists() || !fileInfo.isFile()) {
        return {OpenState::missing, {}, 0, "The file is no longer available."};
    }
    QPdfDocument document;
    switch (const auto error = document.load(sourcePath)) {
    case QPdfDocument::Error::None: {
        const QString title = atlas::index::libraryDisplayTitle(
            document.metaData(QPdfDocument::MetaDataField::Title).toString(),
            fileInfo.completeBaseName());
        return {OpenState::ready, toUtf8(title), document.pageCount(), {}};
    }
    case QPdfDocument::Error::FileNotFound:
        return {OpenState::missing, {}, 0, "The file is no longer available."};
    case QPdfDocument::Error::IncorrectPassword:
        return {OpenState::passwordRequired, {}, 0, "This PDF requires a password."};
    case QPdfDocument::Error::UnsupportedSecurityScheme:
        return {OpenState::unsupportedSecurity, {}, 0, "This PDF uses unsupported security."};
    case QPdfDocument::Error::InvalidFileFormat:
        return {OpenState::malformed, {}, 0, "This file is not a readable PDF."};
    case QPdfDocument::Error::DataNotYetAvailable:
    case QPdfDocument::Error::Unknown:
        return {OpenState::failed, {}, 0, "Atlas could not open this PDF."};
    }
    return {OpenState::failed, {}, 0, "Atlas could not open this PDF."};
}

} // namespace

ReaderController::ReaderController(std::filesystem::path profileDirectory, QObject* parent)
    : QAbstractListModel{parent}, profileDirectory_{std::move(profileDirectory)} {
    restoreSessions();
}

ReaderController::~ReaderController() {
    for (auto& pending : pending_) pending.second->watcher->waitForFinished();
}

int ReaderController::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(model_.sessions().size());
}

QVariant ReaderController::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= rowCount()) return {};
    const auto& session = model_.sessions().at(static_cast<std::size_t>(index.row()));
    switch (role) {
    case SessionIdRole: return QVariant::fromValue<qulonglong>(session.id);
    case TitleRole: return fromUtf8(session.title);
    case SourceRole: return pathToQString(session.source);
    case StatusRole: return QString::fromLatin1(atlas::reader::openStateName(session.state));
    case PageCountRole: return session.pageCount;
    case DetailRole: return fromUtf8(session.detail);
    default: return {};
    }
}

QHash<int, QByteArray> ReaderController::roleNames() const {
    return {{SessionIdRole, "sessionId"}, {TitleRole, "title"}, {SourceRole, "sourcePath"},
        {StatusRole, "status"}, {PageCountRole, "pageCount"}, {DetailRole, "detail"}};
}

int ReaderController::activeIndex() const { return libraryVisible_ ? -1 : model_.activeIndex(); }

void ReaderController::setActiveIndex(int index) {
    if (index < 0 || index >= rowCount()) return;
    const int previous = activeIndex();
    libraryVisible_ = false;
    const bool activated = model_.activate(model_.sessions().at(static_cast<std::size_t>(index)).id);
    Q_ASSERT(activated);
    if (previous != activeIndex()) emit activeIndexChanged();
    saveSessions();
}

bool ReaderController::hasSessions() const { return !model_.sessions().empty(); }
bool ReaderController::restoreEnabled() const { return restoreEnabled_; }

void ReaderController::setRestoreEnabled(bool enabled) {
    if (restoreEnabled_ == enabled) return;
    restoreEnabled_ = enabled;
    emit restoreEnabledChanged();
    saveSessions();
}

void ReaderController::openLocalFile(const QUrl& file) {
    if (file.isLocalFile()) beginOpen(QDir::cleanPath(file.toLocalFile()));
}

void ReaderController::closeAt(int index) {
    if (index < 0 || index >= rowCount()) return;
    const int previousActive = activeIndex();
    const int previousCount = rowCount();
    const auto id = model_.sessions().at(static_cast<std::size_t>(index)).id;
    if (auto pending = pending_.find(id); pending != pending_.end()) {
        pending->second->watcher->cancel();
        pending_.erase(pending);
    }
    beginResetModel();
    const bool closed = model_.close(id);
    Q_ASSERT(closed);
    endResetModel();
    if (model_.sessions().empty()) libraryVisible_ = true;
    refreshModel(previousActive, previousCount);
    saveSessions();
}

void ReaderController::retryAt(int index) {
    if (index < 0 || index >= rowCount()) return;
    const int previousActive = activeIndex();
    const auto& session = model_.sessions().at(static_cast<std::size_t>(index));
    const auto id = session.id;
    const QString sourcePath = pathToQString(session.source);
    const auto revision = model_.reopen(id);
    if (!revision.has_value()) return;
    libraryVisible_ = false;
    emit dataChanged(this->index(index), this->index(index));
    if (previousActive != activeIndex()) emit activeIndexChanged();
    launchOpen(id, *revision, sourcePath);
}

void ReaderController::closeActive() { closeAt(activeIndex()); }

void ReaderController::showLibrary() {
    if (!libraryVisible_) {
        libraryVisible_ = true;
        emit activeIndexChanged();
    }
}

void ReaderController::beginOpen(const QString& sourcePath) {
    const QFileInfo fileInfo{sourcePath};
    const int previousActive = activeIndex();
    const int previousCount = rowCount();
    beginResetModel();
    const auto result = model_.beginOpen(
        toPath(sourcePath), toUtf8(normalizedSourceKey(sourcePath)), toUtf8(fileInfo.completeBaseName()));
    libraryVisible_ = false;
    endResetModel();
    if (result.needsOpen) launchOpen(result.sessionId, result.revision, sourcePath);
    refreshModel(previousActive, previousCount);
    saveSessions();
}

void ReaderController::launchOpen(
    std::uint64_t sessionId, std::uint64_t revision, const QString& sourcePath) {
    auto pending = std::make_unique<PendingOpen>();
    pending->sessionId = sessionId;
    pending->revision = revision;
    pending->watcher = std::make_unique<QFutureWatcher<atlas::reader::OpenResult>>();
    auto* watcher = pending->watcher.get();
    connect(watcher, &QFutureWatcherBase::finished, this, [this, sessionId, revision, watcher] {
        const auto& sessions = model_.sessions();
        const auto found = std::find_if(sessions.begin(), sessions.end(), [sessionId](const auto& session) {
            return session.id == sessionId;
        });
        const int row = found == sessions.end() ? -1 : static_cast<int>(std::distance(sessions.begin(), found));
        if (!watcher->isCanceled() && model_.completeOpen(sessionId, revision, watcher->result()) && row >= 0) {
            emit dataChanged(index(row), index(row));
            saveSessions();
        }
        QTimer::singleShot(0, this, [this, sessionId] { pending_.erase(sessionId); });
    });
    pending_.emplace(sessionId, std::move(pending));
    watcher->setFuture(QtConcurrent::run([sourcePath] { return inspectPdf(sourcePath); }));
}

void ReaderController::refreshModel(int previousActiveIndex, int previousCount) {
    if (previousCount != rowCount()) emit countChanged();
    if (previousActiveIndex != activeIndex()) emit activeIndexChanged();
}

void ReaderController::restoreSessions() {
    QSettings settings{settingsPath(), QSettings::IniFormat};
    restoreEnabled_ = settings.value(QStringLiteral("restoreEnabled"), false).toBool();
    if (!restoreEnabled_) return;
    for (const QString& source : settings.value(QStringLiteral("sources")).toStringList()) beginOpen(source);
}

void ReaderController::saveSessions() const {
    QDir{}.mkpath(pathToQString(profileDirectory_));
    QSettings settings{settingsPath(), QSettings::IniFormat};
    settings.setValue(QStringLiteral("restoreEnabled"), restoreEnabled_);
    if (!restoreEnabled_) {
        settings.remove(QStringLiteral("sources"));
        return;
    }
    QStringList sources;
    for (const auto& session : model_.sessions()) sources.push_back(pathToQString(session.source));
    settings.setValue(QStringLiteral("sources"), sources);
}

QString ReaderController::settingsPath() const {
    return QDir{pathToQString(profileDirectory_)}.filePath(QStringLiteral("reader-session.ini"));
}

QString ReaderController::normalizedSourceKey(const QString& sourcePath) {
    QString key = QDir::cleanPath(QFileInfo{sourcePath}.absoluteFilePath());
#ifdef _WIN32
    key = key.toCaseFolded();
#endif
    return key;
}

std::filesystem::path ReaderController::toPath(const QString& sourcePath) {
#ifdef _WIN32
    return std::filesystem::path{sourcePath.toStdWString()};
#else
    const auto bytes = sourcePath.toUtf8();
    return std::filesystem::path{std::string{bytes.constData(), static_cast<std::size_t>(bytes.size())}};
#endif
}

} // namespace atlas::app
