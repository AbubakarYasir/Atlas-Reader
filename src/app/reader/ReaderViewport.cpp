#include "app/reader/ReaderViewport.h"

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
    setAcceptedMouseButtons(Qt::LeftButton);
    setClip(true);
}

ReaderViewport::~ReaderViewport() {
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
    if (qFuzzyCompare(1.0 + contentY_, 1.0 + value)) return;
    contentY_ = value;
    emit scrollChanged();
    updateCurrentPage();
    scheduleRender();
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
    setContentY(layout_.scrollOffsetForPage(index));
}

void ReaderViewport::zoomBy(qreal factor) {
    if (!(factor > 0.0)) return;
    setZoom(zoom() * factor);
}

void ReaderViewport::resetZoom() { setZoom(1.0); }

void ReaderViewport::scrollByPages(qreal screens) {
    setContentY(contentY_ + screens * std::max<qreal>(kLineStep, height() * 0.9));
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
                                  Q_ARG(int, static_cast<int>(layout_.settings().viewRotation)));
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
    setContentY(contentY_ - dy);
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
    switch (event->key()) {
    case Qt::Key_Down: setContentY(contentY_ + kLineStep); break;
    case Qt::Key_Up: setContentY(contentY_ - kLineStep); break;
    case Qt::Key_Right: setContentX(contentX_ + kLineStep); break;
    case Qt::Key_Left: setContentX(contentX_ - kLineStep); break;
    case Qt::Key_PageDown: scrollByPages(1.0); break;
    case Qt::Key_PageUp: scrollByPages(-1.0); break;
    case Qt::Key_Space: scrollByPages(shift ? -1.0 : 1.0); break;
    case Qt::Key_Home: setContentY(0.0); break;
    case Qt::Key_End: setContentY(contentHeight()); break;
    case Qt::Key_Plus:
    case Qt::Key_Equal:
        if (!ctrl) return QQuickItem::keyPressEvent(event);
        zoomBy(1.25);
        break;
    case Qt::Key_Minus:
        if (!ctrl) return QQuickItem::keyPressEvent(event);
        zoomBy(0.8);
        break;
    case Qt::Key_0:
        if (!ctrl) return QQuickItem::keyPressEvent(event);
        resetZoom();
        break;
    case Qt::Key_1:
        if (!ctrl) return QQuickItem::keyPressEvent(event);
        setFitMode(FitMode::Width);
        break;
    case Qt::Key_2:
        if (!ctrl) return QQuickItem::keyPressEvent(event);
        setFitMode(FitMode::Page);
        break;
    default: QQuickItem::keyPressEvent(event); return;
    }
    event->accept();
}

void ReaderViewport::mousePressEvent(QMouseEvent* event) {
    forceActiveFocus(Qt::MouseFocusReason);
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
    const auto band = layout_.pagesInBand(contentY_, height());
    if (!band) return root;

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
                                               layout_.effectiveRotation(i), 0, wanted);
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
    return root;
}

} // namespace atlas::app
