#include "app/bookmarks/BookmarkModel.h"

#include "core/bookmarks/LocationEnvelope.h"

#include <algorithm>
#include <cmath>

namespace atlas::app {

namespace {

using bookmarks::BookmarkNode;
using bookmarks::StorageState;

[[nodiscard]] std::string toUtf8(const QString& text) {
    const QByteArray bytes = text.toUtf8();
    return {bytes.constData(), static_cast<std::size_t>(bytes.size())};
}

[[nodiscard]] QString fromUtf8(const std::string& text) {
    return QString::fromUtf8(text.data(), static_cast<qsizetype>(text.size()));
}

[[nodiscard]] int pageOfLocation(const std::string& location) {
    if (location.empty()) return -1;
    const auto decoded = bookmarks::decodeLocation(location);
    if (decoded.status != bookmarks::LocationDecodeStatus::ok) return -1;
    return static_cast<int>(decoded.envelope->pdf.pageIndex);
}

// {page, fx, fy} from the viewport → an XYZ envelope that keeps the reader's zoom.
[[nodiscard]] std::optional<std::string> locationFrom(const QVariantMap& location) {
    if (!location.contains(QStringLiteral("page"))) return std::nullopt;
    const int page = location.value(QStringLiteral("page")).toInt();
    if (page < 0) return std::nullopt;
    bookmarks::LocationEnvelope envelope;
    envelope.pdf.pageIndex = static_cast<std::uint32_t>(page);
    envelope.pdf.kind = bookmarks::DestinationKind::xyz;
    const double fx = location.value(QStringLiteral("fx")).toDouble();
    const double fy = location.value(QStringLiteral("fy")).toDouble();
    envelope.pdf.left = std::isfinite(fx) ? std::clamp(fx, 0.0, 1.0) : 0.0;
    envelope.pdf.top = std::isfinite(fy) ? std::clamp(fy, 0.0, 1.0) : 0.0;
    return bookmarks::encodeLocation(envelope);
}

[[nodiscard]] QString editKindText(std::optional<bookmarks::EditKind> kind) {
    if (!kind) return {};
    const auto name = bookmarks::editKindName(*kind);
    return QString::fromLatin1(name.data(), static_cast<qsizetype>(name.size()));
}

} // namespace

BookmarkModel::BookmarkModel(QObject* parent) : QAbstractListModel(parent) {}

BookmarkModel::~BookmarkModel() { flushWrites(); }

int BookmarkModel::rowCount(const QModelIndex& parent) const {
    return parent.isValid() ? 0 : static_cast<int>(rows_.size());
}

QHash<int, QByteArray> BookmarkModel::roleNames() const {
    return {
        {NodeIdRole, "nodeId"},         {TitleRole, "title"},   {DepthRole, "depth"},
        {HasChildrenRole, "hasChildren"}, {ExpandedRole, "expanded"}, {StorageRole, "storage"},
        {PageRole, "page"},             {FxRole, "fx"},         {FyRole, "fy"},
        {BreadcrumbRole, "breadcrumb"}, {MatchRole, "match"},   {NotesRole, "notes"},
        {TagsRole, "tags"},             {FromPdfRole, "fromPdf"},
    };
}

const BookmarkNode* BookmarkModel::nodeAt(int row) const {
    if (!editor_ || row < 0 || row >= static_cast<int>(rows_.size())) return nullptr;
    return editor_->find(rows_[static_cast<std::size_t>(row)].id);
}

QVariant BookmarkModel::data(const QModelIndex& index, int role) const {
    const auto* node = nodeAt(index.row());
    if (node == nullptr) return {};
    switch (role) {
    case NodeIdRole: return fromUtf8(node->id);
    case TitleRole: return fromUtf8(node->title);
    case DepthRole: return rows_[static_cast<std::size_t>(index.row())].depth;
    case HasChildrenRole: {
        if (!filter_.isEmpty()) return false;
        for (const auto& child : editor_->children(node->id)) {
            const auto* kid = editor_->find(child);
            if (kid != nullptr && isVisibleNode(*kid)) return true;
        }
        return false;
    }
    case ExpandedRole: return filter_.isEmpty() && expanded_.contains(fromUtf8(node->id));
    case StorageRole: {
        const auto name = bookmarks::stateName(node->state);
        return QString::fromLatin1(name.data(), static_cast<qsizetype>(name.size()));
    }
    case PageRole: {
        const auto found = pageOf_.find(node->id);
        return found == pageOf_.end() ? -1 : found->second;
    }
    case FxRole:
    case FyRole: {
        const auto decoded = bookmarks::decodeLocation(node->location);
        if (decoded.status != bookmarks::LocationDecodeStatus::ok) return 0.0;
        const auto& value = role == FxRole ? decoded.envelope->pdf.left : decoded.envelope->pdf.top;
        return value.value_or(0.0);
    }
    case BreadcrumbRole: {
        if (filter_.isEmpty()) return QString{};
        auto titles = editor_->breadcrumb(node->id);
        if (!titles.empty()) titles.pop_back();
        QStringList parts;
        for (const auto& title : titles) parts.push_back(fromUtf8(title));
        return parts;
    }
    case MatchRole: {
        for (const auto& hit : hits_) {
            if (hit.id != node->id) continue;
            switch (hit.field) {
            case bookmarks::BookmarkField::title: return QStringLiteral("title");
            case bookmarks::BookmarkField::notes: return QStringLiteral("notes");
            case bookmarks::BookmarkField::tag: return QStringLiteral("tag");
            }
        }
        return QString{};
    }
    case NotesRole: return fromUtf8(node->description);
    case TagsRole: {
        QStringList tags;
        for (const auto& tag : node->tags) tags.push_back(fromUtf8(tag));
        return tags;
    }
    case FromPdfRole: return node->baseKey.has_value();
    default: return {};
    }
}

void BookmarkModel::setService(BookmarkService* service) {
    if (service_ == service) return;
    service_ = service;
    emit serviceChanged();
}

bool BookmarkModel::busy() const { return writing_ || !writes_.empty(); }
bool BookmarkModel::canUndo() const { return editable() && editor_ && editor_->canUndo(); }
bool BookmarkModel::canRedo() const { return editable() && editor_ && editor_->canRedo(); }
QString BookmarkModel::undoKind() const { return editor_ ? editKindText(editor_->undoKind()) : QString{}; }
QString BookmarkModel::redoKind() const { return editor_ ? editKindText(editor_->redoKind()) : QString{}; }
int BookmarkModel::totalCount() const { return editor_ ? static_cast<int>(editor_->size()) : 0; }

void BookmarkModel::setStatus(const QString& status, const QString& reason) {
    if (status_ == status && statusReason_ == reason) return;
    status_ = status;
    statusReason_ = reason;
    emit statusChanged();
    emit historyChanged();
}

void BookmarkModel::attach(const QString& path, const QVariantList& outline) {
    detach();
    path_ = path;
    const auto generation = ++generation_;
    setStatus(QStringLiteral("loading"), {});
    if (!service_) {
        // No storage (tests or a failed profile): the PDF's outline, read-only.
        auto base = baseFromOutline(outline, "unsaved");
        std::vector<BookmarkNode> nodes;
        if (base) nodes = bookmarks::embeddedNodesFromBase("unsaved", *base);
        adoptSnapshot(std::move(nodes), std::move(base), 0);
        setStatus(QStringLiteral("readOnly"), QStringLiteral("storageError"));
        return;
    }
    service_->open(path, outline, this, [this, generation](BookmarkOpenResult result) {
        if (generation != generation_) return;
        using Status = BookmarkOpenResult::Status;
        if (result.status == Status::editable) {
            documentId_ = result.documentId;
            outlineChangedOutside_ = result.adoption == bookmarks::BaseAdoption::needsReconcile;
            adoptSnapshot(std::move(result.snapshot.nodes), std::move(result.snapshot.base),
                          result.snapshot.revision);
            setStatus(QStringLiteral("editable"), {});
            return;
        }
        std::vector<BookmarkNode> nodes;
        if (result.base) nodes = bookmarks::embeddedNodesFromBase("read-only", *result.base);
        adoptSnapshot(std::move(nodes), result.base, 0);
        const QString reason = result.status == Status::notInLibrary ? QStringLiteral("notInLibrary")
                               : result.status == Status::ambiguous  ? QStringLiteral("ambiguous")
                               : result.status == Status::outlineUnsupported
                                   ? QStringLiteral("outlineUnsupported")
                                   : QStringLiteral("storageError");
        setStatus(QStringLiteral("readOnly"), reason);
    });
}

void BookmarkModel::flushWrites() {
    // Queued saves still go to disk, in order, after the tab lets go.
    if (service_ && !documentId_.empty()) {
        for (auto& pending : writes_) {
            service_->write(documentId_, pending.expectedRevision, std::move(pending.change), service_,
                            [](BookmarkWriteResult) {});
        }
    }
    writes_.clear();
}

void BookmarkModel::detach() {
    flushWrites();
    ++generation_;
    beginResetModel();
    rows_.clear();
    editor_.reset();
    search_.reset();
    hits_.clear();
    pageOf_.clear();
    expanded_.clear();
    endResetModel();
    documentId_.clear();
    path_.clear();
    writes_.clear();
    writing_ = false;
    outlineChangedOutside_ = false;
    hiddenCount_ = 0;
    if (!currentId_.isEmpty()) {
        currentId_.clear();
        emit currentIdChanged();
    }
    setStatus(QStringLiteral("none"), {});
    emit countsChanged();
}

void BookmarkModel::adoptSnapshot(std::vector<BookmarkNode> nodes, std::optional<bookmarks::BaseSnapshot> base,
                                  std::int64_t revision) {
    const std::string documentId = documentId_.empty() ? std::string{"read-only"} : documentId_;
    if (editor_) editor_->reset(std::move(nodes), std::move(base), revision);
    else editor_ = std::make_unique<bookmarks::BookmarkEditor>(documentId, std::move(nodes), std::move(base), revision);
    pageOf_.clear();
    for (const auto* node : editor_->inOrder()) pageOf_[node->id] = pageOfLocation(node->location);
    searchStale_ = true;
    beginResetModel();
    rows_.clear();
    endResetModel();
    rebuildRows();
    emit historyChanged();
}

bool BookmarkModel::isVisibleNode(const BookmarkNode& node) const {
    return showHidden_ || node.state != StorageState::locallyHidden;
}

void BookmarkModel::rebuildSearch() {
    if (!editor_) return;
    if (searchStale_) {
        search_ = std::make_unique<bookmarks::BookmarkSearchIndex>(editor_->inOrder());
        searchStale_ = false;
    }
    hits_ = filter_.isEmpty() ? std::vector<bookmarks::BookmarkHit>{}
                              : search_->find(toUtf8(filter_), 500, showHidden_);
}

void BookmarkModel::rebuildRows() {
    std::vector<Row> next;
    if (editor_) {
        if (!filter_.isEmpty()) {
            rebuildSearch();
            next.reserve(hits_.size());
            for (const auto& hit : hits_) next.push_back({hit.id, 0});
        } else {
            // Depth first through expanded, visible nodes.
            std::vector<std::pair<const std::vector<std::string>*, std::size_t>> stack{
                {&editor_->children(std::nullopt), 0}};
            while (!stack.empty()) {
                auto& [list, at] = stack.back();
                if (at >= list->size()) {
                    stack.pop_back();
                    continue;
                }
                const auto& id = (*list)[at++];
                const auto* node = editor_->find(id);
                if (node == nullptr || !isVisibleNode(*node)) continue;
                const int depth = static_cast<int>(stack.size()) - 1;
                next.push_back({id, depth});
                if (expanded_.contains(fromUtf8(id))) stack.emplace_back(&editor_->children(id), 0);
            }
        }
    }

    // Keep the unchanged head and tail so the list keeps its place and focus.
    const auto same = [](const Row& a, const Row& b) { return a.id == b.id && a.depth == b.depth; };
    std::size_t head = 0;
    while (head < rows_.size() && head < next.size() && same(rows_[head], next[head])) ++head;
    std::size_t tail = 0;
    while (tail < rows_.size() - head && tail < next.size() - head &&
           same(rows_[rows_.size() - 1 - tail], next[next.size() - 1 - tail])) {
        ++tail;
    }
    const auto oldMiddle = rows_.size() - head - tail;
    const auto newMiddle = next.size() - head - tail;
    if (oldMiddle > 0) {
        beginRemoveRows({}, static_cast<int>(head), static_cast<int>(head + oldMiddle - 1));
        rows_.erase(rows_.begin() + static_cast<std::ptrdiff_t>(head),
                    rows_.begin() + static_cast<std::ptrdiff_t>(head + oldMiddle));
        endRemoveRows();
    }
    if (newMiddle > 0) {
        beginInsertRows({}, static_cast<int>(head), static_cast<int>(head + newMiddle - 1));
        rows_.insert(rows_.begin() + static_cast<std::ptrdiff_t>(head),
                     next.begin() + static_cast<std::ptrdiff_t>(head),
                     next.begin() + static_cast<std::ptrdiff_t>(head + newMiddle));
        endInsertRows();
    }
    if (!rows_.empty()) emit dataChanged(index(0), index(static_cast<int>(rows_.size()) - 1));
    updateCounts();
    updateCurrent();
}

void BookmarkModel::updateCounts() {
    int hidden = 0;
    if (editor_) {
        for (const auto* node : editor_->inOrder()) {
            if (node->state == StorageState::locallyHidden) ++hidden;
        }
    }
    hiddenCount_ = hidden;
    emit countsChanged();
}

void BookmarkModel::updateCurrent() {
    QString best;
    if (editor_ && currentPage_ >= 0) {
        for (const auto* node : editor_->inOrder()) {
            if (!isVisibleNode(*node)) continue;
            const auto found = pageOf_.find(node->id);
            if (found != pageOf_.end() && found->second >= 0 && found->second <= currentPage_) {
                best = fromUtf8(node->id);
            }
        }
    }
    if (best == currentId_) return;
    currentId_ = best;
    emit currentIdChanged();
}

void BookmarkModel::setFilter(const QString& filter) {
    if (filter_ == filter) return;
    filter_ = filter;
    emit filterChanged();
    rebuildRows();
}

void BookmarkModel::setShowHidden(bool show) {
    if (showHidden_ == show) return;
    showHidden_ = show;
    emit showHiddenChanged();
    rebuildRows();
}

void BookmarkModel::setCurrentPage(int page) {
    if (currentPage_ == page) return;
    currentPage_ = page;
    emit currentPageChanged();
    updateCurrent();
}

void BookmarkModel::toggle(int row) {
    const auto* node = nodeAt(row);
    if (node == nullptr) return;
    setExpanded(row, !expanded_.contains(fromUtf8(node->id)));
}

void BookmarkModel::setExpanded(int row, bool open) {
    const auto* node = nodeAt(row);
    if (node == nullptr || !filter_.isEmpty()) return;
    const QString id = fromUtf8(node->id);
    if (open == expanded_.contains(id)) return;
    if (open) expanded_.insert(id);
    else expanded_.remove(id);
    rebuildRows();
}

void BookmarkModel::expandAll() {
    if (!editor_) return;
    for (const auto* node : editor_->inOrder()) {
        if (!editor_->children(node->id).empty()) expanded_.insert(fromUtf8(node->id));
    }
    rebuildRows();
}

void BookmarkModel::collapseAll() {
    expanded_.clear();
    rebuildRows();
}

int BookmarkModel::rowOf(const QString& id) const {
    const auto key = toUtf8(id);
    for (std::size_t i = 0; i < rows_.size(); ++i) {
        if (rows_[i].id == key) return static_cast<int>(i);
    }
    return -1;
}

QVariantMap BookmarkModel::get(int row) const {
    QVariantMap result;
    if (nodeAt(row) == nullptr) return result;
    const auto names = roleNames();
    for (auto it = names.cbegin(); it != names.cend(); ++it) {
        result.insert(QString::fromLatin1(it.value()), data(index(row), it.key()));
    }
    return result;
}

int BookmarkModel::reveal(const QString& id) {
    if (!editor_) return -1;
    const auto* node = editor_->find(toUtf8(id));
    if (node == nullptr) return -1;
    bool changed = false;
    for (auto parent = node->parentId; parent;) {
        changed = !expanded_.contains(fromUtf8(*parent)) || changed;
        expanded_.insert(fromUtf8(*parent));
        const auto* up = editor_->find(*parent);
        parent = up == nullptr ? std::nullopt : up->parentId;
    }
    if (changed) rebuildRows();
    return rowOf(id);
}

int BookmarkModel::run(bookmarks::EditResult result, const std::string& focusId) {
    if (!result.ok) {
        emit refused(QString::fromStdString(result.reason));
        return -1;
    }
    for (const auto& node : result.change.upserts) pageOf_[node.id] = pageOfLocation(node.location);
    for (const auto& id : result.change.removals) pageOf_.erase(id);
    searchStale_ = true;
    persist(result);
    rebuildRows();
    emit historyChanged();
    return focusId.empty() ? -1 : reveal(fromUtf8(focusId));
}

void BookmarkModel::persist(const bookmarks::EditResult& result) {
    const auto expected = editor_->revision();
    if (result.change.empty()) {
        editor_->confirm(expected);
        return;
    }
    // Confirm at once with the revision this change will produce; the write
    // follows on the worker in order.
    editor_->confirm(expected + 1);
    writes_.push_back({expected, {result.change.upserts, result.change.removals, result.change.restores}});
    writeNext();
}

void BookmarkModel::writeNext() {
    if (writing_ || writes_.empty() || !service_) return;
    writing_ = true;
    emit historyChanged();
    auto next = std::move(writes_.front());
    writes_.erase(writes_.begin());
    const auto generation = generation_;
    const auto expected = next.expectedRevision;
    service_->write(documentId_, expected, std::move(next.change), this,
                    [this, generation, expected](BookmarkWriteResult written) {
                        if (generation != generation_ || !editor_) return;
                        writing_ = false;
                        if (!written.ok || written.revision != expected + 1) {
                            recoverFromFailedWrite(QString::fromStdString(
                                written.ok ? std::string{"revisionConflict"} : written.reason));
                            return;
                        }
                        writeNext();
                        emit historyChanged();
                    });
}

void BookmarkModel::recoverFromFailedWrite(const QString& reason) {
    // Drop the queued changes and show exactly what is stored.
    writes_.clear();
    const auto generation = ++generation_;
    writing_ = true;
    emit historyChanged();
    service_->reload(documentId_, this,
                     [this, generation, reason](std::optional<bookmarks::OverlaySnapshot> snapshot) {
                         if (generation != generation_) return;
                         writing_ = false;
                         if (!snapshot) {
                             setStatus(QStringLiteral("readOnly"), QStringLiteral("storageError"));
                             return;
                         }
                         adoptSnapshot(std::move(snapshot->nodes), std::move(snapshot->base), snapshot->revision);
                         emit reloaded(reason);
                     });
}

int BookmarkModel::addBookmark(int row, bool asChild, const QString& title, const QVariantMap& location) {
    if (!editable() || !editor_) {
        emit refused(QStringLiteral("readOnly"));
        return -1;
    }
    const auto encoded = locationFrom(location);
    std::optional<std::string> parent;
    std::size_t at = editor_->children(std::nullopt).size();
    if (const auto* anchor = nodeAt(row)) {
        if (asChild) {
            parent = anchor->id;
            at = editor_->children(anchor->id).size();
        } else {
            parent = anchor->parentId;
            at = anchor->position + 1;
        }
    }
    auto result = editor_->create(parent, at, toUtf8(title), encoded.value_or(std::string{}));
    const auto id = result.nodeId;
    return run(std::move(result), id);
}

int BookmarkModel::rename(int row, const QString& title) {
    const auto* node = nodeAt(row);
    if (!editable() || node == nullptr) {
        emit refused(QStringLiteral("readOnly"));
        return -1;
    }
    if (fromUtf8(node->title) == title) return row;
    const auto id = node->id;
    return run(editor_->rename(id, toUtf8(title)), id);
}

int BookmarkModel::setDestination(int row, const QVariantMap& location) {
    const auto* node = nodeAt(row);
    const auto encoded = locationFrom(location);
    if (!editable() || node == nullptr || !encoded) {
        emit refused(QStringLiteral("readOnly"));
        return -1;
    }
    const auto id = node->id;
    return run(editor_->setLocation(id, *encoded), id);
}

int BookmarkModel::setNotes(int row, const QString& notes, const QStringList& tags) {
    const auto* node = nodeAt(row);
    if (!editable() || node == nullptr) {
        emit refused(QStringLiteral("readOnly"));
        return -1;
    }
    std::vector<std::string> list;
    for (const QString& tag : tags) {
        if (!tag.isEmpty()) list.push_back(toUtf8(tag));
    }
    const auto id = node->id;
    return run(editor_->setNotes(id, toUtf8(notes), std::move(list)), id);
}

int BookmarkModel::remove(int row) {
    const auto* node = nodeAt(row);
    if (!editable() || node == nullptr) {
        emit refused(QStringLiteral("readOnly"));
        return -1;
    }
    const auto id = node->id;
    // Focus the bookmark that takes its place.
    std::string focus;
    if (row + 1 < static_cast<int>(rows_.size())) focus = rows_[static_cast<std::size_t>(row + 1)].id;
    else if (row > 0) focus = rows_[static_cast<std::size_t>(row - 1)].id;
    const bool fromPdf = node->baseKey.has_value();
    const int result = run(editor_->remove(id), fromPdf && showHidden_ ? id : focus);
    return result;
}

int BookmarkModel::moveUp(int row) {
    const auto* node = nodeAt(row);
    if (!editable() || node == nullptr || !filter_.isEmpty()) {
        emit refused(QStringLiteral("readOnly"));
        return -1;
    }
    const auto& siblings = editor_->children(node->parentId);
    for (std::size_t i = node->position; i-- > 0;) {
        const auto* sibling = editor_->find(siblings[i]);
        if (sibling != nullptr && isVisibleNode(*sibling)) {
            const auto id = node->id;
            return run(editor_->move(id, node->parentId, i), id);
        }
    }
    emit refused(QStringLiteral("atTop"));
    return -1;
}

int BookmarkModel::moveDown(int row) {
    const auto* node = nodeAt(row);
    if (!editable() || node == nullptr || !filter_.isEmpty()) {
        emit refused(QStringLiteral("readOnly"));
        return -1;
    }
    const auto& siblings = editor_->children(node->parentId);
    for (std::size_t i = node->position + 1; i < siblings.size(); ++i) {
        const auto* sibling = editor_->find(siblings[i]);
        if (sibling != nullptr && isVisibleNode(*sibling)) {
            const auto id = node->id;
            return run(editor_->move(id, node->parentId, i), id);
        }
    }
    emit refused(QStringLiteral("atBottom"));
    return -1;
}

int BookmarkModel::indent(int row) {
    const auto* node = nodeAt(row);
    if (!editable() || node == nullptr || !filter_.isEmpty()) {
        emit refused(QStringLiteral("readOnly"));
        return -1;
    }
    const auto& siblings = editor_->children(node->parentId);
    for (std::size_t i = node->position; i-- > 0;) {
        const auto* sibling = editor_->find(siblings[i]);
        if (sibling == nullptr || !isVisibleNode(*sibling)) continue;
        const auto id = node->id;
        const auto target = sibling->id;
        expanded_.insert(fromUtf8(target));
        return run(editor_->move(id, target, editor_->children(target).size()), id);
    }
    emit refused(QStringLiteral("nothingToIndentInto"));
    return -1;
}

int BookmarkModel::outdent(int row) {
    const auto* node = nodeAt(row);
    if (!editable() || node == nullptr || !filter_.isEmpty()) {
        emit refused(QStringLiteral("readOnly"));
        return -1;
    }
    if (!node->parentId) {
        emit refused(QStringLiteral("atTopLevel"));
        return -1;
    }
    const auto* parent = editor_->find(*node->parentId);
    if (parent == nullptr) return -1;
    const auto id = node->id;
    return run(editor_->move(id, parent->parentId, parent->position + 1), id);
}

int BookmarkModel::setHidden(int row, bool hidden) {
    const auto* node = nodeAt(row);
    if (!editable() || node == nullptr) {
        emit refused(QStringLiteral("readOnly"));
        return -1;
    }
    const auto id = node->id;
    return run(editor_->setHidden(id, hidden), id);
}

void BookmarkModel::undo() {
    if (!editable() || !editor_) return;
    auto result = editor_->undo();
    const auto id = result.nodeId;
    run(std::move(result), id);
}

void BookmarkModel::redo() {
    if (!editable() || !editor_) return;
    auto result = editor_->redo();
    const auto id = result.nodeId;
    run(std::move(result), id);
}

} // namespace atlas::app
