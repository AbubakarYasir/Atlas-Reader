#pragma once

#include <QFile>
#include <QObject>
#include <QPointer>
#include <QTimer>

#include <chrono>
#include <vector>

class QQuickWindow;

namespace atlas::diagnostics {

class ShellMetrics final : public QObject {
public:
    using Clock = std::chrono::steady_clock;

    ShellMetrics(Clock::time_point processStart,
                 QString metricsPath,
                 bool runResizeBenchmark,
                 QObject* parent = nullptr);

    void recordQmlLoaded();
    void attach(QQuickWindow* window);
    void recordShutdown();

private:
    void onFrameSwapped();
    void startResizeBenchmark();
    void finishResizeBenchmark();
    void writeMetric(const QString& name, double value, const QString& unit);
    [[nodiscard]] double elapsedMs() const;
    [[nodiscard]] static double percentile(std::vector<double> samples, double fraction);

    Clock::time_point processStart_;
    QString metricsPath_;
    QFile metricsFile_;
    QPointer<QQuickWindow> window_;
    QTimer resizeTimer_;
    bool runResizeBenchmark_ = false;
    bool firstFrameRecorded_ = false;
    bool resizeActive_ = false;
    int resizeStep_ = 0;
    Clock::time_point previousFrame_{};
    std::vector<double> resizeFrameIntervalsMs_;
};

} // namespace atlas::diagnostics
