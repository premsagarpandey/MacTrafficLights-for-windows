#pragma once

#include <windows.h>

namespace MacTrafficLights {

class NativeActions {
public:
    // 🔴 Red button: Send standard native Windows close command
    static bool CloseWindow(HWND targetHwnd);

    // 🟡 Yellow button: Send standard native Windows minimize command
    static bool MinimizeWindow(HWND targetHwnd);

    // 🟢 Green button: Toggle between maximize and restore
    static bool ToggleMaximizeRestore(HWND targetHwnd);

    // Check if target window is currently maximized
    static bool IsWindowMaximized(HWND targetHwnd);

    // Forward title bar drag/interaction to target window
    static void ForwardTitleBarDrag(HWND targetHwnd, POINTS screenPt);
};

} // namespace MacTrafficLights
