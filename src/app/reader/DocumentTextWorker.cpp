#include "app/reader/DocumentTextWorker.h"

#include "core/bookmarks/HeadingDetector.h"
#include "core/reader/TextSearch.h"

#include <QEventLoop>
#include <QImage>
#include "app/document/DocumentCapabilityCheck.h"
#include <QPdfBookmarkModel>
#include <QTemporaryDir>
#include <QFile>
#include <QProcess>
#include <QElapsedTimer>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
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
#include <cmath>
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

// Validate raw tree links before Qt constructs its eager bookmark model. Qt's
// model can loop indefinitely on a /Next cycle, before our traversal runs.
// qpdf only emits raw objects here: it is not asked to traverse outlines.
bool safeOutlineStructure(const QByteArray& bytes, const QString& password, QList<int>& rotations) {
    const QString executable = QpdfInspector::locate();
    if (executable.isEmpty() || bytes.isEmpty()) return false;
    QTemporaryDir directory;
    if (!directory.isValid()) return false;
    const QString path = directory.filePath(QStringLiteral("outline-input.pdf"));
    QFile input{path};
    if (!input.open(QIODevice::WriteOnly) || input.write(bytes) != bytes.size() || !input.flush()) return false;
    input.close();
    QStringList args{QStringLiteral("--json=2"), QStringLiteral("--json-key=qpdf"), QStringLiteral("--json-key=pages")};
    if (!password.isEmpty()) args << QStringLiteral("--password-file=-");
    args << QStringLiteral("--") << path;
    QProcess process;
    process.start(executable, args);
    if (!process.waitForStarted(5000)) return false;
    if (!password.isEmpty()) process.write(password.toUtf8() + '\n');
    process.closeWriteChannel();
    QByteArray output;
    QElapsedTimer timer;
    timer.start();
    do {
        process.waitForFinished(25);
        output += process.readAllStandardOutput();
        (void)process.readAllStandardError();
        if (output.size() > 128 * 1024 * 1024 || timer.elapsed() > 15000) {
            process.kill();
            process.waitForFinished(2000);
            return false;
        }
    } while (process.state() != QProcess::NotRunning);
    if (process.exitStatus() != QProcess::NormalExit || (process.exitCode() != 0 && process.exitCode() != 3)) return false;
    QJsonParseError error;
    const auto result = QJsonDocument::fromJson(output, &error).object();
    const auto json = result.value("qpdf").toArray();
    if (error.error != QJsonParseError::NoError || json.size() < 2) return false;
    const auto objects = json.at(1).toObject();
    const auto resolve = [&objects](const QJsonValue& ref) {
        return ref.isObject() ? ref.toObject() : objects.value("obj:" + ref.toString()).toObject().value("value").toObject();
    };
    const auto root = resolve(objects.value("trailer").toObject().value("value").toObject().value("/Root"));
    if (root.isEmpty()) return false;
    for (const auto& page : result.value("pages").toArray()) {
        auto dict = resolve(page.toObject().value("object"));
        QSet<QString> parents;
        for (int depth = 0; depth < 64 && !dict.contains("/Rotate"); ++depth) {
            const auto parent = dict.value("/Parent");
            if (!parent.isString() || parents.contains(parent.toString())) break;
            parents.insert(parent.toString());
            dict = resolve(parent);
        }
        const int rotate = ((dict.value("/Rotate").toInt() % 360) + 360) % 360;
        if (rotate % 90 != 0) return false;
        rotations.append(rotate);
    }
    const auto outlineRef = root.value("/Outlines");
    if (outlineRef.isNull() || outlineRef.isUndefined()) return true;
    const auto outline = resolve(outlineRef);
    if (outline.isEmpty()) return false;
    struct Link { QJsonValue ref; int depth; };
    QList<Link> pending{{outline.value("/First"), 0}};
    QSet<QString> visited;
    while (!pending.isEmpty()) {
        const auto link = pending.takeLast();
        if (link.ref.isNull() || link.ref.isUndefined()) continue;
        if (!link.ref.isString() || link.depth > 32 || visited.size() >= 100000 || visited.contains(link.ref.toString())) return false;
        visited.insert(link.ref.toString());
        const auto node = resolve(link.ref);
        if (node.isEmpty()) return false;
        pending.append({node.value("/Next"), link.depth});
        pending.append({node.value("/First"), link.depth + 1});
    }
    return true;
}

// Qt reports destination points before the target page's inherent /Rotate.
// QPdfDocument's size already includes that rotation; apply it exactly once.
QPointF rotatedDestination(const QPdfDocument& document, int page, QPointF point, const QList<int>& rotations) {
    if (page < 0 || page >= rotations.size()) return point;
    const QSizeF size = document.pagePointSize(page);
    switch (rotations[page]) {
    case 90: return {size.width() - point.y(), point.x()};
    case 180: return {size.width() - point.x(), size.height() - point.y()};
    case 270: return {point.y(), size.height() - point.x()};
    default: return point;
    }
}

// N5.3 also records, for the bookmark overlay: `key` (the entry's place in the
// outline tree, "0/3/1"), `position` among its siblings, and the destination
// as fractions of the unrotated page (`fx`, `fy`, `hasPoint`) plus `zoom`.
void collectOutline(const QPdfDocument& document, const QPdfBookmarkModel& model, const QModelIndex& parent,
                    int depth, const QString& parentKey, QVariantList& out, const QList<int>& rotations) {
    const int rows = model.rowCount(parent);
    for (int row = 0; row < rows; ++row) {
        const QModelIndex index = model.index(row, 0, parent);
        const QString key = parentKey.isEmpty() ? QString::number(row) : parentKey + u'/' + QString::number(row);
        const int page = model.data(index, static_cast<int>(QPdfBookmarkModel::Role::Page)).toInt();
        const QPointF location = rotatedDestination(document, page,
            model.data(index, static_cast<int>(QPdfBookmarkModel::Role::Location)).toPointF(), rotations);
        QVariantMap entry;
        entry.insert(QStringLiteral("title"), model.data(index, static_cast<int>(QPdfBookmarkModel::Role::Title)));
        entry.insert(QStringLiteral("level"), depth);
        entry.insert(QStringLiteral("page"), page);
        entry.insert(QStringLiteral("location"), location);
        entry.insert(QStringLiteral("children"), model.rowCount(index));
        entry.insert(QStringLiteral("key"), key);
        entry.insert(QStringLiteral("parentKey"), parentKey);
        entry.insert(QStringLiteral("position"), row);
        entry.insert(QStringLiteral("zoom"), model.data(index, static_cast<int>(QPdfBookmarkModel::Role::Zoom)));
        // Only a point that lies on the page is a usable destination.
        const QSizeF size = page >= 0 && page < document.pageCount() ? document.pagePointSize(page) : QSizeF{};
        const bool hasPoint = size.width() > 0 && size.height() > 0 && std::isfinite(location.x()) &&
                              std::isfinite(location.y()) && location.x() >= 0 && location.y() > 0 &&
                              location.x() <= size.width() && location.y() <= size.height();
        entry.insert(QStringLiteral("hasPoint"), hasPoint);
        entry.insert(QStringLiteral("fx"), hasPoint ? location.x() / size.width() : 0.0);
        entry.insert(QStringLiteral("fy"), hasPoint ? location.y() / size.height() : 0.0);
        out.push_back(entry);
        if (depth < 32) collectOutline(document, model, index, depth + 1, key, out, rotations); // flat, depth-first
    }
}

// One page's text lines with their boxes (points, y down). Fast path: the
// page's text boxes, grouped into rows, line up one-to-one with its non-empty
// text lines; otherwise each line is measured on its own.
void collectLines(QPdfDocument& document, int page, std::vector<atlas::bookmarks::TextLine>& out) {
    const QSizeF size = document.pagePointSize(page);
    if (size.isEmpty()) return;
    const QPdfSelection all = document.getAllText(page);
    const QString text = all.text();
    struct Span final { int start; int length; };
    std::vector<Span> spans;
    for (int i = 0, start = 0; i <= text.size(); ++i) {
        if (i == text.size() || text[i] == u'\n' || text[i] == u'\r') {
            const QString piece = text.mid(start, i - start);
            if (!piece.trimmed().isEmpty()) spans.push_back({start, i - start});
            start = i + 1;
        }
    }
    if (spans.empty()) return;
    std::vector<QRectF> rows;
    for (const QPolygonF& polygon : all.bounds()) {
        const QRectF r = polygon.boundingRect();
        if (r.isEmpty()) continue;
        if (!rows.empty()) {
            QRectF& last = rows.back();
            const double overlap = std::min(last.bottom(), r.bottom()) - std::max(last.top(), r.top());
            if (overlap > 0.5 * std::min(last.height(), r.height())) {
                last = last.united(r);
                continue;
            }
        }
        rows.push_back(r);
    }
    const bool aligned = rows.size() == spans.size();
    for (std::size_t i = 0; i < spans.size(); ++i) {
        QRectF box;
        if (aligned) {
            box = rows[i];
        } else {
            for (const QPolygonF& polygon : document.getSelectionAtIndex(page, spans[i].start, spans[i].length).bounds())
                box = box.united(polygon.boundingRect());
        }
        if (box.isEmpty()) continue;
        const QString line = displaySnippet(text.mid(spans[i].start, spans[i].length));
        if (line.isEmpty()) continue;
        atlas::bookmarks::TextLine entry;
        entry.page = page;
        entry.text = line.toStdString();
        entry.top = box.top();
        entry.left = box.left();
        entry.width = box.width();
        entry.height = box.height();
        entry.pageWidth = size.width();
        entry.pageHeight = size.height();
        out.push_back(std::move(entry));
    }
}

} // namespace

struct DocumentTextWorker::HeadingScan final {
    quint64 id{};
    int maxLevels{3};
    int nextPage{};
    std::vector<atlas::bookmarks::TextLine> lines;
    std::chrono::steady_clock::time_point lastProgress{};
};

void DocumentTextWorker::scanHeadings(quint64 scanId, int maxLevels) {
    if (!document_) {
        emit headingsReady(scanId, {}, 0, false);
        return;
    }
    scan_ = std::make_unique<HeadingScan>();
    scan_->id = scanId;
    scan_->maxLevels = maxLevels;
    scanSlice();
}

void DocumentTextWorker::cancelScan() { scan_.reset(); }

void DocumentTextWorker::scanSlice() {
    if (!scan_ || !document_) return;
    const int count = document_->pageCount();
    const auto sliceStart = std::chrono::steady_clock::now();
    while (scan_->nextPage < count && std::chrono::steady_clock::now() - sliceStart < std::chrono::milliseconds{40}) {
        collectLines(*document_, scan_->nextPage++, scan_->lines);
    }
    const auto now = std::chrono::steady_clock::now();
    if (scan_->nextPage < count) {
        if (now - scan_->lastProgress > std::chrono::milliseconds{100}) {
            scan_->lastProgress = now;
            emit headingsProgress(scan_->id, scan_->nextPage, count);
        }
        // Let queued selection, link and search requests run, then go on.
        QMetaObject::invokeMethod(this, &DocumentTextWorker::scanSlice, Qt::QueuedConnection);
        return;
    }
    atlas::bookmarks::HeadingOptions options;
    options.maxLevels = scan_->maxLevels;
    const auto result = atlas::bookmarks::detectHeadings(scan_->lines, options);
    QVariantList headings;
    headings.reserve(static_cast<qsizetype>(result.headings.size()));
    for (const auto& heading : result.headings) {
        QVariantMap entry;
        entry.insert(QStringLiteral("title"), QString::fromStdString(heading.title));
        entry.insert(QStringLiteral("page"), heading.page);
        entry.insert(QStringLiteral("fx"), heading.fx);
        entry.insert(QStringLiteral("fy"), heading.fy);
        entry.insert(QStringLiteral("level"), heading.level);
        entry.insert(QStringLiteral("size"), heading.size);
        headings.push_back(entry);
    }
    const auto id = scan_->id;
    scan_.reset();
    emit headingsProgress(id, count, count);
    emit headingsReady(id, headings, result.bodySize, result.truncated);
}

QVariantList readOutline(const QPdfDocument& document, const QByteArray& sourceBytes, const QString& password, QList<int>* pageRotations) {
    QList<int> rotations;
    const bool safe = safeOutlineStructure(sourceBytes, password, rotations);
    if (pageRotations) *pageRotations = rotations;
    if (!safe || rotations.size() != document.pageCount())
        return {QVariantMap{{QStringLiteral("outlineUnsupported"), true}}};
    QVariantList outline;
    QPdfBookmarkModel model;
    model.setDocument(const_cast<QPdfDocument*>(&document));
    collectOutline(document, model, {}, 0, {}, outline, rotations);
    return outline;
}

DocumentTextWorker::DocumentTextWorker(std::shared_ptr<std::atomic<quint64>> latestSearch)
    : latestSearch_{std::move(latestSearch)} {}

DocumentTextWorker::~DocumentTextWorker() { close(); }

void DocumentTextWorker::close() {
    scan_.reset();
    delete document_;
    document_ = nullptr;
    buffer_.close();
    bytes_.clear();
    pageRotations_.clear();
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

    const QVariantList outline = readOutline(*document_, bytes_, password, &pageRotations_);
    const int count = document_->pageCount();
    QStringList labels;
    labels.reserve(count);
    // Newer Qt returns an empty label when the PDF defines none; use the page number then.
    for (int page = 0; page < count; ++page) {
        const QString label = document_->pageLabel(page).trimmed();
        labels.push_back(label.isEmpty() ? QString::number(page + 1) : label);
    }
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
        const QPointF location = rotatedDestination(*document_, target,
            model.data(index, static_cast<int>(QPdfLinkModel::Role::Location)).toPointF(), pageRotations_);
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
