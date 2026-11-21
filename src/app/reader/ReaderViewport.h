#pragma once

// Atlas-owned virtualized page viewport (N4.2).
//
// The pure-C++ model (`ViewportLayout`, `planRenders`, `RenderScheduler`,
// `PageTextureCache`) decides geometry, render order, cancellation and memory;
// this item only executes those decisions with Qt PDF on a worker thread and
// draws the resulting textures in the Qt Quick scene graph. No PDF work runs on
// the GUI or render thread.

#include "core/reader/RenderPlanner.h"
#include "core/reader/ViewHistory.h"
#include "core/reader/ViewportLayout.h"

#include <QColor>
#include <QImage>
#include <QPointer>
#include <QQuickItem>
#include <QSizeF>
#include <QThread>
#include <QTimer>
#include <QVariantList>
#include <QVariantMap>
#include <QStringList>

#include <atomic>
#include <functional>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

class QSGTexture;

namespace atlas::app {

class DocumentTextWorker;
class PdfRenderWorker;
class ReaderController;
class RenderCancellation;

class ReaderViewport : public QQuickItem {
    Q_OBJECT
    Q_PROPERTY(atlas::app::ReaderController* controller READ controller WRITE setController NOTIFY
                   controllerChanged)
    Q_PROPERTY(qulonglong sessionId READ sessionId WRITE setSessionId NOTIFY sessionIdChanged)
    Q_PROPERTY(bool active READ active WRITE setActive NOTIFY activeChanged)
    Q_PROPERTY(qreal contentX READ contentX WRITE setContentX NOTIFY scrollChanged)
    Q_PROPERTY(qreal contentY READ contentY WRITE setContentY NOTIFY scrollChanged)
    Q_PROPERTY(qreal contentWidth READ contentWidth NOTIFY layoutChanged)
    Q_PROPERTY(qreal contentHeight READ contentHeight NOTIFY layoutChanged)
    Q_PROPERTY(qreal zoom READ zoom WRITE setZoom NOTIFY zoomChanged)
    Q_PROPERTY(FitMode fitMode READ fitMode WRITE setFitMode NOTIFY fitModeChanged)
    Q_PROPERTY(int currentPage READ currentPage NOTIFY currentPageChanged)
    Q_PROPERTY(int pageCount READ pageCount NOTIFY layoutChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY loadingChanged)
    Q_PROPERTY(QString errorKey READ errorKey NOTIFY loadingChanged)
    Q_PROPERTY(int readingColors READ readingColors WRITE setReadingColors NOTIFY readingColorsChanged)
    Q_PROPERTY(int rotation READ rotation NOTIFY rotationChanged)
    Q_PROPERTY(int pageDisplay READ pageDisplay WRITE setPageDisplay NOTIFY pageDisplayChanged)
    Q_PROPERTY(bool coverPage READ coverPage WRITE setCoverPage NOTIFY pageDisplayChanged)
    Q_PROPERTY(bool rightToLeft READ rightToLeft WRITE setRightToLeft NOTIFY pageDisplayChanged)
    // N4.5: text, outline, search, selection, links and tools.
    Q_PROPERTY(QVariantList outline READ outline NOTIFY textInfoChanged)
    Q_PROPERTY(QStringList pageLabels READ pageLabels NOTIFY textInfoChanged)
    Q_PROPERTY(bool hasText READ hasText NOTIFY textInfoChanged)
    Q_PROPERTY(bool textReady READ textReady NOTIFY textInfoChanged)
    Q_PROPERTY(QVariantList searchHits READ searchHits NOTIFY searchChanged)
    Q_PROPERTY(int searchIndex READ searchIndex NOTIFY searchChanged)
    Q_PROPERTY(bool searching READ searching NOTIFY searchChanged)
    Q_PROPERTY(qreal searchProgress READ searchProgress NOTIFY searchChanged)
    Q_PROPERTY(QString searchQuery READ searchQuery NOTIFY searchChanged)
    Q_PROPERTY(QString selectedText READ selectedText NOTIFY selectionChanged)
    Q_PROPERTY(int tool READ tool WRITE setTool NOTIFY toolChanged)
    Q_PROPERTY(bool autoScrolling READ autoScrolling NOTIFY autoScrollChanged)
    Q_PROPERTY(QString hoveredLink READ hoveredLink NOTIFY hoveredLinkChanged)
    // Text of the current page for screen readers (C04); empty for image-only pages.
    Q_PROPERTY(QString pageText READ pageText NOTIFY pageTextChanged)
    Q_PROPERTY(bool canGoBack READ canGoBack NOTIFY historyChanged)
    Q_PROPERTY(bool canGoForward READ canGoForward NOTIFY historyChanged)
    Q_PROPERTY(QColor pageColor READ pageColor WRITE setPageColor NOTIFY pageColorChanged)
    Q_PROPERTY(QColor pageShadowColor READ pageShadowColor WRITE setPageShadowColor NOTIFY
                   pageColorChanged)

public:
    enum class FitMode { None, Width, Page };
    Q_ENUM(FitMode)

    explicit ReaderViewport(QQuickItem* parent = nullptr);
    ~ReaderViewport() override;

    [[nodiscard]] ReaderController* controller() const { return controller_; }
    void setController(ReaderController* controller);
    [[nodiscard]] qulonglong sessionId() const { return sessionId_; }
    void setSessionId(qulonglong sessionId);
    [[nodiscard]] bool active() const { return active_; }
    void setActive(bool active);

    [[nodiscard]] qreal contentX() const { return contentX_; }
    void setContentX(qreal value);
    [[nodiscard]] qreal contentY() const { return contentY_; }
    void setContentY(qreal value);
    [[nodiscard]] qreal contentWidth() const;
    [[nodiscard]] qreal contentHeight() const;
    [[nodiscard]] qreal zoom() const { return layout_.settings().zoom; }
    void setZoom(qreal zoom);
    [[nodiscard]] FitMode fitMode() const { return fitMode_; }
    void setFitMode(FitMode mode);
    [[nodiscard]] int currentPage() const { return currentPage_; }
    [[nodiscard]] int pageCount() const { return static_cast<int>(layout_.pageCount()); }
    [[nodiscard]] bool loading() const { return loading_; }
    [[nodiscard]] QString errorKey() const { return errorKey_; }
    [[nodiscard]] QColor pageColor() const { return pageColor_; }
    void setPageColor(const QColor& color);
    [[nodiscard]] QColor pageShadowColor() const { return pageShadowColor_; }
    void setPageShadowColor(const QColor& color);

    Q_INVOKABLE void goToPage(int oneBasedPage);
    Q_INVOKABLE void zoomBy(qreal factor);
    Q_INVOKABLE void resetZoom();
    Q_INVOKABLE void scrollByPages(qreal screens);
    Q_INVOKABLE void reload();
    Q_INVOKABLE void nextPage();
    Q_INVOKABLE void previousPage();
    Q_INVOKABLE void firstPage();
    Q_INVOKABLE void lastPage();
    Q_INVOKABLE void actualSize();
    Q_INVOKABLE void rotateView(int quarterTurns);
    // Foxit Previous View / Next View (Alt+Left / Alt+Right).
    // Foxit Page Display: 0 Single Page, 1 Continuous, 2 Facing, 3 Continuous Facing.
    [[nodiscard]] int pageDisplay() const { return static_cast<int>(layout_.settings().display); }
    void setPageDisplay(int display);
    [[nodiscard]] bool coverPage() const { return layout_.settings().coverPage; }
    void setCoverPage(bool cover);
    // Reverse View: spreads read right to left (independent of the interface language).
    [[nodiscard]] bool rightToLeft() const { return layout_.settings().rightToLeft; }
    void setRightToLeft(bool rightToLeft);
    enum Tool { HandTool = 0, SelectTool = 1, SnapshotTool = 2, MarqueeTool = 3 };
    Q_ENUM(Tool)

    [[nodiscard]] QVariantList outline() const { return outline_; }
    [[nodiscard]] QStringList pageLabels() const { return pageLabels_; }
    [[nodiscard]] bool hasText() const { return hasText_; }
    [[nodiscard]] bool textReady() const { return textReady_; }
    [[nodiscard]] QVariantList searchHits() const { return searchHits_; }
    [[nodiscard]] int searchIndex() const { return searchIndex_; }
    [[nodiscard]] bool searching() const { return searching_; }
    [[nodiscard]] qreal searchProgress() const { return searchProgress_; }
    [[nodiscard]] QString searchQuery() const { return searchQuery_; }
    [[nodiscard]] QString selectedText() const { return selectedText_; }
    [[nodiscard]] int tool() const { return tool_; }
    void setTool(int tool);
    [[nodiscard]] bool autoScrolling() const { return autoScrollTimer_.isActive(); }
    [[nodiscard]] QString hoveredLink() const { return hoveredLink_; }
    [[nodiscard]] QString pageText() const { return pageText_; }

    Q_INVOKABLE void find(const QString& query, bool matchCase, bool wholeWords);
    Q_INVOKABLE void findNext();
    Q_INVOKABLE void findPrevious();
    Q_INVOKABLE void showSearchHit(int index);
    Q_INVOKABLE void clearSearch();
    Q_INVOKABLE void copySelection();
    Q_INVOKABLE void selectAllOnPage();
    Q_INVOKABLE void clearSelection();
    Q_INVOKABLE void requestWordCount();
    // N5.3 Auto Create Bookmarks: suggests headings from the whole book in the
    // background (headingsProgress, then headingsReady). A new scan or
    // cancelHeadingScan stops the previous one.
    Q_INVOKABLE void scanHeadings(int maxLevels);
    Q_INVOKABLE void cancelHeadingScan();
    Q_INVOKABLE void fitVisible();
    Q_INVOKABLE bool goToLabel(const QString& label);
    Q_INVOKABLE void goToDestination(int zeroBasedPage, QPointF location);
    // N5.3 bookmark destinations: a point given as fractions (0–1) of the
    // unrotated page, origin top-left; fy = 0 means the top of the page.
    Q_INVOKABLE void goToNormalized(int zeroBasedPage, qreal fx, qreal fy);
    // Where the reader is now, for "add a bookmark here": {page (zero-based),
    // fx, fy, zoom}. Empty when no document is shown.
    Q_INVOKABLE QVariantMap viewLocation() const;
    Q_INVOKABLE void toggleAutoScroll();
    Q_INVOKABLE void changeAutoScrollSpeed(int steps);
    Q_INVOKABLE void stopAutoScroll();
    Q_INVOKABLE void goBack();
    Q_INVOKABLE void goForward();
    // Page size in points with /Rotate and the view rotation applied; for thumbnails.
    Q_INVOKABLE QSizeF pageSizePoints(int oneBasedPage) const;
    [[nodiscard]] bool canGoBack() const { return history_.canGoBack(); }
    [[nodiscard]] bool canGoForward() const { return history_.canGoForward(); }

    // 0 Paper, 1 Sepia, 2 Night, 3 High contrast. Rendering only; never written to the PDF.
    [[nodiscard]] int readingColors() const { return readingColors_; }
    void setReadingColors(int colors);
    [[nodiscard]] int rotation() const { return static_cast<int>(layout_.settings().viewRotation) * 90; }

signals:
    void controllerChanged();
    void sessionIdChanged();
    void activeChanged();
    void scrollChanged();
    void layoutChanged();
    void zoomChanged();
    void fitModeChanged();
    void currentPageChanged();
    void loadingChanged();
    void pageColorChanged();
    void readingColorsChanged();
    void rotationChanged();
    void historyChanged();
    void pageDisplayChanged();
    void textInfoChanged();
    void searchChanged();
    void selectionChanged();
    void toolChanged();
    void autoScrollChanged();
    void hoveredLinkChanged();
    void pageTextChanged();
    // An external link was clicked; QML asks the reader before anything opens.
    void externalLinkRequested(const QString& url);
    void wordCountReady(int words, int characters, int pages);
    void headingsProgress(int pagesDone, int pageCount);
    void headingsReady(const QVariantList& headings, double bodySize, bool truncated);
    void snapshotCopied();
    void contextMenuRequested(qreal x, qreal y);

protected:
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* data) override;
    void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void hoverMoveEvent(QHoverEvent* event) override;
    void releaseResources() override;
    void itemChange(ItemChange change, const ItemChangeData& value) override;

private:
    void ensureWorker();
    void stopWorker();
    void openDocument();
    void onOpened(quint64 revision, int error, const QList<QSizeF>& pageSizes);
    void onRendered(quint64 ticket, quint64 revision, int page, const QImage& image);
    void onRenderFailed(quint64 ticket, quint64 revision, int page);
    void dispatch(const atlas::reader::SchedulerActions& actions);
    void scheduleRender();
    void runSchedule();
    void applyFit();
    void relayout();
    void zoomAround(double zoom, QPointF anchor);
    void clampScroll();
    void updateCurrentPage();
    void jumpToIndex(std::size_t index);
    [[nodiscard]] atlas::reader::ViewPosition currentPosition() const;
    void showPosition(const atlas::reader::ViewPosition& position);
    void recordJump();
    void ensureTextWorker();
    void stopTextWorker();
    void onTextInfo(quint64 revision, const QVariantList& outline, const QStringList& labels, bool hasText);
    void onSearchBatch(quint64 searchId, const QVariantList& hits, int pagesDone, int pageCount, bool finished);
    void onSelected(quint64 requestId, int page, const QString& text, const QVariantList& rects);
    void onPageLinks(quint64 revision, int page, const QVariantList& links);
    void requestVisibleLinks();
    void requestPageText();
    // Mapping between page points (with /Rotate) and content coordinates.
    [[nodiscard]] QRectF pointsToContent(std::size_t page, const QRectF& points) const;
    [[nodiscard]] QPointF contentToPoints(std::size_t page, QPointF content) const;
    [[nodiscard]] std::optional<std::size_t> pageAtContent(QPointF content) const;
    [[nodiscard]] QPointF toContent(QPointF itemPos) const;
    [[nodiscard]] std::optional<QVariantMap> linkAt(QPointF itemPos) const;
    void finishSnapshot(const QRectF& itemRect);
    void finishMarquee(const QRectF& itemRect);
    void changeLayout(const std::function<void(atlas::reader::LayoutSettings&)>& edit);
    void scrollBy(qreal delta);  // user scrolling; turns the page in single-page modes
    [[nodiscard]] bool singlePageMode() const;
    [[nodiscard]] std::pair<qreal, qreal> rowScrollRange(std::size_t row) const;
    void dropAllTextures();
    void queueTextureRelease(const std::vector<atlas::reader::TextureKey>& keys);
    [[nodiscard]] double devicePixelRatio() const;
    [[nodiscard]] double horizontalInset() const;

    QPointer<ReaderController> controller_;
    qulonglong sessionId_{};
    bool active_{};
    bool loading_{};
    QString errorKey_;
    QColor pageColor_{Qt::white};
    int readingColors_{};
    QColor pageShadowColor_{0, 0, 0, 38};

    atlas::reader::ViewportLayout layout_;
    atlas::reader::ViewHistory history_;
    std::size_t singleRow_{}; // current row in single-page modes

    QThread textThread_;
    DocumentTextWorker* textWorker_{};
    std::shared_ptr<std::atomic<quint64>> latestSearch_;
    QVariantList outline_;
    QStringList pageLabels_;
    bool hasText_{true};
    bool textReady_{};
    QString searchQuery_;
    QVariantList searchHits_;
    int searchIndex_{-1};
    bool searching_{};
    qreal searchProgress_{};
    bool searchWantsFirstHit_{};
    quint64 selectionRequest_{};
    int selectionPage_{-1};
    QVariantList selectionRects_;
    QString selectedText_;
    std::unordered_map<int, QVariantList> links_;
    std::unordered_map<int, bool> linksRequested_;
    QString hoveredLink_;
    int tool_{HandTool};
    bool pressed_{};
    bool dragged_{};
    QPointF pressPos_;
    QPointF pressScroll_;
    std::optional<std::size_t> pressPage_;
    QRectF rubberBand_; // item coordinates, snapshot/marquee
    QTimer autoScrollTimer_;
    QTimer pageTextTimer_;
    quint64 pageTextRequest_{};
    QString pageText_;
    qreal autoScrollSpeed_{1.5}; // logical px per tick (16 ms)
    quint64 wordCountRequest_{};
    quint64 headingScan_{};
    quint64 contentBoxRequest_{};
    FitMode fitMode_{FitMode::Width};
    qreal contentX_{};
    qreal contentY_{};
    int currentPage_{};
    quint64 documentRevision_{};
    bool documentReady_{};
    bool scheduleQueued_{};

    atlas::reader::RenderScheduler scheduler_{2};
    atlas::reader::PageTextureCache cache_;
    std::unordered_map<quint64, atlas::reader::TextureKey> ticketKeys_;

    // GUI thread -> render thread hand-off, touched only while the GUI thread
    // is blocked in the scene-graph sync phase (updatePaintNode).
    std::unordered_map<atlas::reader::TextureKey, QImage, atlas::reader::TextureKeyHash> uploads_;
    std::vector<atlas::reader::TextureKey> releases_;
    bool releaseAll_{};
    std::unordered_map<atlas::reader::TextureKey, QSGTexture*, atlas::reader::TextureKeyHash>
        textures_; // render thread only

    QMetaObject::Connection invalidatedConnection_;
    QThread workerThread_;
    PdfRenderWorker* worker_{};
    std::shared_ptr<RenderCancellation> cancellation_;
};

} // namespace atlas::app
