#pragma once

#include <windows.h>
#include <gdiplus.h>
#include <memory>
#include <string>

namespace MacTrafficLights {

enum class ButtonType {
    None = -1,
    Minimize = 0, // 🟢 Green
    Maximize = 1, // 🟡 Yellow
    Close = 2     // 🔴 Red
};

class OverlayWindow {
public:
    static bool RegisterOverlayClass(HINSTANCE hInstance);
    static void UnregisterOverlayClass(HINSTANCE hInstance);

    OverlayWindow(HWND targetHwnd, HINSTANCE hInstance);
    ~OverlayWindow();

    bool Create();
    void Destroy();

    HWND GetHwnd() const { return m_hwnd; }
    HWND GetTargetHwnd() const { return m_targetHwnd; }

    void UpdatePosition(bool forceRender = false);
    void Render();
    void SetTargetActive(bool active);
    void Show(bool show);

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    void OnMouseMove(int x, int y);
    void OnMouseLeave();
    void OnLButtonDown(int x, int y);
    void OnLButtonUp(int x, int y);

    ButtonType HitTest(int x, int y) const;
    UINT GetTargetDpi() const;
    void CalculateMetrics(int& buttonSize, int& spacing, int& overlayW, int& overlayH, int& minX, int& maxX, int& closeX, int& btnY) const;
    void DrawButtons(Gdiplus::Graphics& g, int buttonSize, int minX, int maxX, int closeX, int btnY);
    COLORREF DetectTitleBarColor() const;

    HWND m_hwnd = NULL;
    HWND m_targetHwnd = NULL;
    HINSTANCE m_hInstance = NULL;

    bool m_isTargetActive = true;
    bool m_isMouseTracking = false;
    bool m_needsRedraw = true;
    ButtonType m_hoverButton = ButtonType::None;
    ButtonType m_pressedButton = ButtonType::None;

    int m_lastDpi = 96;
    int m_lastW = 0;
    int m_lastH = 0;
    COLORREF m_cachedActiveColor = 0;
    RECT m_lastTargetRect = { 0 };
};

} // namespace MacTrafficLights
