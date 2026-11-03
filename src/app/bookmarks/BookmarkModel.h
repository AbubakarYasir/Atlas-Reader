#pragma once

// N5.3: one open book's bookmarks for the Bookmarks panel. Rows are the
// visible tree (expanded nodes, depth first) or, while filtering, the search
// hits. Commands run on the in-memory editor at once (< 2 ms on 10,000
// bookmarks) and are written by BookmarkService on its worker; a failed write
// reloads what is stored. Books the library does not know (or knows twice) show
// the PDF's own outline read-only: research is never attached to a guess.
// Qt Core only; presentation stays in QML.

#include "app/bookmarks/BookmarkService.h"
#include "core/bookmarks/BookmarkEditor.h"
#include "core/bookmarks/BookmarkSearch.h"

#include <QAbstractListModel>
#include <QPointer>
#include <QSet>
#include <QString>
#include <QVariantList>
#include <QVariantMap>

#include <memory>
#include <unordered_map>
#include <optional>
#include <string>
#include <vector>

namespace atlas::app {

// Not final: QML registration subclasses it.
class BookmarkModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(atlas::app::BookmarkService* service READ service WRITE setService NOTIFY serviceChanged)
    // "none", "loading", "editable", "readOnly"
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    // Why it is read-only: "notInLibrary", "ambiguous", "outlineUnsupported", "storageError", or "".
    Q_PROPERTY(QString statusReason READ statusReason NOTIFY statusChanged)
    // The PDF's outline differs from the one Atlas last read (reconciled in N5.4).
    Q_PROPERTY(bool outlineChangedOutside READ outlineChangedOutside NOTIFY statusChanged)
    Q_PROPERTY(bool editable READ editable NOTIFY statusChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY historyChanged)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY historyChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY historyChanged)
    Q_PROPERTY(QString undoKind READ undoKind NOTIFY historyChanged)
    Q_PROPERTY(QString redoKind READ redoKind NOTIFY historyChanged)
    Q_PROPERTY(int totalCount READ totalCount NOTIFY countsChanged)
    Q_PROPERTY(int hiddenCount READ hiddenCount NOTIFY countsChanged)
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)
    Q_PROPERTY(bool showHidden READ showHidden WRITE setShowHidden NOTIFY showHiddenChanged)
    // Something is expanded: the panel's one Expand/Collapse button collapses.
    Q_PROPERTY(bool anyExpanded READ anyExpanded NOTIFY countsChanged)
    // Zero-based page the reader shows; currentId is the bookmark whose destination
    // is the last one at or before the reading position (see setPosition).
    Q_PROPERTY(int currentPage READ currentPage WRITE setCurrentPage NOTIFY currentPageChanged)
    Q_PROPERTY(QString currentId READ currentId NOTIFY currentIdChanged)

public:
    enum Role {
        NodeIdRole = Qt::UserRole + 1,
        TitleRole,
        DepthRole,
        HasChildrenRole,
        ExpandedRole,
        StorageRole, // "embedded", "local_only", "pending_embed", "local_override", "locally_hidden", "conflict"
        PageRole,    // zero-based, -1 for a grouping-only bookmark
        FxRole,
        FyRole,
        BreadcrumbRole, // parent titles, while filtering
        MatchRole,      // "title", "notes", "tag", while filtering
        NotesRole,
        TagsRole,
        FromPdfRole, // the bookmark comes from the PDF's outline
    };
    Q_ENUM(Role)

    explicit BookmarkModel(QObject* parent = nullptr);
    ~BookmarkModel() override;

    [[nodiscard]] int rowCount(const QModelIndex& parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] BookmarkService* service() const { return service_; }
    void setService(BookmarkService* service);
    [[nodiscard]] QString status() const { return status_; }
    [[nodiscard]] QString statusReason() const { return statusReason_; }
    [[nodiscard]] bool outlineChangedOutside() const { return outlineChangedOutside_; }
    [[nodiscard]] bool editable() const { return status_ == QStringLiteral("editable"); }
    [[nodiscard]] bool busy() const;
    [[nodiscard]] bool canUndo() const;
    [[nodiscard]] bool canRedo() const;
    [[nodiscard]] QString undoKind() const;
    [[nodiscard]] QString redoKind() const;
    [[nodiscard]] int totalCount() const;
    [[nodiscard]] int hiddenCount() const { return hiddenCount_; }
    [[nodiscard]] QString filter() const { return filter_; }
    void setFilter(const QString& filter);
    [[nodiscard]] bool showHidden() const { return showHidden_; }
    [[nodiscard]] bool anyExpanded() const { return !expanded_.isEmpty(); }
    void setShowHidden(bool show);
    [[nodiscard]] int currentPage() const { return currentPage_; }
    void setCurrentPage(int page);
    [[nodiscard]] QString currentId() const { return currentId_; }

    // The book shown in this tab: its file and the outline the reader read.
    Q_INVOKABLE void attach(const QString& path, const QVariantList& outline);
    Q_INVOKABLE void detach();

    Q_INVOKABLE void toggle(int row);
    Q_INVOKABLE void setExpanded(int row, bool expanded);
    Q_INVOKABLE void expandAll();
    Q_INVOKABLE void collapseAll();
    Q_INVOKABLE int rowOf(const QString& id) const;
    // Every role of one row by name ({nodeId, title, page, …}); empty if out of range.
    Q_INVOKABLE QVariantMap get(int row) const;
    // Expands the ancestors so the bookmark has a row; returns it (or -1).
    Q_INVOKABLE int reveal(const QString& id);
    // The reading position: page (zero-based) and fraction down that page.
    Q_INVOKABLE void setPosition(int page, qreal fy);
    // N5.3 Command Center: matching bookmarks of this book, reading order,
    // [{nodeId, title, breadcrumb, page, fx, fy}], at most `limit`, hidden excluded.
    Q_INVOKABLE QVariantList findBookmarks(const QString& text, int limit = 8);

    // Commands return the affected row (or -1 when refused; see refused()).
    // location: {page, fx, fy, zoom} from ReaderViewport::viewLocation().
    // A new bookmark goes after `row` (as its sibling), inside it as the last
    // child when `asChild`, or at the end of the top level when row < 0.
    Q_INVOKABLE int addBookmark(int row, bool asChild, const QString& title, const QVariantMap& location);
    Q_INVOKABLE int rename(int row, const QString& title);
    Q_INVOKABLE int setDestination(int row, const QVariantMap& location);
    Q_INVOKABLE int setNotes(int row, const QString& notes, const QStringList& tags);
    Q_INVOKABLE int remove(int row);
    Q_INVOKABLE int moveUp(int row);
    Q_INVOKABLE int moveDown(int row);
    Q_INVOKABLE int indent(int row);  // into the previous sibling, as its last child
    Q_INVOKABLE int outdent(int row); // after its parent
    Q_INVOKABLE int setHidden(int row, bool hidden);
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();

signals:
    void serviceChanged();
    void statusChanged();
    void historyChanged();
    void countsChanged();
    void filterChanged();
    void showHiddenChanged();
    void currentPageChanged();
    void currentIdChanged();
    // A command was refused; `reason` is a stable name for an I18n message.
    void refused(const QString& reason);
    // A write failed and the panel now shows what is stored.
    void reloaded(const QString& reason);

private:
    struct Row final {
        std::string id;
        int depth{};
    };

    void setStatus(const QString& status, const QString& reason);
    void adoptSnapshot(std::vector<bookmarks::BookmarkNode> nodes, std::optional<bookmarks::BaseSnapshot> base,
                       std::int64_t revision);
    void rebuildRows();
    void rebuildSearch();
    void updateCurrent();
    void updateCounts();
    [[nodiscard]] const bookmarks::BookmarkNode* nodeAt(int row) const;
    [[nodiscard]] bool isVisibleNode(const bookmarks::BookmarkNode& node) const;
    int run(bookmarks::EditResult result, const std::string& focusId);
    void persist(const bookmarks::EditResult& result);
    void writeNext();
    void flushWrites();
    void recoverFromFailedWrite(const QString& reason);

    QPointer<BookmarkService> service_;
    std::unique_ptr<bookmarks::BookmarkEditor> editor_;
    std::unique_ptr<bookmarks::BookmarkSearchIndex> search_;
    std::vector<bookmarks::BookmarkHit> hits_;
    struct Spot final {
        int page{-1}; // zero-based, -1 for none
        double top{};
    };
    std::unordered_map<std::string, Spot> spotOf_; // where each bookmark goes
    bool searchStale_{true};
    std::vector<Row> rows_;
    QSet<QString> expanded_;
    std::string documentId_;
    QString path_;
    QString status_{QStringLiteral("none")};
    QString statusReason_;
    QString filter_;
    QString currentId_;
    bool outlineChangedOutside_{false};
    bool showHidden_{false};
    // Saves in order. Each change expects the revision the one before it makes
    // (the store adds exactly one per change), so editing never waits for disk.
    struct PendingWrite final {
        std::int64_t expectedRevision{};
        bookmarks::OverlayChangeSet change;
    };
    std::vector<PendingWrite> writes_;
    bool writing_{false};
    int currentPage_{-1};
    double currentFy_{0.0};
    int hiddenCount_{0};
    quint64 generation_{0}; // ignores answers for a book that is no longer attached
};

} // namespace atlas::app
