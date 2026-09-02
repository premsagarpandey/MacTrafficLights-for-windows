#pragma once

#include <windows.h>
#include <string>
#include <fstream>
#include <mutex>

namespace MacTrafficLights {

enum class LogLevel {
    Debug,
    Info,
    Warning,
    Error
};

class Logger {
public:
    static Logger& Instance();

    void Init(const std::wstring& logFilePath = L"");
    void Log(LogLevel level, const std::wstring& message);
    void LogFormat(LogLevel level, const wchar_t* format, ...);

    void SetLogLevel(LogLevel level) { m_minLevel = level; }

private:
    Logger();
    ~Logger();

    std::wstring GetDefaultLogPath() const;

    std::wofstream m_file;
    std::mutex m_mutex;
    LogLevel m_minLevel = LogLevel::Info;
};

#define LOG_DEBUG(msg) MacTrafficLights::Logger::Instance().Log(MacTrafficLights::LogLevel::Debug, msg)
#define LOG_INFO(msg)  MacTrafficLights::Logger::Instance().Log(MacTrafficLights::LogLevel::Info, msg)
#define LOG_WARN(msg)  MacTrafficLights::Logger::Instance().Log(MacTrafficLights::LogLevel::Warning, msg)
#define LOG_ERROR(msg) MacTrafficLights::Logger::Instance().Log(MacTrafficLights::LogLevel::Error, msg)

} // namespace MacTrafficLights
