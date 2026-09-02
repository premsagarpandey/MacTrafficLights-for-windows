#pragma once

#include <windows.h>
#include <string>

namespace MacTrafficLights {

class WindowFilter {
public:
    // Determines if a window is an eligible top-level application window
    static bool IsEligibleWindow(HWND hwnd);

    // Checks if a window is cloaked (e.g. suspended UWP, inactive virtual desktop)
    static bool IsWindowCloaked(HWND hwnd);

    // Checks if a window is in full-screen mode (e.g. games, full-screen video, F11)
    static bool IsWindowFullScreen(HWND hwnd);

    // Gets the executable process name for a window (lowercase).g. "notepad.exe")
    static std::wstring GetProcessNameForWindow(HWND hwnd);

    // Retrieves the class name of the window
    static std::wstring GetClassNameForWindow(HWND hwnd);

    // Retrieves the window title
    static std::wstring GetWindowTitle(HWND hwnd);
};

} // namespace MacTrafficLights
