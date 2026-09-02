#pragma once

#include <windows.h>

namespace MacTrafficLights {

class SettingsDialog {
public:
    static void Show(HINSTANCE hInstance, HWND hParent);
    static bool IsOpen() { return s_hwnd != NULL; }

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static void InitializeControls(HWND hwnd);
    static void SaveFromControls(HWND hwnd);
    static void ResetDefaults(HWND hwnd);

    static HWND s_hwnd;
};

} // namespace MacTrafficLights
