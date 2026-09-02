#include "NativeActions.h"
#include "Logger.h"

namespace MacTrafficLights {

bool NativeActions::CloseWindow(HWND targetHwnd) {
    if (!targetHwnd || !IsWindow(targetHwnd)) {
        LOG_WARN(L"CloseWindow called on invalid HWND");
        return false;
    }

    LOG_INFO(L"Executing Close action on target HWND: " + std::to_wstring((uintptr_t)targetHwnd));

    // Prefer WM_SYSCOMMAND SC_CLOSE as it properly triggers app cleanup,
    // unsaved document prompts, and standard Windows close behavior.
    if (PostMessageW(targetHwnd, WM_SYSCOMMAND, SC_CLOSE, 0)) {
        return true;
    }

    // Fallback to WM_CLOSE
    return PostMessageW(targetHwnd, WM_CLOSE, 0, 0) != FALSE;
}

bool NativeActions::MinimizeWindow(HWND targetHwnd) {
    if (!targetHwnd || !IsWindow(targetHwnd)) {
        LOG_WARN(L"MinimizeWindow called on invalid HWND");
        return false;
    }

    LOG_INFO(L"Executing Minimize action on target HWND: " + std::to_wstring((uintptr_t)targetHwnd));

    // Check if the window has a minimize box or system menu command
    LONG_PTR style = GetWindowLongPtrW(targetHwnd, GWL_STYLE);
    if ((style & WS_MINIMIZEBOX) || (style & WS_SYSMENU)) {
        if (PostMessageW(targetHwnd, WM_SYSCOMMAND, SC_MINIMIZE, 0)) {
            return true;
        }
    }

    // Direct ShowWindow fallback
    return ShowWindow(targetHwnd, SW_MINIMIZE) != FALSE;
}

bool NativeActions::ToggleMaximizeRestore(HWND targetHwnd) {
    if (!targetHwnd || !IsWindow(targetHwnd)) {
        LOG_WARN(L"ToggleMaximizeRestore called on invalid HWND");
        return false;
    }

    bool isMaximized = IsWindowMaximized(targetHwnd);
    LOG_INFO(L"Executing Maximize/Restore action (currently " + 
             std::wstring(isMaximized ? L"maximized" : L"normal") + 
             L") on target HWND: " + std::to_wstring((uintptr_t)targetHwnd));

    if (isMaximized) {
        if (PostMessageW(targetHwnd, WM_SYSCOMMAND, SC_RESTORE, 0)) {
            return true;
        }
        return ShowWindow(targetHwnd, SW_RESTORE) != FALSE;
    } else {
        if (PostMessageW(targetHwnd, WM_SYSCOMMAND, SC_MAXIMIZE, 0)) {
            return true;
        }
        return ShowWindow(targetHwnd, SW_MAXIMIZE) != FALSE;
    }
}

bool NativeActions::IsWindowMaximized(HWND targetHwnd) {
    if (!targetHwnd || !IsWindow(targetHwnd)) {
        return false;
    }
    return IsZoomed(targetHwnd) != FALSE;
}

void NativeActions::ForwardTitleBarDrag(HWND targetHwnd, POINTS screenPt) {
    if (!targetHwnd || !IsWindow(targetHwnd)) return;

    // Activate the target window
    SetForegroundWindow(targetHwnd);

    // Forward non-client button down to allow standard window dragging
    LPARAM lParam = MAKELPARAM(screenPt.x, screenPt.y);
    SendMessageW(targetHwnd, WM_NCLBUTTONDOWN, HTCAPTION, lParam);
}

} // namespace MacTrafficLights
