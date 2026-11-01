#include "app/reader/DocumentTextWorker.h"

#include "core/reader/TextSearch.h"

#include <QEventLoop>
#include <QImage>
#include <QPdfBookmarkModel>
#include <QPdfDocument>
#include <QPdfDocumentRenderOptions>
#include <QPdfSelection>
#include <QPolygonF>
#include <QRectF>
#include <QSet>
#include <QUrl>
#include <QVariantMap>

#if __has_include(<QtPdf/QPdfLinkModel>)
#include <QtPdf/QPdfLinkModel>
#define ATLAS_HAS_LINK_MODEL 1
#endif

#include <algorithm>
#include <chrono>

namespace atlas::app {

namespace {

// Context snippets are for display: fold shaped Arabic to plain letters and
// drop control, private-use and replacement characters some PDFs extract.
QString displaySnippet(const QString& raw) {
    const QString folded = raw.normalized(QString::NormalizationForm_KC);
    QString out;
    out.reserve(folded.size());
    for (const QChar c : folded) {
        const auto category = c.category();
        if (category == QChar::Other_Control || category == QChar::Other_PrivateUse ||
            category == QChar::Other_NotAssigned || c == QChar::ReplacementCharacter) {
            continue;
        }
        out.append(c);
    }
    return out.simplified();
}

// Drops a trailing partial word from a context snippet.
QString trailingWords(const QString& raw) {
    QString tail = raw.simplified();
    if (raw.size() < 40) return tail; // reached the end of the page text
    const qsizetype space = tail.lastIndexOf(QLatin1Char(' '));
    if (space > 0) tail.truncate(space);
    return tail;
}

std::u16string toU16(const QString& text) {
    return std::u16string{reinterpret_cast<const char16_t*>(text.utf16()), static_cast<std::size_t>(text.size())};
}

QVariantList rectsOf(const QPdfSelection& selection) {
    QVariantList rects;
    for (const QPolygonF& polygon : selection.bounds()) rects.push_back(polygon.boundingRect());
    return rects;
}

void collectOutline(const QPdfBookmarkModel& model, const QModelIndex& parent, int depth, QVariantList& out) {
    const int rows = model.rowCount(parent);
    for (int row = 0; row < rows; ++row) {
        const QModelIndex index = model.index(row, 0, parent);
        QVariantMap entry;
        entry.insert(QStringLiteral("title"), model.data(index, static_cast<int>(QPdfBookmarkModel::Role::Title)));
        entry.insert(QStringLiteral("level"), depth);
        entry.insert(QStringLiteral("page"), model.data(index, static_cast<int>(QPdfBookmarkModel::Role::Page)));
        entry.insert(QStringLiteral("location"), model.data(index, static_cast<int>(QPdfBookmarkModel::Role::Location)));
        entry.insert(QStringLiteral("children"), model.rowCount(index));
        out.push_back(entry);
        if (depth < 32) collectOutline(model, index, depth + 1, out); // flat, depth-first
    }
}

} // namespace

DocumentTextWorker::DocumentTextWorker(std::shared_ptr<std::atomic<quint64>> latestSearch)
    : latestSearch_{std::move(latestSearch)} {}

DocumentTextWorker::~DocumentTextWorker() { close(); }

void DocumentTextWorker::close() {
    delete document_;
    document_ = nullptr;
    buffer_.close();
    bytes_.clear();
}

void DocumentTextWorker::open(const QByteArray& bytes, const QString& password, quint64 revision) {
    close();
    revision_ = revision;
    bytes_ = bytes; // shared with the renderer; read-only
    buffer_.setData(bytes_);
    if (!buffer_.open(QIODevice::ReadOnly)) return;
    document_ = new QPdfDocument{this};
    if (!password.isEmpty()) document_->setPassword(password);
    document_->load(&buffer_);
    if (document_->status() == QPdfDocument::Status::Loading) {
        QEventLoop loop;
        connect(document_, &QPdfDocument::statusChanged, &loop, [&loop](QPdfDocument::Status status) {
            if (status != QPdfDocument::Status::Loading) loop.quit();
        });
        if (document_->status() == QPdfDocument::Status::Loading) loop.exec();
    }
    if (document_->status() != QPdfDocument::Status::Ready) {
        close();
        return;
    }

    QVariantList outline;
    {
        QPdfBookmarkModel model;
        model.setDocument(document_);
        collectOutline(model, {}, 0, outline);
    }
    const int count = document_->pageCount();
    QStringList labels;
    labels.reserve(count);
    for (int page = 0; page < count; ++page) labels.push_back(document_->pageLabel(page));
    // A book is "image-only" when none of the first pages carries text.
    bool hasText = false;
    for (int page = 0; page < std::min(count, 12) && !hasText; ++page) {
        hasText = !document_->getAllText(page).text().trimmed().isEmpty();
    }
    emit info(revision_, outline, labels, hasText);
}

void DocumentTextWorker::search(quint64 searchId, const QString& query, bool matchCase, bool wholeWords,
                                int fromPage) {
    if (!document_ || query.trimmed().isEmpty()) {
        emit searchBatch(searchId, {}, 0, document_ ? document_->pageCount() : 0, true);
        return;
    }
    const atlas::reader::SearchOptions options{matchCase, wholeWords};
    const std::u16string needle = toU16(query.trimmed());
    const int count = document_->pageCount();
    QVariantList batch;
    auto lastEmit = std::chrono::steady_clock::now();
    for (int step = 0; step < count; ++step) {
        if (latestSearch_->load() != searchId) return; // superseded
        const int page = (std::clamp(fromPage, 0, std::max(0, count - 1)) + step) % count;
        const QString text = document_->getAllText(page).text();
        const std::u16string haystack = toU16(text);
        for (const auto& match : atlas::reader::findMatches(haystack, needle, options)) {
            const QPdfSelection hit = document_->getSelectionAtIndex(
                page, static_cast<int>(match.start), static_cast<int>(match.length));
            QVariantMap entry;
            entry.insert(QStringLiteral("page"), page);
            entry.insert(QStringLiteral("rects"), rectsOf(hit));
            const int before = std::max(0, static_cast<int>(match.start) - 40);
            QString lead = text.mid(before, static_cast<int>(match.start) - before).simplified();
            if (before > 0) { // start the context at a whole word
                const qsizetype space = lead.indexOf(QLatin1Char(' '));
                if (space >= 0) lead = lead.mid(space + 1);
            }
            entry.insert(QStringLiteral("before"), displaySnippet(lead));
            entry.insert(QStringLiteral("match"), displaySnippet(text.mid(static_cast<int>(match.start), static_cast<int>(match.length))));
            entry.insert(QStringLiteral("after"), displaySnippet(trailingWords(text.mid(static_cast<int>(match.start + match.length), 40))));
            batch.push_back(entry);
        }
        const auto now = std::chrono::steady_clock::now();
        if (now - lastEmit > std::chrono::milliseconds{120} || batch.size() > 200) {
            emit searchBatch(searchId, batch, step + 1, count, false);
            batch.clear();
            lastEmit = now;
        }
    }
    emit searchBatch(searchId, batch, count, count, true);
}

void DocumentTextWorker::select(quint64 requestId, int page, QPointF start, QPointF end) {
    if (!document_ || page < 0 || page >= document_->pageCount()) return;
    const QPdfSelection selection = document_->getSelection(page, start, end);
    emit selected(requestId, page, selection.text(), rectsOf(selection));
}

void DocumentTextWorker::selectPage(quint64 requestId, int page) {
    if (!document_ || page < 0 || page >= document_->pageCount()) return;
    const QPdfSelection selection = document_->getAllText(page);
    emit selected(requestId, page, selection.text(), rectsOf(selection));
}

void DocumentTextWorker::links(int page) {
    if (!document_ || page < 0 || page >= document_->pageCount()) return;
    QVariantList out;
#ifdef ATLAS_HAS_LINK_MODEL
    QPdfLinkModel model;
    model.setDocument(document_);
    model.setPage(page);
    QSet<QString> seen; // Qt PDF can report the same link twice (N2 finding)
    for (int row = 0; row < model.rowCount({}); ++row) {
        const QModelIndex index = model.index(row, 0);
        const QRectF rect = model.data(index, static_cast<int>(QPdfLinkModel::Role::Rectangle)).toRectF();
        const QUrl url = model.data(index, static_cast<int>(QPdfLinkModel::Role::Url)).toUrl();
        const int target = model.data(index, static_cast<int>(QPdfLinkModel::Role::Page)).toInt();
        const QPointF location = model.data(index, static_cast<int>(QPdfLinkModel::Role::Location)).toPointF();
        const QString key = QStringLiteral("%1,%2,%3,%4|%5|%6")
                                .arg(rect.x()).arg(rect.y()).arg(rect.width()).arg(rect.height())
                                .arg(url.toString()).arg(target);
        if (seen.contains(key)) continue;
        seen.insert(key);
        QVariantMap link;
        link.insert(QStringLiteral("rect"), rect);
        link.insert(QStringLiteral("url"), url.toString());
        link.insert(QStringLiteral("page"), url.isValid() && !url.isEmpty() ? -1 : target);
        link.insert(QStringLiteral("location"), location);
        out.push_back(link);
    }
#endif
    emit pageLinks(revision_, page, out);
}

void DocumentTextWorker::wordCount(quint64 requestId) {
    if (!document_) return;
    int words = 0;
    int characters = 0;
    const int count = document_->pageCount();
    for (int page = 0; page < count; ++page) {
        const QString text = document_->getAllText(page).text();
        words += static_cast<int>(atlas::reader::countWords(toU16(text)));
        for (const QChar c : text) {
            if (!c.isSpace()) ++characters;
        }
    }
    emit wordCountReady(requestId, words, characters, count);
}

void DocumentTextWorker::contentBox(quint64 requestId, int page) {
    if (!document_ || page < 0 || page >= document_->pageCount()) return;
    const QSizeF points = document_->pagePointSize(page);
    if (points.isEmpty()) return;
    const QSize size{400, std::max(1, static_cast<int>(400 * points.height() / points.width()))};
    QImage image = document_->render(page, size, {});
    image = image.convertToFormat(QImage::Format_ARGB32);
    int left = size.width(), right = -1, top = size.height(), bottom = -1;
    for (int y = 0; y < image.height(); ++y) {
        const auto* line = reinterpret_cast<const QRgb*>(image.constScanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            const QRgb px = line[x];
            if (qAlpha(px) > 16 && (qRed(px) < 235 || qGreen(px) < 235 || qBlue(px) < 235)) {
                left = std::min(left, x);
                right = std::max(right, x);
                top = std::min(top, y);
                bottom = std::max(bottom, y);
            }
        }
    }
    QRectF fraction{0, 0, 1, 1};
    if (right >= left && bottom >= top) {
        const double pad = 4.0;
        fraction = QRectF{std::max(0.0, (left - pad) / size.width()), std::max(0.0, (top - pad) / size.height()),
                          std::min(1.0, (right - left + 2 * pad) / size.width()),
                          std::min(1.0, (bottom - top + 2 * pad) / size.height())};
    }
    emit contentBoxReady(requestId, page, fraction);
}

} // namespace atlas::app
