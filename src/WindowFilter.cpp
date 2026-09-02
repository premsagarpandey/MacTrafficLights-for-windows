#include "WindowFilter.h"
#include "Config.h"
#include "Logger.h"
#include <dwmapi.h>
#include <psapi.h>
#include <algorithm>

namespace MacTrafficLights {

bool WindowFilter::IsWindowCloaked(HWND hwnd) {
    if (!hwnd || !IsWindow(hwnd)) return true;

    int cloaked = 0;
    HRESULT hr = DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked));
    if (SUCCEEDED(hr)) {
        return cloaked != 0;
    }
    return false;
}

std::wstring WindowFilter::GetProcessNameForWindow(HWND hwnd) {
    if (!hwnd || !IsWindow(hwnd)) return L"";

    DWORD processId = 0;
    GetWindowThreadProcessId(hwnd, &processId);
    if (processId == 0) return L"";

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId);
    if (!hProcess) return L"";

    wchar_t fullPath[MAX_PATH] = { 0 };
    DWORD size = MAX_PATH;
    std::wstring processName;

    if (QueryFullProcessImageNameW(hProcess, 0, fullPath, &size)) {
        wchar_t* lastSlash = wcsrchr(fullPath, L'\\');
        if (lastSlash) {
            processName = (lastSlash + 1);
        } else {
            processName = fullPath;
        }
    }

    CloseHandle(hProcess);

    std::transform(processName.begin(), processName.end(), processName.begin(), ::towlower);
    return processName;
}

std::wstring WindowFilter::GetClassNameForWindow(HWND hwnd) {
    if (!hwnd || !IsWindow(hwnd)) return L"";
    wchar_t className[256] = { 0 };
    if (GetClassNameW(hwnd, className, 256) > 0) {
        return std::wstring(className);
    }
    return L"";
}

std::wstring WindowFilter::GetWindowTitle(HWND hwnd) {
    if (!hwnd || !IsWindow(hwnd)) return L"";
    int len = GetWindowTextLengthW(hwnd);
    if (len <= 0) return L"";
    std::wstring title;
    title.resize(len + 1);
    GetWindowTextW(hwnd, &title[0], len + 1);
    title.resize(len);
    return title;
}

bool WindowFilter::IsEligibleWindow(HWND hwnd) {
    if (!hwnd || !IsWindow(hwnd)) return false;
    if (hwnd == GetDesktopWindow()) return false;

    // 1. Must be visible
    if (!IsWindowVisible(hwnd)) return false;

    // 2. Ignore minimized windows during eligibility check
    if (IsIconic(hwnd)) return false;

    // 3. Reject windows belonging to our own process
    DWORD processId = 0;
    GetWindowThreadProcessId(hwnd, &processId);
    if (processId == GetCurrentProcessId()) return false;

    // 4. Must not be cloaked by DWM (virtual desktop / suspended UWP)
    if (IsWindowCloaked(hwnd)) return false;

    // 5. Inspect Styles
    LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
    LONG_PTR exStyle = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);

    // Reject child windows
    if (style & WS_CHILD) return false;

    // Reject tool windows (floating toolbars, tooltips, palettes)
    if (exStyle & WS_EX_TOOLWINDOW) return false;

    // Must have a title bar/caption or be a thick-frame popup (e.g. Chrome/VS Code/Edge)
    bool hasCaption = (style & WS_CAPTION) == WS_CAPTION;
    bool hasThickFrame = (style & WS_THICKFRAME) != 0;
    bool isAppWindow = (exStyle & WS_EX_APPWINDOW) != 0;

    if (!hasCaption && !isAppWindow && !hasThickFrame) {
        return false;
    }

    // 6. Check size - reject 0x0 or miniature hidden tracking windows
    RECT rc = { 0 };
    if (!GetWindowRect(hwnd, &rc)) return false;
    int width = rc.right - rc.left;
    int height = rc.bottom - rc.top;
    if (width < 140 || height < 80) return false;

    // 7. Check Class Name against known shell, system, and desktop classes
    std::wstring cls = GetClassNameForWindow(hwnd);

    // Shell & Desktop exclusions
    if (cls == L"Progman" || cls == L"WorkerW") return false; // Desktop
    if (cls == L"Shell_TrayWnd" || cls == L"Shell_SecondaryTrayWnd") return false; // Taskbars
    if (cls == L"Windows.UI.Core.CoreWindow") return false; // Start Menu, Notification Center, Search
    if (cls == L"Xaml_WindowedPopupClass" || cls == L"PopupHost") return false; // Flyouts
    if (cls == L"TopLevelWindowForOverflowXamlIsland") return false; // Overflow islands
    if (cls == L"TaskListThumbnailWnd" || cls == L"TaskListOverlayWnd") return false; // Task view
    if (cls == L"ForegroundStaging") return false;
    if (cls == L"EdgeUiInputTopWndClass") return false;
    if (cls == L"NativeHWNDHost") return false;
    if (cls == L"Button" || cls == L"Static" || cls == L"Edit") return false;

    // 8. Check process name against user configured exclusions
    std::wstring procName = GetProcessNameForWindow(hwnd);
    if (ConfigManager::Instance().IsProcessExcluded(procName)) {
        return false;
    }

    return true;
}

} // namespace MacTrafficLights
