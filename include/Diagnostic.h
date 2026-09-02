#pragma once

#include <windows.h>
#include <psapi.h>
#include <string>

namespace MacTrafficLights {

struct DiagnosticMetrics {
    double cpuUsagePercent = 0.0;
    double memoryWorkingSetMB = 0.0;
    double memoryPrivateBytesMB = 0.0;
    size_t trackedWindowsCount = 0;
    size_t activeOverlaysCount = 0;
    bool hooksInstalled = false;
};

class DiagnosticManager {
public:
    static DiagnosticManager& Instance();

    void UpdateMetrics();
    const DiagnosticMetrics& GetMetrics() const { return m_metrics; }

    void SetCounts(size_t tracked, size_t activeOverlays, bool hooksOk);

private:
    DiagnosticManager();
    ~DiagnosticManager() = default;

    DiagnosticMetrics m_metrics;
    ULARGE_INTEGER m_lastCpuTime{};
    ULARGE_INTEGER m_lastSysTime{};
    int m_processorCount = 1;
};

} // namespace MacTrafficLights
