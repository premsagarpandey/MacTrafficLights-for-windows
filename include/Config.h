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
    int rightMargin = 14;        // Offset from target window right border at 96 DPI
    int topMargin = 10;          // Offset from target window top border at 96 DPI
    bool dimWhenInactive = true; // Subtle grey/desaturated when target window is inactive
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

    bool EnableAutoStart(bool enable);
    bool IsAutoStartEnabled() const;

private:
    ConfigManager();
    ~ConfigManager() = default;

    std::wstring GetIniFilePath() const;
    std::wstring GetExecutableDirectory() const;

    Configuration m_config;
};

} // namespace MacTrafficLights
