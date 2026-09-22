#pragma once

#include <QJsonArray>
#include <QJsonObject>

#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

#ifdef Q_OS_WIN
#include <windows.h>
#include <psapi.h>
#endif

namespace atlas::bench {

struct MemorySnapshot {
    double workingSetMiB = 0.0;
    double peakWorkingSetMiB = 0.0;
};

inline double bytesToMiB(std::size_t bytes)
{
    return static_cast<double>(bytes) / (1024.0 * 1024.0);
}

inline MemorySnapshot memorySnapshot()
{
#ifdef Q_OS_WIN
    PROCESS_MEMORY_COUNTERS_EX counters{};
    counters.cb = sizeof(counters);
    if (GetProcessMemoryInfo(
            GetCurrentProcess(),
            reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&counters),
            static_cast<DWORD>(sizeof(counters)))) {
        return {
            bytesToMiB(static_cast<std::size_t>(counters.WorkingSetSize)),
            bytesToMiB(static_cast<std::size_t>(counters.PeakWorkingSetSize)),
        };
    }
#endif
    return {};
}

inline double nearestRank(std::vector<double> values, double percentile)
{
    if (values.empty()) {
        return 0.0;
    }
    std::sort(values.begin(), values.end());
    const double rawRank = std::ceil(percentile * static_cast<double>(values.size()));
    const std::size_t rank = static_cast<std::size_t>(std::max(1.0, rawRank));
    return values[std::min(rank - 1U, values.size() - 1U)];
}

inline QJsonObject metricSummary(double firstMs, const std::vector<double>& samples)
{
    QJsonObject output;
    output.insert(QStringLiteral("first_ms"), firstMs);
    output.insert(QStringLiteral("sample_count"), static_cast<int>(samples.size()));

    QJsonArray raw;
    for (const double value : samples) {
        raw.append(value);
    }
    output.insert(QStringLiteral("samples_ms"), raw);

    if (samples.empty()) {
        output.insert(QStringLiteral("mean_ms"), 0.0);
        output.insert(QStringLiteral("p50_ms"), 0.0);
        output.insert(QStringLiteral("p95_ms"), 0.0);
        output.insert(QStringLiteral("max_ms"), 0.0);
        return output;
    }

    const double sum = std::accumulate(samples.begin(), samples.end(), 0.0);
    output.insert(QStringLiteral("mean_ms"), sum / static_cast<double>(samples.size()));
    output.insert(QStringLiteral("p50_ms"), nearestRank(samples, 0.50));
    output.insert(QStringLiteral("p95_ms"), nearestRank(samples, 0.95));
    output.insert(QStringLiteral("max_ms"), *std::max_element(samples.begin(), samples.end()));
    return output;
}

inline QJsonObject memoryEvidence(const MemorySnapshot& before, const MemorySnapshot& after)
{
    QJsonObject output;
    output.insert(QStringLiteral("working_set_before_mib"), before.workingSetMiB);
    output.insert(QStringLiteral("working_set_after_mib"), after.workingSetMiB);
    output.insert(QStringLiteral("working_set_delta_mib"), after.workingSetMiB - before.workingSetMiB);
    output.insert(QStringLiteral("peak_working_set_mib"), std::max(before.peakWorkingSetMiB, after.peakWorkingSetMiB));
    return output;
}

} // namespace atlas::bench
