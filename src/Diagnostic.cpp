#include "Diagnostic.h"

namespace MacTrafficLights {

DiagnosticManager& DiagnosticManager::Instance() {
    static DiagnosticManager instance;
    return instance;
}

DiagnosticManager::DiagnosticManager() {
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    m_processorCount = sysInfo.dwNumberOfProcessors > 0 ? sysInfo.dwNumberOfProcessors : 1;

    FILETIME now, creation, exit, kernel, user;
    GetSystemTimeAsFileTime(&now);
    m_lastSysTime.LowPart = now.dwLowDateTime;
    m_lastSysTime.HighPart = now.dwHighDateTime;

    if (GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel, &user)) {
        ULARGE_INTEGER k, u;
        k.LowPart = kernel.dwLowDateTime;
        k.HighPart = kernel.dwHighDateTime;
        u.LowPart = user.dwLowDateTime;
        u.HighPart = user.dwHighDateTime;
        m_lastCpuTime.QuadPart = k.QuadPart + u.QuadPart;
    }
}

void DiagnosticManager::SetCounts(size_t tracked, size_t activeOverlays, bool hooksOk) {
    m_metrics.trackedWindowsCount = tracked;
    m_metrics.activeOverlaysCount = activeOverlays;
    m_metrics.hooksInstalled = hooksOk;
}

void DiagnosticManager::UpdateMetrics() {
    // 1. Process CPU calculation
    FILETIME now, creation, exit, kernel, user;
    GetSystemTimeAsFileTime(&now);

    ULARGE_INTEGER currentSysTime;
    currentSysTime.LowPart = now.dwLowDateTime;
    currentSysTime.HighPart = now.dwHighDateTime;

    if (GetProcessTimes(GetCurrentProcess(), &creation, &exit, &kernel, &user)) {
        ULARGE_INTEGER k, u, currentCpuTime;
        k.LowPart = kernel.dwLowDateTime;
        k.HighPart = kernel.dwHighDateTime;
        u.LowPart = user.dwLowDateTime;
        u.HighPart = user.dwHighDateTime;
        currentCpuTime.QuadPart = k.QuadPart + u.QuadPart;

        ULONGLONG sysDiff = currentSysTime.QuadPart - m_lastSysTime.QuadPart;
        ULONGLONG cpuDiff = currentCpuTime.QuadPart - m_lastCpuTime.QuadPart;

        if (sysDiff > 0) {
            m_metrics.cpuUsagePercent = (double)(cpuDiff * 100.0) / (double)(sysDiff * m_processorCount);
            if (m_metrics.cpuUsagePercent < 0.0) m_metrics.cpuUsagePercent = 0.0;
            if (m_metrics.cpuUsagePercent > 100.0) m_metrics.cpuUsagePercent = 100.0;
        }

        m_lastSysTime = currentSysTime;
        m_lastCpuTime = currentCpuTime;
    }

    // 2. Memory calculation
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        m_metrics.memoryWorkingSetMB = (double)pmc.WorkingSetSize / (1024.0 * 1024.0);
        m_metrics.memoryPrivateBytesMB = (double)pmc.PrivateUsage / (1024.0 * 1024.0);
    }
}

} // namespace MacTrafficLights
