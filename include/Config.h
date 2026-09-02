#pragma once

#include <windows.h>
#include <string>
#include <vector>
#include <algorithm>

namespace MacTrafficLights {

struct Configuration {
    bool enabled = true;
    int buttonSize = 12;         // Diameter in pixels at 96 DPI (100%)
    int buttonSpacing = 8;       // Spacing between circles in pixels at 96 DPI
    int leftMargin = 14;         // Offset from target window left border at 96 DPI
    int topMargin = 10;          // Offset from target window top border at 96 DPI
    int verticalAlignment = 0;   // 0 = Center in title bar, 1 = Fixed top margin
    bool dimWhenInactive = true; // Subtle grey/desaturated when target window is inactive
    bool showHoverSymbols = true;// Show x, -, + symbols on button hover
    bool startWithWindows = false;
    std::vector<std::wstring> excludedProcesses;
};

class ConfigManager {
public:
    static ConfigManager& Instance();

    void Load();
    void Save();
    void ResetToDefaults();

    const Configuration& GetConfig() const { return m_config; }
    Configuration& GetMutableConfig() { return m_config; }

    bool IsProcessExcluded(const std::wstring& processName) const;
    void AddExclusion(const std::wstring& processName);
    void RemoveExclusion(const std::wstring& processName);

    // Startup registry management (HKCU\Software\Microsoft\Windows\CurrentVersion\Run)
    bool IsStartWithWindowsEnabled() const;
    bool SetStartWithWindows(bool enable);

private:
    ConfigManager();
    ~ConfigManager() = default;

    std::wstring GetIniFilePath() const;
    std::wstring GetExecutableDirectory() const;

    Configuration m_config;
};

} // namespace MacTrafficLights
