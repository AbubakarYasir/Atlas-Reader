#pragma once

// Atlas-owned virtualized page viewport (N4.2).
//
// The pure-C++ model (`ViewportLayout`, `planRenders`, `RenderScheduler`,
// `PageTextureCache`) decides geometry, render order, cancellation and memory;
// this item only executes those decisions with Qt PDF on a worker thread and
// draws the resulting textures in the Qt Quick scene graph. No PDF work runs on
// the GUI or render thread.

#include "core/reader/RenderPlanner.h"
#include "core/reader/ViewportLayout.h"

#include <QColor>
#include <QImage>
#include <QPointer>
#include <QQuickItem>
#include <QSizeF>
#include <QThread>

#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

class QSGTexture;

namespace atlas::app {

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

protected:
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData* data) override;
    void geometryChange(const QRectF& newGeometry, const QRectF& oldGeometry) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
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
