#include "Logger.h"
#include <ctime>
#include <iomanip>
#include <cstdarg>
#include <vector>

namespace MacTrafficLights {

Logger& Logger::Instance() {
    static Logger instance;
    return instance;
}

Logger::Logger() {
    Init();
}

Logger::~Logger() {
    if (m_file.is_open()) {
        m_file.close();
    }
}

std::wstring Logger::GetDefaultLogPath() const {
    wchar_t path[MAX_PATH] = { 0 };
    if (GetModuleFileNameW(NULL, path, MAX_PATH) > 0) {
        wchar_t* lastSlash = wcsrchr(path, L'\\');
        if (lastSlash) {
            *lastSlash = L'\0';
            return std::wstring(path) + L"\\MacTrafficLights.log";
        }
    }
    return L"MacTrafficLights.log";
}

void Logger::Init(const std::wstring& logFilePath) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_file.is_open()) {
        m_file.close();
    }

    std::wstring targetPath = logFilePath.empty() ? GetDefaultLogPath() : logFilePath;
    m_file.open(targetPath, std::ios::out | std::ios::app);
    if (m_file.is_open()) {
        m_file << L"=== MacTrafficLights Log Started ===" << std::endl;
    }
}

void Logger::Log(LogLevel level, const std::wstring& message) {
    if (level < m_minLevel) return;

    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_file.is_open()) return;

    std::time_t now = std::time(nullptr);
    std::tm tm_now;
    localtime_s(&tm_now, &now);

    const wchar_t* levelStr = L"INFO";
    switch (level) {
        case LogLevel::Debug:   levelStr = L"DEBUG"; break;
        case LogLevel::Info:    levelStr = L"INFO "; break;
        case LogLevel::Warning: levelStr = L"WARN "; break;
        case LogLevel::Error:   levelStr = L"ERROR"; break;
    }

    m_file << L"[" << std::put_time(&tm_now, L"%Y-%m-%d %H:%M:%S") << L"] "
           << L"[" << levelStr << L"] "
           << message << std::endl;
    m_file.flush();
}

void Logger::LogFormat(LogLevel level, const wchar_t* format, ...) {
    if (level < m_minLevel) return;

    va_list args;
    va_start(args, format);
    int size = _vscwprintf(format, args);
    if (size > 0) {
        std::vector<wchar_t> buffer(size + 1);
        vswprintf_s(buffer.data(), buffer.size(), format, args);
        Log(level, std::wstring(buffer.data()));
    }
    va_end(args);
}

} // namespace MacTrafficLights
