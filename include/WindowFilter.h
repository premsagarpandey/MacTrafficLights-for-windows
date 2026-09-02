#pragma once

#include <windows.h>
#include <string>

namespace MacTrafficLights {

class WindowFilter {
public:
    // Determines if a window is an eligible top-level application window
    static bool IsEligibleWindow(HWND hwnd);

    // Determines if a window is cloaked by DWM (e.g. other virtual desktops or suspended UWP)
    static bool IsWindowCloaked(HWND hwnd);

    // Retrieves the base executable name for a given window (e.g. "notepad.exe")
    static std::wstring GetProcessNameForWindow(HWND hwnd);

    // Retrieves the class name of the window
    static std::wstring GetClassNameForWindow(HWND hwnd);

    // Retrieves the window title
    static std::wstring GetWindowTitle(HWND hwnd);
};

} // namespace MacTrafficLights
