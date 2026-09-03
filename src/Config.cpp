#include "Config.h"
#include <shlobj.h>
#include <sstream>

namespace MacTrafficLights {

static const wchar_t* INI_SECTION_GENERAL = L"General";
static const wchar_t* INI_SECTION_APPEARANCE = L"Appearance";
static const wchar_t* INI_SECTION_EXCLUSIONS = L"Exclusions";

ConfigManager& ConfigManager::Instance() {
    static ConfigManager instance;
    return instance;
}

ConfigManager::ConfigManager() {
    ResetToDefaults();
    Load();
}

void ConfigManager::ResetToDefaults() {
    m_config.enabled = true;
    m_config.buttonSize = 12;
    m_config.buttonSpacing = 8;
    m_config.rightMargin = 14;
    m_config.topMargin = 10;
    m_config.dimWhenInactive = true;
    m_config.excludedProcesses = {
        L"dwm.exe",
        L"shellexperiencehost.exe",
        L"searchhost.exe",
        L"startmenuexperiencehost.exe",
        L"lockapp.exe",
        L"logonui.exe"
    };
}

std::wstring ConfigManager::GetExecutableDirectory() const {
    wchar_t path[MAX_PATH] = { 0 };
    if (GetModuleFileNameW(NULL, path, MAX_PATH) > 0) {
        wchar_t* lastSlash = wcsrchr(path, L'\\');
        if (lastSlash) {
            *lastSlash = L'\0';
            return std::wstring(path);
        }
    }
    return L".";
}

std::wstring ConfigManager::GetIniFilePath() const {
    std::wstring dir = GetExecutableDirectory();
    std::wstring configDir = dir + L"\\config";
    CreateDirectoryW(configDir.c_str(), NULL);
    return configDir + L"\\settings.ini";
}

void ConfigManager::Load() {
    std::wstring iniPath = GetIniFilePath();

    m_config.enabled = (GetPrivateProfileIntW(INI_SECTION_GENERAL, L"Enabled", m_config.enabled ? 1 : 0, iniPath.c_str()) != 0);
    m_config.buttonSize = GetPrivateProfileIntW(INI_SECTION_APPEARANCE, L"ButtonSize", m_config.buttonSize, iniPath.c_str());
    m_config.buttonSpacing = GetPrivateProfileIntW(INI_SECTION_APPEARANCE, L"ButtonSpacing", m_config.buttonSpacing, iniPath.c_str());
    m_config.rightMargin = GetPrivateProfileIntW(INI_SECTION_APPEARANCE, L"RightMargin", m_config.rightMargin, iniPath.c_str());
    m_config.topMargin = GetPrivateProfileIntW(INI_SECTION_APPEARANCE, L"TopMargin", m_config.topMargin, iniPath.c_str());
    m_config.dimWhenInactive = (GetPrivateProfileIntW(INI_SECTION_APPEARANCE, L"DimWhenInactive", m_config.dimWhenInactive ? 1 : 0, iniPath.c_str()) != 0);

    // Bounds checking
    if (m_config.buttonSize < 8) m_config.buttonSize = 8;
    if (m_config.buttonSize > 24) m_config.buttonSize = 24;
    if (m_config.buttonSpacing < 4) m_config.buttonSpacing = 4;
    if (m_config.buttonSpacing > 20) m_config.buttonSpacing = 20;
    if (m_config.rightMargin < 2) m_config.rightMargin = 2;
    if (m_config.rightMargin > 50) m_config.rightMargin = 50;
    if (m_config.topMargin < 2) m_config.topMargin = 2;
    if (m_config.topMargin > 40) m_config.topMargin = 40;

    // Load exclusions
    wchar_t buf[2048] = { 0 };
    GetPrivateProfileStringW(INI_SECTION_EXCLUSIONS, L"ProcessList", L"", buf, 2048, iniPath.c_str());
    if (wcslen(buf) > 0) {
        m_config.excludedProcesses.clear();
        std::wstringstream ss(buf);
        std::wstring item;
        while (std::getline(ss, item, L',')) {
            item.erase(0, item.find_first_not_of(L" \t\r\n"));
            item.erase(item.find_last_not_of(L" \t\r\n") + 1);
            if (!item.empty()) {
                std::transform(item.begin(), item.end(), item.begin(), ::towlower);
                m_config.excludedProcesses.push_back(item);
            }
        }
    }
}

void ConfigManager::Save() {
    std::wstring iniPath = GetIniFilePath();

    WritePrivateProfileStringW(INI_SECTION_GENERAL, L"Enabled", m_config.enabled ? L"1" : L"0", iniPath.c_str());
    WritePrivateProfileStringW(INI_SECTION_APPEARANCE, L"ButtonSize", std::to_wstring(m_config.buttonSize).c_str(), iniPath.c_str());
    WritePrivateProfileStringW(INI_SECTION_APPEARANCE, L"ButtonSpacing", std::to_wstring(m_config.buttonSpacing).c_str(), iniPath.c_str());
    WritePrivateProfileStringW(INI_SECTION_APPEARANCE, L"RightMargin", std::to_wstring(m_config.rightMargin).c_str(), iniPath.c_str());
    WritePrivateProfileStringW(INI_SECTION_APPEARANCE, L"TopMargin", std::to_wstring(m_config.topMargin).c_str(), iniPath.c_str());
    WritePrivateProfileStringW(INI_SECTION_APPEARANCE, L"DimWhenInactive", m_config.dimWhenInactive ? L"1" : L"0", iniPath.c_str());

    std::wstring exclusionsStr;
    for (size_t i = 0; i < m_config.excludedProcesses.size(); ++i) {
        if (i > 0) exclusionsStr += L",";
        exclusionsStr += m_config.excludedProcesses[i];
    }
    WritePrivateProfileStringW(INI_SECTION_EXCLUSIONS, L"ProcessList", exclusionsStr.c_str(), iniPath.c_str());
}

bool ConfigManager::IsProcessExcluded(const std::wstring& processName) const {
    if (processName.empty()) return false;
    std::wstring lower = processName;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);

    for (const auto& excluded : m_config.excludedProcesses) {
        if (lower == excluded) return true;
    }
    return false;
}

void ConfigManager::AddExclusion(const std::wstring& processName) {
    if (processName.empty()) return;
    std::wstring lower = processName;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);

    if (!IsProcessExcluded(lower)) {
        m_config.excludedProcesses.push_back(lower);
        Save();
    }
}

void ConfigManager::RemoveExclusion(const std::wstring& processName) {
    if (processName.empty()) return;
    std::wstring lower = processName;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::towlower);

    auto it = std::remove(m_config.excludedProcesses.begin(), m_config.excludedProcesses.end(), lower);
    if (it != m_config.excludedProcesses.end()) {
        m_config.excludedProcesses.erase(it, m_config.excludedProcesses.end());
        Save();
    }
}

static const wchar_t* RUN_REG_KEY = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
static const wchar_t* APP_REG_NAME = L"MacTrafficLights";

bool ConfigManager::IsAutoStartEnabled() const {
    HKEY hKey = NULL;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, RUN_REG_KEY, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        DWORD type = 0;
        DWORD size = 0;
        LONG result = RegQueryValueExW(hKey, APP_REG_NAME, NULL, &type, NULL, &size);
        RegCloseKey(hKey);
        return (result == ERROR_SUCCESS);
    }
    return false;
}

bool ConfigManager::EnableAutoStart(bool enable) {
    HKEY hKey = NULL;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, RUN_REG_KEY, 0, KEY_WRITE, &hKey) == ERROR_SUCCESS) {
        if (enable) {
            wchar_t exePath[MAX_PATH] = { 0 };
            GetModuleFileNameW(NULL, exePath, MAX_PATH);
            std::wstring command = L"\"" + std::wstring(exePath) + L"\"";
            LONG res = RegSetValueExW(hKey, APP_REG_NAME, 0, REG_SZ, 
                reinterpret_cast<const BYTE*>(command.c_str()), 
                static_cast<DWORD>((command.length() + 1) * sizeof(wchar_t)));
            RegCloseKey(hKey);
            return (res == ERROR_SUCCESS);
        } else {
            RegDeleteValueW(hKey, APP_REG_NAME);
            RegCloseKey(hKey);
            return true;
        }
    }
    return false;
}

} // namespace MacTrafficLights
