#pragma once

// Text, outline, links and search for one open document (N4.5), on its own
// background thread so long searches never delay page rendering or the UI.
// Opens the same in-memory bytes the renderer already read (implicitly shared,
// never written), so the file is never reopened or locked.

#include <QBuffer>
#include <QByteArray>
#include <QObject>
#include <QPointF>
#include <QString>
#include <QStringList>
#include <QVariantList>

#include <atomic>
#include <memory>

class QPdfDocument;

namespace atlas::app {

class DocumentTextWorker final : public QObject {
    Q_OBJECT

public:
    explicit DocumentTextWorker(std::shared_ptr<std::atomic<quint64>> latestSearch);
    ~DocumentTextWorker() override;

public slots:
    void open(const QByteArray& bytes, const QString& password, quint64 revision);
    void close();
    // Matches with Arabic/Urdu-aware normalization (core TextSearch). Results
    // stream in batches; a newer search id cancels this one between pages.
    void search(quint64 searchId, const QString& query, bool matchCase, bool wholeWords, int fromPage);
    void select(quint64 requestId, int page, QPointF start, QPointF end);
    void selectPage(quint64 requestId, int page);
    void links(int page);
    void wordCount(quint64 requestId);
    // Bounding box of the page's visible content as fractions (Fit Visible).
    void contentBox(quint64 requestId, int page);
    // N5.3 Auto Create Bookmarks: reads every page's text lines in short
    // slices (other text work runs in between) and suggests headings.
    void scanHeadings(quint64 scanId, int maxLevels);
    void cancelScan();

signals:
    void info(quint64 revision, const QVariantList& outline, const QStringList& pageLabels, bool hasText);
    void searchBatch(quint64 searchId, const QVariantList& hits, int pagesDone, int pageCount, bool finished);
    void selected(quint64 requestId, int page, const QString& text, const QVariantList& rects);
    void pageLinks(quint64 revision, int page, const QVariantList& links);
    void wordCountReady(quint64 requestId, int words, int characters, int pages);
    void contentBoxReady(quint64 requestId, int page, const QRectF& fraction);
    void headingsProgress(quint64 scanId, int pagesDone, int pageCount);
    // [{title, page, fx, fy, level, size}] in reading order.
    void headingsReady(quint64 scanId, const QVariantList& headings, double bodySize, bool truncated);

private:
    void scanSlice();

    struct HeadingScan;
    std::unique_ptr<HeadingScan> scan_;
    std::shared_ptr<std::atomic<quint64>> latestSearch_;
    QByteArray bytes_;
    QBuffer buffer_;
    QPdfDocument* document_{};
    quint64 revision_{};
};

} // namespace atlas::app
