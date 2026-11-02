#include "app/reader/ReaderViewport.h"

#include "app/reader/DocumentTextWorker.h"
#include "app/reader/PdfRenderWorker.h"
#include "app/reader/ReaderController.h"

#include <QKeyEvent>
#include <QQuickWindow>
#include <QRunnable>
#include <QSGNode>
#include <QSGTransformNode>
#include <QSGSimpleRectNode>
#include <QSGSimpleTextureNode>
#include <QSGTexture>
#include <QTimer>
#include <QWheelEvent>

#include <algorithm>
#include <cmath>

namespace atlas::app {

namespace {

using atlas::reader::PageGeometry;
using atlas::reader::PageRotation;
using atlas::reader::RenderPriority;
using atlas::reader::TextureKey;

constexpr std::size_t kTextureBudgetBytes = 256ULL * 1024ULL * 1024ULL;
constexpr qreal kLineStep = 56.0;
constexpr qreal kWheelZoomStep = 1.1;

class TextureCleanupJob final : public QRunnable {
public:
    explicit TextureCleanupJob(std::vector<QSGTexture*> textures)
        : textures_{std::move(textures)} {}
    void run() override {
        for (QSGTexture* texture : textures_) delete texture;
    }

private:
    std::vector<QSGTexture*> textures_;
};

} // namespace

ReaderViewport::ReaderViewport(QQuickItem* parent)
    : QQuickItem{parent}, cache_{kTextureBudgetBytes} {
    setFlag(ItemHasContents, true);
    setFlag(ItemIsFocusScope, true);
    setActiveFocusOnTab(true);
    setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton);
    setAcceptHoverEvents(true);
    setCursor(Qt::OpenHandCursor);
    setClip(true);
    connect(&autoScrollTimer_, &QTimer::timeout, this, [this] {
        const qreal before = contentY_;
        scrollBy(autoScrollSpeed_);
        if (qFuzzyCompare(1.0 + before, 1.0 + contentY_)) stopAutoScroll(); // reached the end
    });
    // Screen readers get the current page's text once the reader settles on a page.
    pageTextTimer_.setSingleShot(true);
    pageTextTimer_.setInterval(300);
    connect(&pageTextTimer_, &QTimer::timeout, this, &ReaderViewport::requestPageText);
    connect(this, &ReaderViewport::currentPageChanged, this, [this] { pageTextTimer_.start(); });
}

ReaderViewport::~ReaderViewport() {
    stopTextWorker();
    stopWorker();
    if (!textures_.empty()) {
        std::vector<QSGTexture*> textures;
        for (auto& [key, texture] : textures_) textures.push_back(texture);
        textures_.clear();
        if (QQuickWindow* w = window()) {
            w->scheduleRenderJob(new TextureCleanupJob(std::move(textures)),
                                 QQuickWindow::BeforeSynchronizingStage);
        }
    }
}

// ----------------------------------------------------------------- properties

void ReaderViewport::setController(ReaderController* controller) {
    if (controller_ == controller) return;
    if (controller_) disconnect(controller_, nullptr, this, nullptr);
    controller_ = controller;
    if (controller_) {
        const auto refresh = [this] { openDocument(); };
        connect(controller_, &QAbstractItemModel::dataChanged, this, refresh);
        connect(controller_, &QAbstractItemModel::modelReset, this, refresh);
    }
    emit controllerChanged();
    openDocument();
}

void ReaderViewport::setSessionId(qulonglong sessionId) {
    if (sessionId_ == sessionId) return;
    sessionId_ = sessionId;
    documentRevision_ = 0;
    emit sessionIdChanged();
    openDocument();
}

void ReaderViewport::setActive(bool active) {
    if (active_ == active) return;
    active_ = active;
    emit activeChanged();
    if (active_) {
        openDocument();
        scheduleRender();
    } else {
        // Hidden tabs keep their layout and position but give back texture memory.
        dropAllTextures();
    }
}

qreal ReaderViewport::contentWidth() const { return layout_.contentSize().width; }
qreal ReaderViewport::contentHeight() const { return layout_.contentSize().height; }

void ReaderViewport::setContentX(qreal value) {
    const qreal maxX = std::max<qreal>(0.0, contentWidth() - width());
    value = std::clamp<qreal>(std::isfinite(value) ? value : 0.0, 0.0, maxX);
    if (qFuzzyCompare(1.0 + contentX_, 1.0 + value)) return;
    contentX_ = value;
    emit scrollChanged();
    update();
}

void ReaderViewport::setContentY(qreal value) {
    const qreal maxY = std::max<qreal>(0.0, contentHeight() - height());
    value = std::clamp<qreal>(std::isfinite(value) ? value : 0.0, 0.0, maxY);
    if (singlePageMode() && !layout_.rows().empty()) {
        // One page (or spread) at a time: a jump selects the row, scrolling stays inside it.
        const auto row = layout_.pageAt(value + layout_.settings().pageGap).value_or(0);
        singleRow_ = layout_.rowOf(row);
        const auto [low, high] = rowScrollRange(singleRow_);
        value = std::clamp(value, low, high);
    }
    if (qFuzzyCompare(1.0 + contentY_, 1.0 + value)) return;
    contentY_ = value;
    emit scrollChanged();
    updateCurrentPage();
    scheduleRender();
    requestVisibleLinks();
    update();
}

void ReaderViewport::setZoom(qreal zoom) {
    if (fitMode_ != FitMode::None) {
        fitMode_ = FitMode::None;
        emit fitModeChanged();
    }
    zoomAround(zoom, QPointF{width() / 2.0, height() / 3.0});
}

void ReaderViewport::zoomAround(double zoom, QPointF anchor) {
    auto settings = layout_.settings();
    const double clamped = atlas::reader::clampZoom(zoom);
    if (std::abs(settings.zoom - clamped) < 1e-9) return;

    // Keep the document point under `anchor` stable across the zoom change.
    std::optional<std::size_t> page;
    double fraction = 0.0;
    if (layout_.pageCount() > 0) {
        page = layout_.pageAt(contentY_ + anchor.y());
        const auto before = layout_.pageRect(*page);
        fraction = before.height > 0 ? (contentY_ + anchor.y() - before.y) / before.height : 0.0;
        fraction = std::clamp(fraction, -1.0, 2.0); // margins/gaps scale with their page
    }
    const double widthBefore = std::max<qreal>(1.0, contentWidth());
    const double xFraction = (contentX_ + anchor.x() - horizontalInset()) / widthBefore;

    settings.zoom = clamped;
    layout_.setSettings(settings);
    if (page) {
        const auto after = layout_.pageRect(*page);
        contentY_ = after.y + fraction * after.height - anchor.y();
    }
    contentX_ = xFraction * contentWidth() - anchor.x() + horizontalInset();
    relayout();
    emit zoomChanged();
}

void ReaderViewport::setFitMode(FitMode mode) {
    if (fitMode_ == mode) return;
    const int page = currentPage_;
    fitMode_ = mode;
    emit fitModeChanged();
    applyFit();
    if (mode == FitMode::Page && page > 0) goToPage(page); // show the whole current page
}

void ReaderViewport::setPageColor(const QColor& color) {
    if (pageColor_ == color) return;
    pageColor_ = color;
    emit pageColorChanged();
    update();
}

void ReaderViewport::setPageShadowColor(const QColor& color) {
    if (pageShadowColor_ == color) return;
    pageShadowColor_ = color;
    emit pageColorChanged();
    update();
}

// ------------------------------------------------------------------- commands

void ReaderViewport::goToPage(int oneBasedPage) {
    if (layout_.pageCount() == 0) return;
    const auto index = static_cast<std::size_t>(
        std::clamp(oneBasedPage, 1, static_cast<int>(layout_.pageCount())) - 1);
    recordJump();
    jumpToIndex(index);
}

void ReaderViewport::jumpToIndex(std::size_t index) {
    setContentY(layout_.scrollOffsetForPage(index));
}

atlas::reader::ViewPosition ReaderViewport::currentPosition() const {
    if (layout_.pageCount() == 0) return {};
    const auto index = layout_.pageAt(contentY_).value_or(0);
    const auto rect = layout_.pageRect(index);
    const double fraction = rect.height > 0.0 ? std::clamp((contentY_ - rect.y) / rect.height, 0.0, 1.0) : 0.0;
    return {index, fraction};
}

void ReaderViewport::showPosition(const atlas::reader::ViewPosition& position) {
    if (layout_.pageCount() == 0) return;
    const auto index = std::min(position.page, layout_.pageCount() - 1);
    const auto rect = layout_.pageRect(index);
    setContentY(std::max(layout_.scrollOffsetForPage(index), rect.y + position.fraction * rect.height));
}

void ReaderViewport::recordJump() {
    if (layout_.pageCount() == 0) return;
    history_.recordJump(currentPosition());
    emit historyChanged();
}

void ReaderViewport::goBack() {
    if (const auto target = history_.back(currentPosition())) showPosition(*target);
    emit historyChanged();
}

void ReaderViewport::goForward() {
    if (const auto target = history_.forward(currentPosition())) showPosition(*target);
    emit historyChanged();
}

QSizeF ReaderViewport::pageSizePoints(int oneBasedPage) const {
    if (oneBasedPage < 1 || static_cast<std::size_t>(oneBasedPage) > layout_.pageCount()) return {};
    const auto index = static_cast<std::size_t>(oneBasedPage - 1);
    const auto rect = layout_.pageRect(index);
    const double scale = layout_.settings().zoom * atlas::reader::kPointsToLogicalPixels;
    return scale > 0.0 ? QSizeF{rect.width / scale, rect.height / scale} : QSizeF{};
}

void ReaderViewport::zoomBy(qreal factor) {
    if (!(factor > 0.0)) return;
    setZoom(zoom() * factor);
}

void ReaderViewport::resetZoom() { setZoom(1.0); }

void ReaderViewport::scrollByPages(qreal screens) {
    scrollBy(screens * std::max<qreal>(kLineStep, height() * 0.9));
}

bool ReaderViewport::singlePageMode() const {
    return !atlas::reader::isContinuous(layout_.settings().display);
}

std::pair<qreal, qreal> ReaderViewport::rowScrollRange(std::size_t row) const {
    const auto& r = layout_.rows().at(row);
    const qreal gap = layout_.settings().pageGap;
    const qreal low = std::max<qreal>(0.0, r.top - gap / 2.0);
    const qreal bottom = r.top + r.height + (row + 1 == layout_.rows().size() ? layout_.settings().margin : gap / 2.0);
    return {low, std::max(low, bottom - height())};
}

void ReaderViewport::scrollBy(qreal delta) {
    if (!singlePageMode() || layout_.rows().empty()) {
        setContentY(contentY_ + delta);
        return;
    }
    const auto [low, high] = rowScrollRange(singleRow_);
    const qreal target = contentY_ + delta;
    if (delta > 0 && contentY_ >= high - 0.5 && singleRow_ + 1 < layout_.rows().size()) {
        setContentY(rowScrollRange(singleRow_ + 1).first); // turn to the next page
    } else if (delta < 0 && contentY_ <= low + 0.5 && singleRow_ > 0) {
        setContentY(rowScrollRange(singleRow_ - 1).second); // back to the end of the previous page
    } else {
        setContentY(std::clamp(target, low, high));
    }
}

void ReaderViewport::changeLayout(const std::function<void(atlas::reader::LayoutSettings&)>& edit) {
    auto settings = layout_.settings();
    edit(settings);
    const int page = currentPage_;
    layout_.setSettings(settings);
    emit pageDisplayChanged();
    if (fitMode_ != FitMode::None) applyFit();
    relayout();
    if (page > 0) jumpToIndex(static_cast<std::size_t>(page - 1));
    update();
}

void ReaderViewport::setPageDisplay(int display) {
    display = std::clamp(display, 0, 3);
    if (display == pageDisplay()) return;
    changeLayout([display](auto& s) { s.display = static_cast<atlas::reader::PageDisplay>(display); });
}

void ReaderViewport::setCoverPage(bool cover) {
    if (cover == coverPage()) return;
    changeLayout([cover](auto& s) { s.coverPage = cover; });
}

void ReaderViewport::setRightToLeft(bool rightToLeft) {
    if (rightToLeft == this->rightToLeft()) return;
    changeLayout([rightToLeft](auto& s) { s.rightToLeft = rightToLeft; });
}

void ReaderViewport::nextPage() {
    if (layout_.pageCount() == 0 || currentPage_ < 1) return;
    // Move by row so facing spreads turn two pages at a time.
    const auto row = layout_.rowOf(static_cast<std::size_t>(currentPage_ - 1));
    if (row + 1 < layout_.rows().size()) jumpToIndex(layout_.rows()[row + 1].first);
}

void ReaderViewport::previousPage() {
    if (layout_.pageCount() == 0 || currentPage_ < 1) return;
    const auto row = layout_.rowOf(static_cast<std::size_t>(currentPage_ - 1));
    if (row > 0) jumpToIndex(layout_.rows()[row - 1].first);
    else jumpToIndex(0);
}

void ReaderViewport::firstPage() {
    recordJump();
    setContentY(0.0);
}

void ReaderViewport::lastPage() {
    if (layout_.pageCount() > 0) goToPage(static_cast<int>(layout_.pageCount()));
}

void ReaderViewport::actualSize() { setZoom(1.0); }

void ReaderViewport::rotateView(int quarterTurns) {
    auto settings = layout_.settings();
    const int turns = ((static_cast<int>(settings.viewRotation) + quarterTurns) % 4 + 4) % 4;
    settings.viewRotation = static_cast<atlas::reader::PageRotation>(turns);
    const int page = currentPage_;
    layout_.setSettings(settings);
    emit rotationChanged();
    if (fitMode_ != FitMode::None) applyFit();
    relayout();
    if (page > 0) goToPage(page);
}

void ReaderViewport::setReadingColors(int colors) {
    colors = std::clamp(colors, 0, 3);
    if (colors == readingColors_) return;
    readingColors_ = colors;
    emit readingColorsChanged();
    queueTextureRelease(cache_.clear()); // textures for the old colours are no longer useful
    scheduleRender();
    update();
}

void ReaderViewport::reload() {
    documentRevision_ = 0;
    openDocument();
}

// ------------------------------------------------------------------ document

void ReaderViewport::ensureWorker() {
    if (worker_) return;
    cancellation_ = std::make_shared<RenderCancellation>();
    worker_ = new PdfRenderWorker{cancellation_};
    worker_->moveToThread(&workerThread_);
    connect(&workerThread_, &QThread::finished, worker_, &QObject::deleteLater);
    connect(worker_, &PdfRenderWorker::opened, this, &ReaderViewport::onOpened);
    connect(worker_, &PdfRenderWorker::rendered, this, &ReaderViewport::onRendered);
    connect(worker_, &PdfRenderWorker::renderFailed, this, &ReaderViewport::onRenderFailed);
    connect(worker_, &PdfRenderWorker::documentBytes, this,
            [this](quint64 revision, const QByteArray& bytes, const QString& password) {
                if (revision != documentRevision_) return;
                ensureTextWorker();
                QMetaObject::invokeMethod(textWorker_, [worker = textWorker_, bytes, password, revision] {
                    worker->open(bytes, password, revision);
                }, Qt::QueuedConnection);
            });
    workerThread_.setObjectName(QStringLiteral("AtlasPageRenderer"));
    workerThread_.start(QThread::LowPriority);
}

void ReaderViewport::stopWorker() {
    if (!worker_) return;
    disconnect(worker_, nullptr, this, nullptr);
    QMetaObject::invokeMethod(worker_, &PdfRenderWorker::close, Qt::QueuedConnection);
    workerThread_.quit();
    workerThread_.wait();
    worker_ = nullptr; // deleted by QThread::finished -> deleteLater
}

void ReaderViewport::openDocument() {
    if (!controller_ || sessionId_ == 0 || !active_) return;
    const auto source = controller_->renderSource(sessionId_);
    if (!source) return; // not ready (loading, password, error): the status view handles it
    if (documentRevision_ == source->revision && (documentReady_ || loading_)) return;

    ensureWorker();
    outline_.clear();
    pageLabels_.clear();
    hasText_ = true;
    textReady_ = false;
    links_.clear();
    linksRequested_.clear();
    emit textInfoChanged();
    clearSearch();
    clearSelection();
    history_.clear();
    emit historyChanged();
    documentRevision_ = source->revision;
    documentReady_ = false;
    loading_ = true;
    errorKey_.clear();
    emit loadingChanged();
    cancellation_->clear();
    ticketKeys_.clear();
    scheduler_ = atlas::reader::RenderScheduler{2};
    queueTextureRelease(cache_.clear());
    QMetaObject::invokeMethod(worker_, "open", Qt::QueuedConnection,
                              Q_ARG(QString, source->path), Q_ARG(QString, source->password),
                              Q_ARG(quint64, source->revision));
}

void ReaderViewport::onOpened(quint64 revision, int error, const QList<QSizeF>& pageSizes) {
    if (revision != documentRevision_) return;
    loading_ = false;
    if (error != static_cast<int>(PdfRenderWorker::OpenError::none)) {
        documentReady_ = false;
        switch (static_cast<PdfRenderWorker::OpenError>(error)) {
        case PdfRenderWorker::OpenError::missing: errorKey_ = QStringLiteral("missing"); break;
        case PdfRenderWorker::OpenError::password: errorKey_ = QStringLiteral("password"); break;
        case PdfRenderWorker::OpenError::unsupportedSecurity:
            errorKey_ = QStringLiteral("unsupportedSecurity");
            break;
        case PdfRenderWorker::OpenError::malformed: errorKey_ = QStringLiteral("malformed"); break;
        default: errorKey_ = QStringLiteral("failed"); break;
        }
        emit loadingChanged();
        return;
    }

    std::vector<PageGeometry> pages;
    pages.reserve(static_cast<std::size_t>(pageSizes.size()));
    for (const QSizeF& size : pageSizes) {
        // Sizes already include each page's /Rotate, so the layout treats them
        // as upright; only the user view rotation is added on top.
        pages.push_back(PageGeometry{size.width(), size.height(), PageRotation::r0});
    }
    layout_ = atlas::reader::ViewportLayout{std::move(pages), layout_.settings()};
    documentReady_ = true;
    errorKey_.clear();
    contentY_ = 0.0;
    contentX_ = 0.0;
    emit loadingChanged();
    emit layoutChanged();
    applyFit();
    contentY_ = 0.0; // a freshly opened document starts at its first page
    contentX_ = 0.0;
    relayout();
}

void ReaderViewport::onRendered(quint64 ticket, quint64 revision, int page, const QImage& image) {
    Q_UNUSED(page);
    const auto keyIt = ticketKeys_.find(ticket);
    const bool current = revision == documentRevision_ && keyIt != ticketKeys_.end() &&
                         scheduler_.isCurrent(ticket);
    if (!current) {
        if (keyIt != ticketKeys_.end()) ticketKeys_.erase(keyIt);
        dispatch(scheduler_.finished(ticket));
        return;
    }
    const TextureKey key = keyIt->second;
    ticketKeys_.erase(keyIt);
    const auto bytes = static_cast<std::size_t>(image.sizeInBytes());
    queueTextureRelease(cache_.insert(key, bytes));
    uploads_[key] = image;
    dispatch(scheduler_.finished(ticket));
    update();
}

void ReaderViewport::onRenderFailed(quint64 ticket, quint64 revision, int page) {
    Q_UNUSED(revision);
    Q_UNUSED(page);
    ticketKeys_.erase(ticket);
    dispatch(scheduler_.finished(ticket));
}

// ------------------------------------------------------------------ scheduling

void ReaderViewport::scheduleRender() {
    if (scheduleQueued_) return;
    scheduleQueued_ = true;
    QTimer::singleShot(0, this, [this] {
        scheduleQueued_ = false;
        runSchedule();
    });
}

void ReaderViewport::runSchedule() {
    if (!documentReady_ || !active_ || !worker_ || height() <= 0.0) return;

    atlas::reader::ViewportState state;
    state.scrollTop = contentY_;
    state.viewportHeight = height();
    state.devicePixelRatio = devicePixelRatio();
    state.prefetchScreens = 1.0;
    state.documentRevision = documentRevision_;
    state.theme = static_cast<std::uint8_t>(readingColors_);
    const atlas::reader::RenderPlan plan = atlas::reader::planRenders(layout_, state);

    std::vector<TextureKey> visible;
    for (const auto& request : plan.requests) {
        if (request.priority == RenderPriority::visible) visible.push_back(request.key);
    }
    queueTextureRelease(cache_.pin(visible));

    const auto isCached = [this](const TextureKey& key) {
        return cache_.contains(key) || uploads_.contains(key);
    };
    dispatch(scheduler_.update(plan, isCached));
    update();
}

void ReaderViewport::dispatch(const atlas::reader::SchedulerActions& actions) {
    for (const auto ticket : actions.cancel) {
        cancellation_->cancel(ticket);
        ticketKeys_.erase(ticket);
    }
    for (const auto& [ticket, request] : actions.start) {
        ticketKeys_[ticket] = request.key;
        const QSize pixels{static_cast<int>(request.pixelSize.width),
                           static_cast<int>(request.pixelSize.height)};
        QMetaObject::invokeMethod(worker_, "render", Qt::QueuedConnection, Q_ARG(quint64, ticket),
                                  Q_ARG(quint64, request.key.documentRevision),
                                  Q_ARG(int, static_cast<int>(request.key.page)),
                                  Q_ARG(QSize, pixels),
                                  Q_ARG(int, static_cast<int>(layout_.settings().viewRotation)),
                                  Q_ARG(int, static_cast<int>(request.key.theme)));
    }
}

// -------------------------------------------------------------------- layout

void ReaderViewport::applyFit() {
    if (!documentReady_ || width() <= 0.0 || height() <= 0.0) return;
    auto settings = layout_.settings();
    double zoom = settings.zoom;
    if (fitMode_ == FitMode::Width) {
        zoom = layout_.fitWidthZoom(width());
    } else if (fitMode_ == FitMode::Page) {
        const auto reference = static_cast<std::size_t>(std::max(0, currentPage_ - 1));
        zoom = layout_.fitPageZoom({width(), height()}, reference);
    } else {
        return;
    }
    zoomAround(zoom, QPointF{width() / 2.0, 0.0});
}

void ReaderViewport::relayout() {
    emit layoutChanged();
    clampScroll();
    emit scrollChanged();
    updateCurrentPage();
    queueTextureRelease(cache_.enforceBudget());
    scheduleRender();
    update();
}

void ReaderViewport::clampScroll() {
    contentY_ = std::clamp<qreal>(contentY_, 0.0, std::max<qreal>(0.0, contentHeight() - height()));
    contentX_ = std::clamp<qreal>(contentX_, 0.0, std::max<qreal>(0.0, contentWidth() - width()));
}

void ReaderViewport::updateCurrentPage() {
    int page = 0;
    if (layout_.pageCount() > 0) {
        const auto index = layout_.pageAt(contentY_ + height() / 3.0);
        page = index ? static_cast<int>(*index) + 1 : 1;
    }
    if (page != currentPage_) {
        currentPage_ = page;
        emit currentPageChanged();
    }
}

double ReaderViewport::devicePixelRatio() const {
    if (const QQuickWindow* w = window()) return w->effectiveDevicePixelRatio();
    return 1.0;
}

double ReaderViewport::horizontalInset() const {
    return std::max<qreal>(0.0, (width() - contentWidth()) / 2.0);
}

void ReaderViewport::geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) {
    QQuickItem::geometryChange(newGeometry, oldGeometry);
    if (newGeometry.size() == oldGeometry.size()) return;
    if (fitMode_ != FitMode::None) {
        applyFit();
    }
    clampScroll();
    emit scrollChanged();
    updateCurrentPage();
    scheduleRender();
    update();
}

// --------------------------------------------------------------------- input

void ReaderViewport::wheelEvent(QWheelEvent* event) {
    if (!documentReady_) {
        event->ignore();
        return;
    }
    const QPoint pixels = event->pixelDelta();
    const QPoint degrees = event->angleDelta();
    if (event->modifiers().testFlag(Qt::ControlModifier)) {
        const double steps = degrees.y() / 120.0;
        if (steps != 0.0) {
            if (fitMode_ != FitMode::None) {
                fitMode_ = FitMode::None;
                emit fitModeChanged();
            }
            zoomAround(zoom() * std::pow(kWheelZoomStep, steps), event->position());
        }
        event->accept();
        return;
    }
    const qreal dy = !pixels.isNull() ? pixels.y() : degrees.y() / 120.0 * kLineStep * 1.5;
    const qreal dx = !pixels.isNull() ? pixels.x() : degrees.x() / 120.0 * kLineStep * 1.5;
    scrollBy(-dy);
    setContentX(contentX_ - dx);
    event->accept();
}

void ReaderViewport::keyPressEvent(QKeyEvent* event) {
    if (!documentReady_) {
        QQuickItem::keyPressEvent(event);
        return;
    }
    const bool ctrl = event->modifiers().testFlag(Qt::ControlModifier);
    const bool shift = event->modifiers().testFlag(Qt::ShiftModifier);
    // AutoScroll (Foxit): Up/Down change the speed, Esc or any click stops it.
    if (autoScrolling()) {
        switch (event->key()) {
        case Qt::Key_Down: changeAutoScrollSpeed(1); event->accept(); return;
        case Qt::Key_Up: changeAutoScrollSpeed(-1); event->accept(); return;
        case Qt::Key_Escape: stopAutoScroll(); event->accept(); return;
        default: break;
        }
    }
    switch (event->key()) {
    case Qt::Key_Down: scrollBy(kLineStep); break;
    case Qt::Key_Up: scrollBy(-kLineStep); break;
    case Qt::Key_C:
        if (!ctrl || selectedText().isEmpty()) return QQuickItem::keyPressEvent(event);
        copySelection();
        break;
    case Qt::Key_A:
        if (!ctrl) return QQuickItem::keyPressEvent(event);
        selectAllOnPage();
        break;
    case Qt::Key_Escape:
        if (selectedText().isEmpty()) return QQuickItem::keyPressEvent(event);
        clearSelection();
        break;
    case Qt::Key_Right:
    case Qt::Key_Left: {
        if (event->modifiers().testFlag(Qt::AltModifier)) { // Foxit Previous/Next View
            if (event->key() == Qt::Key_Left) goBack();
            else goForward();
            break;
        }
        // Scroll sideways when the page is wider than the view; otherwise turn
        // the page. Right-to-left book progression arrives with N4.4 spreads.
        // Reverse View (right-to-left books): Left turns forward.
        const bool forward = (event->key() == Qt::Key_Right) != rightToLeft();
        if (contentWidth() > width() + 1.0) {
            setContentX(contentX_ + (event->key() == Qt::Key_Right ? kLineStep : -kLineStep));
        } else if (forward) {
            nextPage();
        } else {
            previousPage();
        }
        break;
    }
    case Qt::Key_PageDown:
        if (ctrl) nextPage();
        else scrollByPages(1.0);
        break;
    case Qt::Key_PageUp:
        if (ctrl) previousPage();
        else scrollByPages(-1.0);
        break;
    case Qt::Key_Space: scrollByPages(shift ? -1.0 : 1.0); break;
    case Qt::Key_Home:
        recordJump();
        setContentY(0.0);
        break;
    case Qt::Key_End:
        recordJump();
        setContentY(contentHeight());
        break;
    case Qt::Key_Plus:
    case Qt::Key_Equal:
        if (!ctrl) return QQuickItem::keyPressEvent(event);
        if (shift) rotateView(1);
        else zoomBy(1.25);
        break;
    case Qt::Key_Minus:
    case Qt::Key_Underscore:
        if (!ctrl) return QQuickItem::keyPressEvent(event);
        if (shift) rotateView(-1);
        else zoomBy(0.8);
        break;
    case Qt::Key_1: // Foxit-compatible: Ctrl+1 actual size
        if (!ctrl) return QQuickItem::keyPressEvent(event);
        actualSize();
        break;
    case Qt::Key_2: // Ctrl+2 fit width
        if (!ctrl) return QQuickItem::keyPressEvent(event);
        setFitMode(FitMode::Width);
        break;
    case Qt::Key_0: // Ctrl+0 fit page
        if (!ctrl) return QQuickItem::keyPressEvent(event);
        setFitMode(FitMode::Page);
        break;
    case Qt::Key_3: // Ctrl+3 fit visible
        if (!ctrl) return QQuickItem::keyPressEvent(event);
        fitVisible();
        break;
    default: QQuickItem::keyPressEvent(event); return;
    }
    event->accept();
}


// ----------------------------------------------------------------- textures

void ReaderViewport::queueTextureRelease(const std::vector<TextureKey>& keys) {
    for (const auto& key : keys) {
        uploads_.erase(key);
        releases_.push_back(key);
    }
    if (!keys.empty()) update();
}

void ReaderViewport::dropAllTextures() {
    if (cancellation_) {
        for (const auto& entry : ticketKeys_) cancellation_->cancel(entry.first);
    }
    ticketKeys_.clear();
    scheduler_ = atlas::reader::RenderScheduler{2};
    (void)cache_.clear();
    uploads_.clear();
    releases_.clear();
    releaseAll_ = true;
    update();
}

void ReaderViewport::releaseResources() {
    std::vector<QSGTexture*> textures;
    for (auto& [key, texture] : textures_) textures.push_back(texture);
    textures_.clear();
    if (QQuickWindow* w = window(); w && !textures.empty()) {
        w->scheduleRenderJob(new TextureCleanupJob(std::move(textures)),
                             QQuickWindow::BeforeSynchronizingStage);
    }
    (void)cache_.clear();
    uploads_.clear();
    releases_.clear();
}

void ReaderViewport::itemChange(ItemChange change, const ItemChangeData& value) {
    QQuickItem::itemChange(change, value);
    if (change == ItemSceneChange) {
        disconnect(invalidatedConnection_);
    }
    if (change == ItemSceneChange && value.window) {
        invalidatedConnection_ = connect(value.window, &QQuickWindow::sceneGraphInvalidated, this,
                [this] {
                    // Render thread: the scene graph is gone, so are our textures.
                    for (auto& [key, texture] : textures_) delete texture;
                    textures_.clear();
                    QMetaObject::invokeMethod(
                        this,
                        [this] {
                            (void)cache_.clear();
                            uploads_.clear();
                            releases_.clear();
                            scheduleRender();
                        },
                        Qt::QueuedConnection);
                },
                Qt::DirectConnection);
    }
}

QSGNode* ReaderViewport::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*) {
    // A transform node root keeps every scene-graph backend (including the
    // software renderer) placing and clipping our children in item space.
    auto* root = oldNode ? static_cast<QSGTransformNode*>(oldNode) : new QSGTransformNode;
    // Children reference textures; remove them before any texture is released.
    while (QSGNode* child = root->firstChild()) {
        root->removeChildNode(child);
        delete child;
    }

    if (releaseAll_) {
        for (auto& [key, texture] : textures_) delete texture;
        textures_.clear();
        releaseAll_ = false;
    }
    for (const auto& key : releases_) {
        if (const auto it = textures_.find(key); it != textures_.end()) {
            delete it->second;
            textures_.erase(it);
        }
    }
    releases_.clear();

    QQuickWindow* w = window();
    if (w) {
        for (auto& [key, image] : uploads_) {
            if (auto existing = textures_.find(key); existing != textures_.end()) {
                delete existing->second;
                textures_.erase(existing);
            }
            if (QSGTexture* texture = w->createTextureFromImage(image)) {
                texture->setFiltering(QSGTexture::Linear);
                textures_.emplace(key, texture);
            }
        }
        uploads_.clear(); // no duplicate CPU copy once on the GPU
    }

    if (!documentReady_ || !active_ || layout_.pageCount() == 0) return root;
    auto band = layout_.pagesInBand(contentY_, height());
    if (!band) return root;
    if (singlePageMode() && singleRow_ < layout_.rows().size()) {
        // Single-page modes show only the current page or spread.
        const auto& row = layout_.rows()[singleRow_];
        band = atlas::reader::PageRange{row.first, row.last};
    }

    const double dpr = devicePixelRatio();
    const int wanted = atlas::reader::scaleBucketFor(layout_.settings().zoom *
                                                     atlas::reader::kPointsToLogicalPixels * dpr);
    const qreal dx = horizontalInset() - contentX_;
    const qreal dy = -contentY_;

    for (std::size_t i = band->first; i <= band->last; ++i) {
        const auto r = layout_.pageRect(i);
        const QRectF rect{std::round(r.x + dx), std::round(r.y + dy), std::round(r.width),
                          std::round(r.height)};

        auto* shadow = new QSGSimpleRectNode(rect.adjusted(-1, 0, 1, 2), pageShadowColor_);
        root->appendChildNode(shadow);
        auto* paper = new QSGSimpleRectNode(rect, pageColor_);
        root->appendChildNode(paper);

        const auto best = cache_.bestAvailable(documentRevision_, static_cast<std::uint32_t>(i),
                                               layout_.effectiveRotation(i),
                                               static_cast<std::uint8_t>(readingColors_), wanted);
        if (!best) continue;
        const auto texture = textures_.find(*best);
        if (texture == textures_.end()) continue;
        cache_.touch(*best);
        auto* node = new QSGSimpleTextureNode;
        node->setOwnsTexture(false);
        node->setTexture(texture->second);
        node->setFiltering(QSGTexture::Linear);
        node->setRect(rect);
        root->appendChildNode(node);
    }

    // Overlays: search hits (current hit stronger), text selection, rubber band.
    const QPointF shift{dx, dy};
    const auto addRect = [root](const QRectF& r, const QColor& color) {
        root->appendChildNode(new QSGSimpleRectNode(r, color));
    };
    for (int h = 0; h < searchHits_.size(); ++h) {
        const QVariantMap hit = searchHits_.at(h).toMap();
        const int page = hit.value(QStringLiteral("page")).toInt();
        if (page < static_cast<int>(band->first) || page > static_cast<int>(band->last)) continue;
        const QColor color = h == searchIndex_ ? QColor{255, 140, 0, 130} : QColor{255, 214, 0, 90};
        for (const QVariant& r : hit.value(QStringLiteral("rects")).toList()) {
            addRect(pointsToContent(static_cast<std::size_t>(page), r.toRectF()).translated(shift), color);
        }
    }
    if (selectionPage_ >= static_cast<int>(band->first) && selectionPage_ <= static_cast<int>(band->last)) {
        for (const QVariant& r : selectionRects_) {
            addRect(pointsToContent(static_cast<std::size_t>(selectionPage_), r.toRectF()).translated(shift),
                    QColor{11, 79, 174, 80}); // Atlas Blue at 30 %
        }
    }
    if (!rubberBand_.isEmpty()) {
        addRect(rubberBand_, QColor{11, 79, 174, 40});
        const QRectF b = rubberBand_;
        const QColor edge{11, 79, 174, 220};
        addRect({b.left(), b.top(), b.width(), 1}, edge);
        addRect({b.left(), b.bottom() - 1, b.width(), 1}, edge);
        addRect({b.left(), b.top(), 1, b.height()}, edge);
        addRect({b.right() - 1, b.top(), 1, b.height()}, edge);
    }
    return root;
}

} // namespace atlas::app
