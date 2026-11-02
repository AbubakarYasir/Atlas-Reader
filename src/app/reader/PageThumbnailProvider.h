#pragma once

// Page thumbnails for the Pages panel (N4.4), served to QML as
// "image://atlasthumb/<sessionId>/<revision>/<page>/<readingColors>".
// Rendering runs on one background thread; documents are opened from an
// in-memory copy so the file is never locked, exactly like the viewport.

#include <QHash>
#include <QMutex>
#include <QQuickAsyncImageProvider>
#include <QString>
#include <QThreadPool>

#include <memory>

namespace atlas::app {

class ReaderController;

class PageThumbnailProvider final : public QQuickAsyncImageProvider {
public:
    struct Source {
        QString path;
        QString password; // memory only, never logged
        quint64 revision{};
    };
    struct Document;

    PageThumbnailProvider();
    ~PageThumbnailProvider() override;

    // GUI thread: copy the open sessions' render sources (called on every
    // change of the reader model).
    void refresh(const ReaderController& controller);

    QQuickImageResponse* requestImageResponse(const QString& id, const QSize& requestedSize) override;

    [[nodiscard]] std::shared_ptr<Document> documentFor(qulonglong sessionId);

private:
    QMutex mutex_;
    QHash<qulonglong, Source> sources_;
    QHash<qulonglong, std::shared_ptr<Document>> documents_;
    QThreadPool pool_;
};

} // namespace atlas::app
