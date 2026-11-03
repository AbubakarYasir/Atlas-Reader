#pragma once

// N5.3: the one place that touches bookmark storage. A single worker thread
// owns the profile's library index (to resolve which book a file is) and the
// overlay repository; every request runs there in order, and every answer is
// delivered back on the thread that owns the service (the UI thread). QML never
// sees SQL, and nothing here blocks the UI.

#include "app/bookmarks/OutlineSafeSave.h"
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
        recoveryRequired,   // unresolved interrupted save; preserve local research
        recoveredReopen,    // original restored; reader must reload its PDF
    };
    Status status{Status::storageError};
    std::string documentId;
    bookmarks::BaseAdoption adoption{bookmarks::BaseAdoption::unchanged};
    bookmarks::OverlaySnapshot snapshot;      // when editable
    std::optional<bookmarks::BaseSnapshot> base; // the outline as read now (always, if valid)
    // N5.4 reconcile when the PDF's outline changed outside Atlas.
    struct Reconciled final {
        bool done{false};
        int added{}, updated{}, removed{}, conflicts{};
        std::string held; // "largeRemoval" (needs the reader's go-ahead) or "cannotMerge"
    } reconciled;
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
    // A PDF whose outline changed outside Atlas is reconciled here: Atlas
    // follows the PDF where the reader did nothing and marks conflicts. If
    // that would drop most of the PDF's bookmarks, it waits for
    // `followPdf` (the reader's go-ahead).
    void open(const QString& path, const QVariantList& outline, QObject* receiver,
              std::function<void(BookmarkOpenResult)> done, bool followPdf = false);
    void write(const std::string& documentId, std::int64_t expectedRevision, bookmarks::OverlayChangeSet change,
               QObject* receiver, std::function<void(BookmarkWriteResult)> done);
    void reload(const std::string& documentId, QObject* receiver,
                std::function<void(std::optional<bookmarks::OverlaySnapshot>)> done);

    // N5.4 safe save into the PDF (ADR-0009), on the worker; queued after any
    // pending writes. The password comes from the password source (the open
    // reader session), never from QML. Backups go to <profile>/backups.
    void save(SafeSaveInput input, QObject* receiver, std::function<void(SafeSaveOutcome)> done);
    // Save a Copy (N5.4): the same, into `target`; the source and overlay stay as they are.
    void saveCopy(SafeSaveInput input, QString target, QObject* receiver, std::function<void(SafeSaveOutcome)> done);
    void setPasswordSource(std::function<QString(const QString& path)> source) { passwordSource_ = std::move(source); }

    // Blocks until every queued request has finished (tests and shutdown).
    void waitForIdle();
    // CPU/file work serialized with bookmark storage, never on the UI thread.
    void exchangeWork(std::function<void()> work);

private:
    struct Storage;
    void post(std::function<void(Storage&)> job);

    QThread thread_;
    QObject* worker_{}; // lives on thread_
    std::function<QString(const QString& path)> passwordSource_;
    std::shared_ptr<Storage> storage_;
};

} // namespace atlas::app
