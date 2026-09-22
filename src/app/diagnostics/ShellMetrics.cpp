#include "app/diagnostics/ShellMetrics.h"

#include "app/logging/Logging.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QQuickWindow>
#include <QScreen>
#include <QSize>

#include <algorithm>
#include <cmath>

namespace atlas::diagnostics {

ShellMetrics::ShellMetrics(Clock::time_point processStart,
                           QString metricsPath,
                           bool runResizeBenchmark,
                           QObject* parent)
    : QObject(parent),
      processStart_(processStart),
      metricsPath_(std::move(metricsPath)),
      runResizeBenchmark_(runResizeBenchmark) {
    resizeTimer_.setInterval(16);
    resizeTimer_.setTimerType(Qt::PreciseTimer);

    connect(&resizeTimer_, &QTimer::timeout, this, [this] {
        if (!window_) {
            resizeTimer_.stop();
            resizeActive_ = false;
            return;
        }

        constexpr int resizeOperations = 120;
        static const QSize sizes[] = {
            QSize(900, 600),
            QSize(1180, 760),
            QSize(1360, 820),
            QSize(1024, 700),
        };

        if (resizeStep_ >= resizeOperations) {
            resizeTimer_.stop();
            QTimer::singleShot(300, this, [this] { finishResizeBenchmark(); });
            return;
        }

        const auto sizeIndex = resizeStep_ % static_cast<int>(std::size(sizes));
        window_->resize(sizes[sizeIndex]);
        ++resizeStep_;
    });

    if (!metricsPath_.isEmpty()) {
        const QFileInfo info(metricsPath_);
        const QString directory = info.absolutePath();
        if (!directory.isEmpty()) {
            QDir().mkpath(directory);
        }

        metricsFile_.setFileName(metricsPath_);
        if (!metricsFile_.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
            qCWarning(atlas::logging::performance)
                << "Metrics output could not be opened; continuing without a metrics file";
        }
    }
}

void ShellMetrics::recordCheckpoint(const QString& metricName, Clock::time_point checkpoint) {
    writeMetric(metricName, elapsedMsAt(checkpoint), QStringLiteral("ms"));
}

void ShellMetrics::recordStage(const QString& metricName) {
    writeMetric(metricName, elapsedMs(), QStringLiteral("ms"));
}

void ShellMetrics::recordQmlLoaded() {
    writeMetric(QStringLiteral("startup.qml_loaded_ms"), elapsedMs(), QStringLiteral("ms"));
}

void ShellMetrics::attach(QQuickWindow* window) {
    window_ = window;
    if (!window_) {
        return;
    }

    writeMetric(QStringLiteral("display.device_pixel_ratio"), window_->devicePixelRatio(), QStringLiteral("ratio"));
    writeMetric(QStringLiteral("display.window_width_px"), window_->width(), QStringLiteral("px"));
    writeMetric(QStringLiteral("display.window_height_px"), window_->height(), QStringLiteral("px"));

    if (const QScreen* screen = window_->screen()) {
        writeMetric(QStringLiteral("display.logical_dpi"), screen->logicalDotsPerInch(), QStringLiteral("dpi"));
        writeMetric(QStringLiteral("display.physical_dpi"), screen->physicalDotsPerInch(), QStringLiteral("dpi"));
        writeMetric(QStringLiteral("display.refresh_hz"), screen->refreshRate(), QStringLiteral("hz"));
    }

    connect(window_, &QQuickWindow::frameSwapped, this, [this] { onFrameSwapped(); });
}

void ShellMetrics::recordShutdown() {
    writeMetric(QStringLiteral("lifecycle.shutdown_ms"), elapsedMs(), QStringLiteral("ms"));
    if (metricsFile_.isOpen()) {
        metricsFile_.flush();
    }
}

void ShellMetrics::onFrameSwapped() {
    const auto now = Clock::now();

    if (!firstFrameRecorded_) {
        firstFrameRecorded_ = true;
        writeMetric(QStringLiteral("startup.first_frame_ms"), elapsedMs(), QStringLiteral("ms"));

        if (runResizeBenchmark_) {
            QTimer::singleShot(250, this, [this] { startResizeBenchmark(); });
        }
    }

    if (resizeActive_ && previousFrame_ != Clock::time_point{}) {
        const auto interval = std::chrono::duration<double, std::milli>(now - previousFrame_).count();
        resizeFrameIntervalsMs_.push_back(interval);
    }

    previousFrame_ = now;
}

void ShellMetrics::startResizeBenchmark() {
    if (!window_ || resizeActive_) {
        return;
    }

    resizeFrameIntervalsMs_.clear();
    resizeStep_ = 0;
    previousFrame_ = Clock::now();
    resizeActive_ = true;
    resizeTimer_.start();
    qCInfo(atlas::logging::performance) << "Empty-shell resize benchmark started";
}

void ShellMetrics::finishResizeBenchmark() {
    if (!resizeActive_) {
        return;
    }

    resizeActive_ = false;

    writeMetric(QStringLiteral("resize.frame_samples"),
                static_cast<double>(resizeFrameIntervalsMs_.size()),
                QStringLiteral("count"));

    if (!resizeFrameIntervalsMs_.empty()) {
        writeMetric(QStringLiteral("resize.frame_p50_ms"),
                    percentile(resizeFrameIntervalsMs_, 0.50),
                    QStringLiteral("ms"));
        writeMetric(QStringLiteral("resize.frame_p95_ms"),
                    percentile(resizeFrameIntervalsMs_, 0.95),
                    QStringLiteral("ms"));
        writeMetric(QStringLiteral("resize.frame_p99_ms"),
                    percentile(resizeFrameIntervalsMs_, 0.99),
                    QStringLiteral("ms"));
    }

    qCInfo(atlas::logging::performance) << "Empty-shell resize benchmark completed";
}

void ShellMetrics::writeMetric(const QString& name, double value, const QString& unit) {
    qCInfo(atlas::logging::performance).noquote()
        << QStringLiteral("ATLAS_METRIC %1=%2 %3").arg(name).arg(value, 0, 'f', 3).arg(unit);

    if (!metricsFile_.isOpen()) {
        return;
    }

    const QJsonObject object{
        {QStringLiteral("metric"), name},
        {QStringLiteral("value"), value},
        {QStringLiteral("unit"), unit},
        {QStringLiteral("elapsed_process_ms"), elapsedMs()},
    };

    metricsFile_.write(QJsonDocument(object).toJson(QJsonDocument::Compact));
    metricsFile_.write("\n");
    metricsFile_.flush();
}

double ShellMetrics::elapsedMs() const {
    return elapsedMsAt(Clock::now());
}

double ShellMetrics::elapsedMsAt(Clock::time_point checkpoint) const {
    return std::chrono::duration<double, std::milli>(checkpoint - processStart_).count();
}

double ShellMetrics::percentile(std::vector<double> samples, double fraction) {
    if (samples.empty()) {
        return 0.0;
    }

    std::sort(samples.begin(), samples.end());
    const auto lastIndex = samples.size() - 1;
    const auto rawIndex = fraction * static_cast<double>(lastIndex);
    const auto index = static_cast<std::size_t>(std::ceil(rawIndex));
    return samples[index];
}

} // namespace atlas::diagnostics
