#pragma once

// Background page renderer for the reader viewport (N4.2).
//
// Lives on its own thread. The PDF is read once into memory and opened from a
// buffer, so the source file is never held open (N4.1 file-release guarantee)
// and the source bytes are never written. Qt PDF serialises engine calls
// internally; one worker per open tab keeps documents independent.

#include <QBuffer>
#include <QByteArray>
#include <QImage>
#include <QList>
#include <QObject>
#include <QSize>
#include <QSizeF>
#include <QString>

#include <memory>
#include <mutex>
#include <unordered_set>

class QPdfDocument;

namespace atlas::app {

// Tickets the GUI thread has cancelled; checked before each render starts.
class RenderCancellation final {
public:
    void cancel(quint64 ticket) {
        const std::scoped_lock lock{mutex_};
        cancelled_.insert(ticket);
    }
    [[nodiscard]] bool take(quint64 ticket) {
        const std::scoped_lock lock{mutex_};
        return cancelled_.erase(ticket) > 0;
    }
    void clear() {
        const std::scoped_lock lock{mutex_};
        cancelled_.clear();
    }

private:
    std::mutex mutex_;
    std::unordered_set<quint64> cancelled_;
};

class PdfRenderWorker final : public QObject {
    Q_OBJECT

public:
    enum class OpenError { none, missing, password, unsupportedSecurity, malformed, failed };
    Q_ENUM(OpenError)

    explicit PdfRenderWorker(std::shared_ptr<RenderCancellation> cancellation);
    ~PdfRenderWorker() override;

public slots:
    // `password` lives only in this process for the session; it is never logged
    // or persisted.
    void open(const QString& sourcePath, const QString& password, quint64 documentRevision);
    // `rotationQuarterTurns` is the user view rotation (0-3) applied on top of
    // the page's own /Rotate. `pixelSize` already reflects that rotation.
    // `readingColors`: 0 Paper (unchanged), 1 Sepia, 2 Night, 3 High contrast.
    // Applied to the rendered image only; the PDF is never modified.
    void render(quint64 ticket, quint64 documentRevision, int page, QSize pixelSize,
                int rotationQuarterTurns, int readingColors);
    void close();

signals:
    // Page sizes are in points with each page's /Rotate already applied.
    void opened(quint64 documentRevision, int error, const QList<QSizeF>& pageSizes);
    void rendered(quint64 ticket, quint64 documentRevision, int page, const QImage& image);
    void renderFailed(quint64 ticket, quint64 documentRevision, int page);

private:
    std::shared_ptr<RenderCancellation> cancellation_;
    QByteArray bytes_;
    QBuffer buffer_;
    QPdfDocument* document_{};
    quint64 revision_{};
};

} // namespace atlas::app
