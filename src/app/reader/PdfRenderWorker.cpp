#include "app/reader/PdfRenderWorker.h"

#include <QEventLoop>
#include <QRgb>
#include <QFile>
#include <QFileInfo>
#include <QPdfDocument>
#include <QPdfDocumentRenderOptions>

#include <algorithm>

namespace atlas::app {

namespace {

QPdfDocumentRenderOptions::Rotation toQtRotation(int quarterTurns) {
    switch (((quarterTurns % 4) + 4) % 4) {
    case 1: return QPdfDocumentRenderOptions::Rotation::Clockwise90;
    case 2: return QPdfDocumentRenderOptions::Rotation::Clockwise180;
    case 3: return QPdfDocumentRenderOptions::Rotation::Clockwise270;
    default: return QPdfDocumentRenderOptions::Rotation::None;
    }
}

} // namespace

// Reading colours remap luminance between a background and a text colour so
// text, line art and images stay legible (Foxit's "replace document colours"
// concept). Rendering only.
void applyReadingColors(QImage& image, int readingColors) {
    if (readingColors <= 0 || readingColors > 3) return;
    // background (luminance 255) and ink (luminance 0) per mode
    static constexpr int paper[4][3] = {{255, 255, 255}, {244, 236, 216}, {27, 27, 29}, {0, 0, 0}};
    static constexpr int ink[4][3] = {{0, 0, 0}, {67, 52, 34}, {222, 220, 214}, {255, 255, 255}};
    const int* bg = paper[readingColors];
    const int* fg = ink[readingColors];
    const bool contrast = readingColors == 3;
    for (int y = 0; y < image.height(); ++y) {
        auto* line = reinterpret_cast<QRgb*>(image.scanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            const QRgb px = line[x];
            const int alpha = qAlpha(px);
            int lum = (qRed(px) * 299 + qGreen(px) * 587 + qBlue(px) * 114) / 1000;
            if (contrast) lum = lum > 160 ? 255 : (lum < 96 ? 0 : (lum - 96) * 4);
            // Keep some of the original colour so figures stay recognisable.
            const int sat = contrast ? 0 : 35; // percent of original chroma kept
            const int r = fg[0] + (bg[0] - fg[0]) * lum / 255;
            const int g = fg[1] + (bg[1] - fg[1]) * lum / 255;
            const int b = fg[2] + (bg[2] - fg[2]) * lum / 255;
            const int cr = qRed(px) - lum;
            const int cg = qGreen(px) - lum;
            const int cb = qBlue(px) - lum;
            line[x] = qRgba(std::clamp(r + cr * sat / 100, 0, 255) * alpha / 255,
                            std::clamp(g + cg * sat / 100, 0, 255) * alpha / 255,
                            std::clamp(b + cb * sat / 100, 0, 255) * alpha / 255, alpha);
        }
    }
}


PdfRenderWorker::PdfRenderWorker(std::shared_ptr<RenderCancellation> cancellation)
    : cancellation_{std::move(cancellation)} {}

PdfRenderWorker::~PdfRenderWorker() { close(); }

void PdfRenderWorker::close() {
    if (document_) {
        document_->close();
        delete document_;
        document_ = nullptr;
    }
    if (buffer_.isOpen()) buffer_.close();
    buffer_.setData(QByteArray{});
    bytes_.clear();
    bytes_.squeeze();
}

void PdfRenderWorker::open(const QString& sourcePath, const QString& password,
                           quint64 documentRevision) {
    close();
    revision_ = documentRevision;

    {
        // Read once and release the file immediately: Atlas must never keep a
        // handle that blocks rename/move, and never writes during reading.
        QFile file{sourcePath};
        if (!QFileInfo::exists(sourcePath) || !file.open(QIODevice::ReadOnly)) {
            emit opened(documentRevision, static_cast<int>(OpenError::missing), {});
            return;
        }
        bytes_ = file.readAll();
    }

    buffer_.setData(bytes_);
    if (!buffer_.open(QIODevice::ReadOnly)) {
        emit opened(documentRevision, static_cast<int>(OpenError::failed), {});
        return;
    }

    document_ = new QPdfDocument{this};
    if (!password.isEmpty()) document_->setPassword(password);
    document_->load(&buffer_);
    if (document_->status() == QPdfDocument::Status::Loading) {
        // A complete in-memory buffer normally loads synchronously; wait if not.
        QEventLoop loop;
        connect(document_, &QPdfDocument::statusChanged, &loop, [&loop](QPdfDocument::Status status) {
            if (status != QPdfDocument::Status::Loading) loop.quit();
        });
        if (document_->status() == QPdfDocument::Status::Loading) loop.exec();
    }

    OpenError error = OpenError::none;
    switch (document_->error()) {
    case QPdfDocument::Error::None: break;
    case QPdfDocument::Error::FileNotFound: error = OpenError::missing; break;
    case QPdfDocument::Error::IncorrectPassword: error = OpenError::password; break;
    case QPdfDocument::Error::UnsupportedSecurityScheme:
        error = OpenError::unsupportedSecurity;
        break;
    case QPdfDocument::Error::InvalidFileFormat: error = OpenError::malformed; break;
    case QPdfDocument::Error::DataNotYetAvailable:
    case QPdfDocument::Error::Unknown: error = OpenError::failed; break;
    }
    if (error == OpenError::none && document_->status() != QPdfDocument::Status::Ready) {
        error = OpenError::failed;
    }

    QList<QSizeF> sizes;
    if (error == OpenError::none) {
        const int count = document_->pageCount();
        sizes.reserve(count);
        for (int page = 0; page < count; ++page) {
            // Qt PDF reports the page size with the page's /Rotate applied.
            sizes.push_back(document_->pagePointSize(page));
        }
    } else {
        close();
    }
    emit opened(documentRevision, static_cast<int>(error), sizes);
}

void PdfRenderWorker::render(quint64 ticket, quint64 documentRevision, int page, QSize pixelSize,
                             int rotationQuarterTurns, int readingColors) {
    if (cancellation_->take(ticket)) return; // superseded before it started
    if (!document_ || documentRevision != revision_ || page < 0 ||
        page >= document_->pageCount() || pixelSize.isEmpty()) {
        emit renderFailed(ticket, documentRevision, page);
        return;
    }

    QPdfDocumentRenderOptions options;
    options.setRotation(toQtRotation(rotationQuarterTurns));
    options.setRenderFlags(QPdfDocumentRenderOptions::RenderFlag::Annotations);
    QImage image = document_->render(page, pixelSize, options);
    if (cancellation_->take(ticket)) return; // cancelled while rendering
    if (image.isNull()) {
        emit renderFailed(ticket, documentRevision, page);
        return;
    }
    if (image.format() != QImage::Format_ARGB32_Premultiplied) {
        image.convertTo(QImage::Format_ARGB32_Premultiplied);
    }
    applyReadingColors(image, readingColors);
    emit rendered(ticket, documentRevision, page, image);
}

} // namespace atlas::app
