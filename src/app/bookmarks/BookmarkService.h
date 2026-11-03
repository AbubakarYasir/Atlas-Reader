#pragma once

// N5.3: the one place that touches bookmark storage. A single worker thread
// owns the profile's library index (to resolve which book a file is) and the
// overlay repository; every request runs there in order, and every answer is
// delivered back on the thread that owns the service (the UI thread). QML never
// sees SQL, and nothing here blocks the UI.

#include "app/bookmarks/SqliteBookmarkOverlay.h"
#include "core/bookmarks/BookmarkOverlay.h"

#include <QObject>
#include <QPointer>
#include <QString>
#include <QThread>
#include <QVariantList>

#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <string>

namespace atlas::app {

struct BookmarkOpenResult final {
    enum class Status : std::uint8_t {
        editable,           // the book is in the library; its overlay is loaded
        notInLibrary,       // no library document at this path: read-only outline
        ambiguous,          // more than one library document at this path: read-only
        outlineUnsupported, // the PDF's outline breaks overlay limits: read-only
        storageError,       // the profile could not be read: read-only
    };
    Status status{Status::storageError};
    std::string documentId;
    bookmarks::BaseAdoption adoption{bookmarks::BaseAdoption::unchanged};
    bookmarks::OverlaySnapshot snapshot;      // when editable
    std::optional<bookmarks::BaseSnapshot> base; // the outline as read now (always, if valid)
};

struct BookmarkWriteResult final {
    bool ok{false};
    std::int64_t revision{};
    std::string reason; // rule name, "revisionConflict" or "storageError"
};

// The PDF's outline (DocumentTextWorker entries) as an overlay base snapshot.
// Destinations become versioned location envelopes; entries without a usable
// point go to the top of their page. Returns nullopt if the outline breaks the
// overlay's limits.
[[nodiscard]] std::optional<bookmarks::BaseSnapshot> baseFromOutline(const QVariantList& outline,
                                                                    const std::string& sourceRevision);

class BookmarkService final : public QObject {
    Q_OBJECT

public:
    explicit BookmarkService(std::filesystem::path databasePath, QObject* parent = nullptr);
    ~BookmarkService() override;

    BookmarkService(const BookmarkService&) = delete;
    BookmarkService& operator=(const BookmarkService&) = delete;

    // Each callback runs on this object's thread, and only while `receiver` exists.
    void open(const QString& path, const QVariantList& outline, QObject* receiver,
              std::function<void(BookmarkOpenResult)> done);
    void write(const std::string& documentId, std::int64_t expectedRevision, bookmarks::OverlayChangeSet change,
               QObject* receiver, std::function<void(BookmarkWriteResult)> done);
    void reload(const std::string& documentId, QObject* receiver,
                std::function<void(std::optional<bookmarks::OverlaySnapshot>)> done);

    // Blocks until every queued request has finished (tests and shutdown).
    void waitForIdle();

private:
    struct Storage;
    void post(std::function<void(Storage&)> job);

    QThread thread_;
    QObject* worker_{}; // lives on thread_
    std::shared_ptr<Storage> storage_;
};

} // namespace atlas::app
