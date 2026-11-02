#include "app/reader/PageThumbnailProvider.h"

#include "app/reader/PdfRenderWorker.h"
#include "app/reader/ReaderController.h"

#include <QBuffer>
#include <QCoreApplication>
#include <QPointer>
#include <QEventLoop>
#include <QFile>
#include <QImage>
#include <QPdfDocument>
#include <QPdfDocumentRenderOptions>
#include <QQuickTextureFactory>
#include <QRunnable>

#include <algorithm>

namespace atlas::app {

struct PageThumbnailProvider::Document {
    QMutex mutex;
    quint64 revision{};
    bool failed{};
    QByteArray bytes;
    std::unique_ptr<QBuffer> buffer;
    std::unique_ptr<QPdfDocument> pdf;
    Source source;
};

namespace {

class ThumbnailResponse final : public QQuickImageResponse {
public:
    void deliver(QImage image) {
        image_ = std::move(image);
        emit finished();
    }
    QQuickTextureFactory* textureFactory() const override {
        return QQuickTextureFactory::textureFactoryForImage(image_);
    }

private:
    QImage image_;
};

class ThumbnailJob final : public QRunnable {
public:
    ThumbnailJob(std::shared_ptr<PageThumbnailProvider::Document> document, int page, int colors,
                 QSize size, ThumbnailResponse* response)
        : document_{std::move(document)}, page_{page}, colors_{colors}, size_{size},
          response_{response} {}

    void run() override {
        QImage image;
        if (document_) image = render();
        // Deliver on the GUI thread; the engine may already have dropped the response.
        QMetaObject::invokeMethod(QCoreApplication::instance(), [response = response_, image] {
            if (response) response->deliver(image);
        }, Qt::QueuedConnection);
    }

private:
    QImage render() {
        const QMutexLocker lock{&document_->mutex};
        auto& doc = *document_;
        if (!doc.pdf || doc.revision != doc.source.revision) {
            doc.pdf.reset();
            doc.buffer.reset();
            doc.failed = false;
            QFile file{doc.source.path};
            if (!file.open(QIODevice::ReadOnly)) {
                doc.failed = true;
                return {};
            }
            doc.bytes = file.readAll(); // read once; the file is released at once
            file.close();
            doc.buffer = std::make_unique<QBuffer>(&doc.bytes);
            doc.buffer->open(QIODevice::ReadOnly);
            doc.pdf = std::make_unique<QPdfDocument>();
            if (!doc.source.password.isEmpty()) doc.pdf->setPassword(doc.source.password);
            doc.pdf->load(doc.buffer.get());
            if (doc.pdf->status() == QPdfDocument::Status::Loading) {
                QEventLoop loop;
                QObject::connect(doc.pdf.get(), &QPdfDocument::statusChanged, &loop,
                                 [&loop](QPdfDocument::Status status) {
                                     if (status != QPdfDocument::Status::Loading) loop.quit();
                                 });
                if (doc.pdf->status() == QPdfDocument::Status::Loading) loop.exec();
            }
            doc.revision = doc.source.revision;
            doc.failed = doc.pdf->status() != QPdfDocument::Status::Ready;
        }
        if (doc.failed || page_ < 0 || page_ >= doc.pdf->pageCount()) return {};
        const QSizeF points = doc.pdf->pagePointSize(page_);
        if (points.isEmpty()) return {};
        const int width = std::clamp(size_.width() > 0 ? size_.width() : 160, 16, 1024);
        const int height = std::max(1, static_cast<int>(width * points.height() / points.width()));
        QPdfDocumentRenderOptions options;
        options.setRenderFlags(QPdfDocumentRenderOptions::RenderFlag::Annotations);
        QImage image = doc.pdf->render(page_, QSize{width, height}, options);
        if (image.isNull()) return {};
        image = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
        applyReadingColors(image, colors_);
        return image;
    }

    std::shared_ptr<PageThumbnailProvider::Document> document_;
    int page_;
    int colors_;
    QSize size_;
    QPointer<ThumbnailResponse> response_;
};

} // namespace

PageThumbnailProvider::PageThumbnailProvider() {
    pool_.setMaxThreadCount(1);   // one background renderer; never the GUI thread
    pool_.setExpiryTimeout(-1);   // keep the thread that owns the documents
}

PageThumbnailProvider::~PageThumbnailProvider() {
    pool_.clear();
    pool_.waitForDone();
}

void PageThumbnailProvider::refresh(const ReaderController& controller) {
    QHash<qulonglong, Source> next;
    for (int row = 0; row < controller.rowCount(); ++row) {
        const auto id = controller.data(controller.index(row), ReaderController::SessionIdRole).toULongLong();
        if (const auto source = controller.renderSource(id)) {
            next.insert(id, Source{source->path, source->password, source->revision});
        }
    }
    const QMutexLocker lock{&mutex_};
    sources_ = std::move(next);
    for (auto it = documents_.begin(); it != documents_.end();) {
        if (!sources_.contains(it.key())) it = documents_.erase(it); // closed tab frees memory
        else ++it;
    }
}

std::shared_ptr<PageThumbnailProvider::Document> PageThumbnailProvider::documentFor(qulonglong sessionId) {
    const QMutexLocker lock{&mutex_};
    const auto source = sources_.find(sessionId);
    if (source == sources_.end()) return {};
    auto& document = documents_[sessionId];
    if (!document) document = std::make_shared<Document>();
    {
        const QMutexLocker docLock{&document->mutex};
        document->source = *source;
    }
    return document;
}

QQuickImageResponse* PageThumbnailProvider::requestImageResponse(const QString& id, const QSize& requestedSize) {
    // id: sessionId/revision/page(1-based)/readingColors
    const auto parts = id.split(QLatin1Char('/'));
    auto* response = new ThumbnailResponse;
    if (parts.size() < 4) {
        QMetaObject::invokeMethod(response, [response] { response->deliver({}); }, Qt::QueuedConnection);
        return response;
    }
    const auto sessionId = parts.at(0).toULongLong();
    const int page = parts.at(2).toInt() - 1;
    const int colors = parts.at(3).toInt();
    pool_.start(new ThumbnailJob{documentFor(sessionId), page, colors, requestedSize, response});
    return response;
}

} // namespace atlas::app
