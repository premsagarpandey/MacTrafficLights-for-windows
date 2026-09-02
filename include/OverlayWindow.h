#pragma once

#include <windows.h>
#include <gdiplus.h>
#include <memory>
#include <string>

namespace MacTrafficLights {

enum class ButtonType {
    None = -1,
    Close = 0,    // 🔴 Red
    Minimize = 1, // 🟡 Yellow
    Maximize = 2  // 🟢 Green
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
    HWND GetRightMaskHwnd() const { return m_hRightMaskWnd; }
    HWND GetTargetHwnd() const { return m_targetHwnd; }

    void UpdatePosition();
    void Render();
    void RenderRightMask();
    void SetTargetActive(bool active);
    void Show(bool show);

private:
    static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    static LRESULT CALLBACK WndProcRightMask(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam);
    LRESULT HandleRightMaskMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    void OnMouseMove(int x, int y);
    void OnMouseLeave();
    void OnLButtonDown(int x, int y);
    void OnLButtonUp(int x, int y);

    ButtonType HitTest(int x, int y) const;
    UINT GetTargetDpi() const;
    void CalculateMetrics(int& buttonSize, int& spacing, int& leftMargin, int& topMargin, int& totalW, int& totalH) const;
    void DrawTrafficLightButtons(Gdiplus::Graphics& g, int buttonSize, int spacing, int leftMargin, int topMargin);
    COLORREF DetectTitleBarColor() const;

    HWND m_hwnd = NULL;
    HWND m_hRightMaskWnd = NULL;
    HWND m_targetHwnd = NULL;
    HINSTANCE m_hInstance = NULL;

    bool m_isTargetActive = true;
    bool m_isMouseTracking = false;
    ButtonType m_hoverButton = ButtonType::None;
    ButtonType m_pressedButton = ButtonType::None;

    int m_lastDpi = 96;
    RECT m_lastTargetRect = { 0 };
};

} // namespace MacTrafficLights
