// ReaderViewport, N4.5 part: text, outline, search, selection, links, tools
// (Hand, Select, SnapShot, Marquee zoom), Fit Visible, AutoScroll.
// All PDF text work runs on DocumentTextWorker's thread.

#include "app/reader/DocumentTextWorker.h"
#include "app/reader/ReaderController.h"
#include "app/reader/ReaderViewport.h"

#include <QClipboard>
#include <QCursor>
#include <QGuiApplication>
#include <QHoverEvent>
#include <QImage>
#include <QMouseEvent>
#include <QQuickWindow>

#include <algorithm>
#include <cmath>

namespace atlas::app {

namespace {
constexpr qreal kDragThreshold = 4.0;
}

// ------------------------------------------------------------ text worker

void ReaderViewport::ensureTextWorker() {
    if (textWorker_) return;
    latestSearch_ = std::make_shared<std::atomic<quint64>>(0);
    textWorker_ = new DocumentTextWorker{latestSearch_};
    textWorker_->moveToThread(&textThread_);
    connect(&textThread_, &QThread::finished, textWorker_, &QObject::deleteLater);
    connect(textWorker_, &DocumentTextWorker::info, this, &ReaderViewport::onTextInfo);
    connect(textWorker_, &DocumentTextWorker::searchBatch, this, &ReaderViewport::onSearchBatch);
    connect(textWorker_, &DocumentTextWorker::selected, this, &ReaderViewport::onSelected);
    connect(textWorker_, &DocumentTextWorker::pageLinks, this, &ReaderViewport::onPageLinks);
    connect(textWorker_, &DocumentTextWorker::wordCountReady, this,
            [this](quint64 id, int words, int characters, int pages) {
                if (id == wordCountRequest_) emit wordCountReady(words, characters, pages);
            });
    connect(textWorker_, &DocumentTextWorker::contentBoxReady, this,
            [this](quint64 id, int page, const QRectF& fraction) {
                if (id != contentBoxRequest_ || page < 0 || static_cast<std::size_t>(page) >= layout_.pageCount())
                    return;
                const QSizeF points = pageSizePoints(page + 1);
                const double contentWidthPoints = std::max(1.0, fraction.width() * points.width());
                const double available = width() - 2.0 * layout_.settings().pageGap;
                if (fitMode_ != FitMode::None) {
                    fitMode_ = FitMode::None;
                    emit fitModeChanged();
                }
                setZoom(available / (contentWidthPoints * atlas::reader::kPointsToLogicalPixels));
                const auto rect = layout_.pageRect(static_cast<std::size_t>(page));
                const QRectF content = pointsToContent(
                    static_cast<std::size_t>(page),
                    QRectF{fraction.x() * points.width(), fraction.y() * points.height(),
                           fraction.width() * points.width(), fraction.height() * points.height()});
                Q_UNUSED(rect);
                setContentX(content.x() - layout_.settings().pageGap);
            });
    textThread_.setObjectName(QStringLiteral("AtlasDocumentText"));
    textThread_.start(QThread::LowPriority);
}

void ReaderViewport::stopTextWorker() {
    if (!textWorker_) return;
    if (latestSearch_) latestSearch_->fetch_add(1); // stop a running search
    disconnect(textWorker_, nullptr, this, nullptr);
    textThread_.quit();
    textThread_.wait();
    textWorker_ = nullptr; // deleted by QThread::finished -> deleteLater
}

void ReaderViewport::onTextInfo(quint64 revision, const QVariantList& outline, const QStringList& labels,
                                bool hasText) {
    if (revision != documentRevision_) return;
    outline_ = outline;
    pageLabels_ = labels;
    hasText_ = hasText;
    textReady_ = true;
    emit textInfoChanged();
    requestVisibleLinks();
    requestPageText();
}

// Page-text requests use the top bit so they never collide with selection requests.
constexpr quint64 kPageTextBit = quint64{1} << 63;

void ReaderViewport::requestPageText() {
    if (!textWorker_ || !textReady_ || currentPage_ < 1) return;
    if (!hasText_) {
        if (!pageText_.isEmpty()) { pageText_.clear(); emit pageTextChanged(); }
        return;
    }
    const quint64 id = kPageTextBit | ++pageTextRequest_;
    const int page = currentPage_ - 1;
    QMetaObject::invokeMethod(textWorker_, [worker = textWorker_, id, page] { worker->selectPage(id, page); },
                              Qt::QueuedConnection);
}

// ---------------------------------------------------------------- search

void ReaderViewport::find(const QString& query, bool matchCase, bool wholeWords) {
    searchQuery_ = query.trimmed();
    searchHits_.clear();
    searchIndex_ = -1;
    searchProgress_ = 0.0;
    if (!textWorker_ || searchQuery_.isEmpty()) {
        searching_ = false;
        emit searchChanged();
        update();
        return;
    }
    const quint64 id = latestSearch_->fetch_add(1) + 1;
    searching_ = true;
    searchWantsFirstHit_ = true;
    emit searchChanged();
    const int fromPage = std::max(0, currentPage_ - 1);
    QMetaObject::invokeMethod(textWorker_, [worker = textWorker_, id, query = searchQuery_, matchCase, wholeWords, fromPage] {
        worker->search(id, query, matchCase, wholeWords, fromPage);
    }, Qt::QueuedConnection);
    update();
}

void ReaderViewport::onSearchBatch(quint64 searchId, const QVariantList& hits, int pagesDone, int pageCount,
                                   bool finished) {
    if (!latestSearch_ || searchId != latestSearch_->load()) return;
    searchHits_.append(hits);
    searchProgress_ = pageCount > 0 ? static_cast<qreal>(pagesDone) / pageCount : 1.0;
    if (finished) {
        searching_ = false;
        // Results arrive starting at the current page and wrap; keep reading order for the list.
        std::stable_sort(searchHits_.begin(), searchHits_.end(), [](const QVariant& a, const QVariant& b) {
            return a.toMap().value(QStringLiteral("page")).toInt() < b.toMap().value(QStringLiteral("page")).toInt();
        });
        if (searchIndex_ >= 0) {
            // Keep the shown hit selected after sorting.
            const int page = currentPage_ - 1;
            for (int i = 0; i < searchHits_.size(); ++i) {
                if (searchHits_.at(i).toMap().value(QStringLiteral("page")).toInt() >= page) {
                    searchIndex_ = i;
                    break;
                }
            }
        }
    }
    if (searchWantsFirstHit_ && !searchHits_.isEmpty()) {
        searchWantsFirstHit_ = false;
        showSearchHit(0);
        return;
    }
    emit searchChanged();
    update();
}

void ReaderViewport::showSearchHit(int index) {
    if (searchHits_.isEmpty()) return;
    index = ((index % searchHits_.size()) + searchHits_.size()) % searchHits_.size();
    searchIndex_ = index;
    const QVariantMap hit = searchHits_.at(index).toMap();
    const int page = hit.value(QStringLiteral("page")).toInt();
    const QVariantList rects = hit.value(QStringLiteral("rects")).toList();
    if (page >= 0 && static_cast<std::size_t>(page) < layout_.pageCount()) {
        recordJump();
        if (!rects.isEmpty()) {
            const QRectF area = pointsToContent(static_cast<std::size_t>(page), rects.first().toRectF());
            // Bring the hit into view with some context above it.
            if (area.top() < contentY_ || area.bottom() > contentY_ + height()) {
                setContentY(area.top() - height() / 3.0);
            }
            if (area.left() < contentX_ || area.right() > contentX_ + width()) {
                setContentX(area.left() - width() / 3.0);
            }
        } else {
            jumpToIndex(static_cast<std::size_t>(page));
        }
    }
    emit searchChanged();
    update();
}

void ReaderViewport::findNext() { showSearchHit(searchIndex_ + 1); }
void ReaderViewport::findPrevious() { showSearchHit(searchIndex_ - 1); }

void ReaderViewport::clearSearch() {
    if (latestSearch_) latestSearch_->fetch_add(1);
    searchQuery_.clear();
    searchHits_.clear();
    searchIndex_ = -1;
    searching_ = false;
    searchProgress_ = 0.0;
    emit searchChanged();
    update();
}

// ------------------------------------------------------------- selection

void ReaderViewport::onSelected(quint64 requestId, int page, const QString& text, const QVariantList& rects) {
    if ((requestId & kPageTextBit) != 0) {
        if (requestId != (kPageTextBit | pageTextRequest_)) return;
        const QString plain = text.normalized(QString::NormalizationForm_KC).simplified();
        if (plain != pageText_) { pageText_ = plain; emit pageTextChanged(); }
        return;
    }
    if (requestId != selectionRequest_) return;
    selectionPage_ = page;
    selectionRects_ = rects;
    selectedText_ = text;
    emit selectionChanged();
    update();
}

void ReaderViewport::copySelection() {
    if (selectedText_.isEmpty()) return;
    // Shaped Arabic glyphs and typographic ligatures copy as plain letters (NFKC).
    if (auto* clipboard = QGuiApplication::clipboard()) {
        clipboard->setText(selectedText_.normalized(QString::NormalizationForm_KC));
    }
}

void ReaderViewport::selectAllOnPage() {
    if (!textWorker_ || currentPage_ < 1) return;
    const quint64 id = ++selectionRequest_;
    const int page = currentPage_ - 1;
    QMetaObject::invokeMethod(textWorker_, [worker = textWorker_, id, page] { worker->selectPage(id, page); },
                              Qt::QueuedConnection);
}

void ReaderViewport::clearSelection() {
    ++selectionRequest_;
    if (selectedText_.isEmpty() && selectionRects_.isEmpty()) return;
    selectionPage_ = -1;
    selectionRects_.clear();
    selectedText_.clear();
    emit selectionChanged();
    update();
}

// ----------------------------------------------------------------- links

void ReaderViewport::requestVisibleLinks() {
    if (!textWorker_ || !textReady_ || layout_.pageCount() == 0) return;
    const auto band = layout_.pagesInBand(contentY_, height());
    if (!band) return;
    for (std::size_t i = band->first; i <= band->last; ++i) {
        const int page = static_cast<int>(i);
        if (linksRequested_[page]) continue;
        linksRequested_[page] = true;
        QMetaObject::invokeMethod(textWorker_, [worker = textWorker_, page] { worker->links(page); },
                                  Qt::QueuedConnection);
    }
}

void ReaderViewport::onPageLinks(quint64 revision, int page, const QVariantList& links) {
    if (revision != documentRevision_) return;
    links_[page] = links;
}

std::optional<QVariantMap> ReaderViewport::linkAt(QPointF itemPos) const {
    const QPointF content = toContent(itemPos);
    const auto page = pageAtContent(content);
    if (!page) return std::nullopt;
    const auto it = links_.find(static_cast<int>(*page));
    if (it == links_.end()) return std::nullopt;
    const QPointF points = contentToPoints(*page, content);
    for (const QVariant& value : it->second) {
        const QVariantMap link = value.toMap();
        if (link.value(QStringLiteral("rect")).toRectF().contains(points)) return link;
    }
    return std::nullopt;
}

void ReaderViewport::goToDestination(int zeroBasedPage, QPointF location) {
    if (layout_.pageCount() == 0) return;
    const auto page = static_cast<std::size_t>(std::clamp(zeroBasedPage, 0, static_cast<int>(layout_.pageCount()) - 1));
    recordJump();
    // Some files carry garbage or /Fit destinations: trust only a point on the page.
    const QSizeF pagePoints = pageSizePoints(static_cast<int>(page) + 1);
    const bool usable = std::isfinite(location.x()) && std::isfinite(location.y()) && location.y() > 0.0 &&
                        location.y() <= pagePoints.height() && location.x() >= 0.0 &&
                        location.x() <= pagePoints.width();
    if (usable) {
        const QRectF target = pointsToContent(page, QRectF{location, QSizeF{1, 1}});
        setContentY(target.top() - layout_.settings().pageGap);
    } else {
        jumpToIndex(page);
    }
}

void ReaderViewport::goToNormalized(int zeroBasedPage, qreal fx, qreal fy) {
    if (layout_.pageCount() == 0) return;
    const auto page = static_cast<std::size_t>(std::clamp(zeroBasedPage, 0, static_cast<int>(layout_.pageCount()) - 1));
    if (!std::isfinite(fx) || !std::isfinite(fy) || fy <= 0.0 || fy > 1.0) {
        recordJump();
        jumpToIndex(page);
        return;
    }
    const QSizeF size = pageSizePoints(static_cast<int>(page) + 1);
    const bool upright = static_cast<int>(layout_.settings().viewRotation) % 2 == 0;
    const double w = upright ? size.width() : size.height();
    const double h = upright ? size.height() : size.width();
    const QPointF location{std::clamp(fx, 0.0, 1.0) * w, fy * h};
    recordJump();
    const QRectF target = pointsToContent(page, QRectF{location, QSizeF{1, 1}});
    setContentY(target.top() - layout_.settings().pageGap);
}

QVariantMap ReaderViewport::viewLocation() const {
    QVariantMap result;
    if (layout_.pageCount() == 0 || currentPage_ < 1) return result;
    const auto page = static_cast<std::size_t>(currentPage_ - 1);
    const auto r = layout_.pageRect(page);
    const QSizeF size = pageSizePoints(currentPage_);
    const bool upright = static_cast<int>(layout_.settings().viewRotation) % 2 == 0;
    const double w = upright ? size.width() : size.height();
    const double h = upright ? size.height() : size.width();
    // The top of the view, or the top of the page when the view starts above it.
    const QPointF points = contentToPoints(page, QPointF{r.x, std::max<double>(contentY_, r.y)});
    const double fx = w > 0 ? std::clamp(points.x() / w, 0.0, 1.0) : 0.0;
    const double fy = h > 0 ? std::clamp(points.y() / h, 0.0, 1.0) : 0.0;
    result.insert(QStringLiteral("page"), static_cast<int>(page));
    result.insert(QStringLiteral("fx"), fx);
    result.insert(QStringLiteral("fy"), fy);
    result.insert(QStringLiteral("zoom"), layout_.settings().zoom);
    return result;
}

// ------------------------------------------------------------ geometry

QPointF ReaderViewport::toContent(QPointF itemPos) const {
    return {itemPos.x() - horizontalInset() + contentX_, itemPos.y() + contentY_};
}

std::optional<std::size_t> ReaderViewport::pageAtContent(QPointF content) const {
    if (layout_.pageCount() == 0) return std::nullopt;
    const auto band = layout_.pagesInBand(content.y() - 1.0, 2.0);
    if (!band) return std::nullopt;
    for (std::size_t i = band->first; i <= band->last; ++i) {
        const auto r = layout_.pageRect(i);
        if (content.x() >= r.x && content.x() <= r.right() && content.y() >= r.y && content.y() <= r.bottom()) return i;
    }
    return std::nullopt;
}

QRectF ReaderViewport::pointsToContent(std::size_t page, const QRectF& points) const {
    const auto r = layout_.pageRect(page);
    const QSizeF size = pageSizePoints(static_cast<int>(page) + 1); // upright (view rotation removed below)
    const double s = layout_.settings().zoom * atlas::reader::kPointsToLogicalPixels;
    const int turns = static_cast<int>(layout_.settings().viewRotation);
    // pageSizePoints() is already rotated by the view; recover the unrotated size.
    const double w = (turns % 2 == 0) ? size.width() : size.height();
    const double h = (turns % 2 == 0) ? size.height() : size.width();
    QRectF out;
    switch (turns) {
    case 1: out = QRectF{(h - points.bottom()) * s, points.left() * s, points.height() * s, points.width() * s}; break;
    case 2: out = QRectF{(w - points.right()) * s, (h - points.bottom()) * s, points.width() * s, points.height() * s}; break;
    case 3: out = QRectF{points.top() * s, (w - points.right()) * s, points.height() * s, points.width() * s}; break;
    default: out = QRectF{points.left() * s, points.top() * s, points.width() * s, points.height() * s}; break;
    }
    return out.translated(r.x, r.y);
}

QPointF ReaderViewport::contentToPoints(std::size_t page, QPointF content) const {
    const auto r = layout_.pageRect(page);
    const QSizeF size = pageSizePoints(static_cast<int>(page) + 1);
    const double s = layout_.settings().zoom * atlas::reader::kPointsToLogicalPixels;
    const int turns = static_cast<int>(layout_.settings().viewRotation);
    const double w = (turns % 2 == 0) ? size.width() : size.height();
    const double h = (turns % 2 == 0) ? size.height() : size.width();
    const double x = (content.x() - r.x) / s;
    const double y = (content.y() - r.y) / s;
    switch (turns) {
    case 1: return {y, h - x};
    case 2: return {w - x, h - y};
    case 3: return {w - y, x};
    default: return {x, y};
    }
}

// ----------------------------------------------------------------- tools

void ReaderViewport::setTool(int tool) {
    tool = std::clamp(tool, 0, 3);
    if (tool == tool_) return;
    tool_ = tool;
    setCursor(tool_ == SelectTool ? Qt::IBeamCursor : (tool_ == HandTool ? Qt::OpenHandCursor : Qt::CrossCursor));
    emit toolChanged();
}

void ReaderViewport::mousePressEvent(QMouseEvent* event) {
    forceActiveFocus(Qt::MouseFocusReason);
    if (autoScrolling()) { // any click stops AutoScroll (Foxit)
        stopAutoScroll();
        event->accept();
        return;
    }
    if (event->button() == Qt::RightButton) {
        emit contextMenuRequested(event->position().x(), event->position().y());
        event->accept();
        return;
    }
    pressed_ = true;
    dragged_ = false;
    pressPos_ = event->position();
    pressScroll_ = {contentX_, contentY_};
    pressPage_ = pageAtContent(toContent(pressPos_));
    if (tool_ == HandTool) setCursor(Qt::ClosedHandCursor);
    if (tool_ == SelectTool) clearSelection();
    event->accept();
}

void ReaderViewport::mouseMoveEvent(QMouseEvent* event) {
    if (!pressed_) return;
    const QPointF delta = event->position() - pressPos_;
    if (!dragged_ && std::hypot(delta.x(), delta.y()) < kDragThreshold) return;
    dragged_ = true;
    switch (tool_) {
    case HandTool:
        setContentX(pressScroll_.x() - delta.x());
        setContentY(pressScroll_.y() - delta.y());
        break;
    case SelectTool:
        if (pressPage_ && textWorker_) {
            const QPointF start = contentToPoints(*pressPage_, toContent(pressPos_));
            const QPointF end = contentToPoints(*pressPage_, toContent(event->position()));
            const quint64 id = ++selectionRequest_;
            const int page = static_cast<int>(*pressPage_);
            QMetaObject::invokeMethod(textWorker_, [worker = textWorker_, id, page, start, end] {
                worker->select(id, page, start, end);
            }, Qt::QueuedConnection);
        }
        break;
    default:
        rubberBand_ = QRectF{pressPos_, event->position()}.normalized();
        update();
        break;
    }
    event->accept();
}

void ReaderViewport::mouseReleaseEvent(QMouseEvent* event) {
    if (!pressed_) return;
    pressed_ = false;
    if (tool_ == HandTool) setCursor(Qt::OpenHandCursor);
    if (!dragged_) {
        // A click: follow a link (any tool), otherwise clear the selection.
        if (const auto link = linkAt(event->position())) {
            const QString url = link->value(QStringLiteral("url")).toString();
            const int page = link->value(QStringLiteral("page")).toInt();
            if (!url.isEmpty()) emit externalLinkRequested(url);
            else if (page >= 0) goToDestination(page, link->value(QStringLiteral("location")).toPointF());
        }
    } else if (tool_ == SnapshotTool && rubberBand_.width() > 4 && rubberBand_.height() > 4) {
        finishSnapshot(rubberBand_);
    } else if (tool_ == MarqueeTool && rubberBand_.width() > 4 && rubberBand_.height() > 4) {
        finishMarquee(rubberBand_);
    }
    rubberBand_ = {};
    update();
    event->accept();
}

void ReaderViewport::mouseDoubleClickEvent(QMouseEvent* event) {
    if (tool_ == SelectTool && textWorker_) {
        // Double-click selects the word under the pointer (tiny drag box).
        const auto page = pageAtContent(toContent(event->position()));
        if (page) {
            const QPointF at = contentToPoints(*page, toContent(event->position()));
            const quint64 id = ++selectionRequest_;
            const int p = static_cast<int>(*page);
            QMetaObject::invokeMethod(textWorker_, [worker = textWorker_, id, p, at] {
                worker->select(id, p, at - QPointF{2, 0}, at + QPointF{2, 0});
            }, Qt::QueuedConnection);
        }
    }
    event->accept();
}

void ReaderViewport::hoverMoveEvent(QHoverEvent* event) {
    const auto link = linkAt(event->position());
    const QString target = link ? (link->value(QStringLiteral("url")).toString().isEmpty()
                                       ? QStringLiteral("#%1").arg(link->value(QStringLiteral("page")).toInt() + 1)
                                       : link->value(QStringLiteral("url")).toString())
                                : QString{};
    if (target != hoveredLink_) {
        hoveredLink_ = target;
        emit hoveredLinkChanged();
        if (!target.isEmpty()) setCursor(Qt::PointingHandCursor);
        else setCursor(tool_ == SelectTool ? Qt::IBeamCursor : (tool_ == HandTool ? Qt::OpenHandCursor : Qt::CrossCursor));
    }
}

void ReaderViewport::finishSnapshot(const QRectF& itemRect) {
    QQuickWindow* w = window();
    if (!w) return;
    const QImage frame = w->grabWindow();
    if (frame.isNull()) return;
    const qreal dpr = frame.devicePixelRatio() > 0 ? frame.devicePixelRatio() : 1.0;
    const QPointF origin = mapToScene(itemRect.topLeft());
    const QRect crop{static_cast<int>(origin.x() * dpr), static_cast<int>(origin.y() * dpr),
                     static_cast<int>(itemRect.width() * dpr), static_cast<int>(itemRect.height() * dpr)};
    const QImage image = frame.copy(crop);
    if (auto* clipboard = QGuiApplication::clipboard()) clipboard->setImage(image);
    emit snapshotCopied();
}

void ReaderViewport::finishMarquee(const QRectF& itemRect) {
    const QPointF centre = toContent(itemRect.center());
    const double factor = std::min(width() / itemRect.width(), height() / itemRect.height());
    if (fitMode_ != FitMode::None) {
        fitMode_ = FitMode::None;
        emit fitModeChanged();
    }
    const double oldZoom = zoom();
    setZoom(oldZoom * factor);
    const double scale = zoom() / oldZoom;
    setContentX(centre.x() * scale - width() / 2.0);
    setContentY(centre.y() * scale - height() / 2.0);
}

// ------------------------------------------------------------ commands

void ReaderViewport::requestWordCount() {
    if (!textWorker_) return;
    const quint64 id = ++wordCountRequest_;
    QMetaObject::invokeMethod(textWorker_, [worker = textWorker_, id] { worker->wordCount(id); },
                              Qt::QueuedConnection);
}

void ReaderViewport::fitVisible() {
    if (!textWorker_ || currentPage_ < 1) return;
    const quint64 id = ++contentBoxRequest_;
    const int page = currentPage_ - 1;
    QMetaObject::invokeMethod(textWorker_, [worker = textWorker_, id, page] { worker->contentBox(id, page); },
                              Qt::QueuedConnection);
}

bool ReaderViewport::goToLabel(const QString& label) {
    const QString wanted = label.trimmed();
    if (wanted.isEmpty()) return false;
    bool numeric = false;
    const int number = wanted.toInt(&numeric);
    // A printed label wins over the physical number when both exist ("12" may be page 20).
    for (int i = 0; i < pageLabels_.size(); ++i) {
        if (pageLabels_.at(i).compare(wanted, Qt::CaseInsensitive) == 0 && pageLabels_.at(i) != QString::number(i + 1)) {
            goToPage(i + 1);
            return true;
        }
    }
    if (numeric && number >= 1 && number <= pageCount()) {
        goToPage(number);
        return true;
    }
    return false;
}

void ReaderViewport::toggleAutoScroll() {
    if (autoScrollTimer_.isActive()) {
        stopAutoScroll();
        return;
    }
    autoScrollTimer_.start(16);
    emit autoScrollChanged();
}

void ReaderViewport::changeAutoScrollSpeed(int steps) {
    autoScrollSpeed_ = std::clamp(autoScrollSpeed_ * std::pow(1.4, steps), 0.25, 40.0);
}

void ReaderViewport::stopAutoScroll() {
    if (!autoScrollTimer_.isActive()) return;
    autoScrollTimer_.stop();
    emit autoScrollChanged();
}

} // namespace atlas::app
