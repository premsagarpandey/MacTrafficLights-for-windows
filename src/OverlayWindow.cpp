#include "OverlayWindow.h"
#include "Config.h"
#include "NativeActions.h"
#include "Logger.h"
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "gdiplus.lib")

#ifndef DWMWA_CAPTION_COLOR
#define DWMWA_CAPTION_COLOR 35
#endif

namespace MacTrafficLights {

static const wchar_t* OVERLAY_CLASS_NAME = L"MacTrafficLights_Overlay";
static const wchar_t* RIGHT_MASK_CLASS_NAME = L"MacTrafficLights_RightMask";

typedef UINT (WINAPI *GetDpiForWindowProc)(HWND);

static UINT SafeGetDpiForWindow(HWND hwnd) {
    static GetDpiForWindowProc pGetDpiForWindow = (GetDpiForWindowProc)GetProcAddress(
        GetModuleHandleW(L"user32.dll"), "GetDpiForWindow");
    if (pGetDpiForWindow && hwnd) {
        UINT dpi = pGetDpiForWindow(hwnd);
        if (dpi > 0) return dpi;
    }
    return 96;
}

bool OverlayWindow::RegisterOverlayClass(HINSTANCE hInstance) {
    WNDCLASSEXW wcex = { 0 };
    wcex.cbSize = sizeof(WNDCLASSEXW);
    wcex.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wcex.lpfnWndProc = OverlayWindow::WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = sizeof(OverlayWindow*);
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wcex.hbrBackground = NULL;
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = OVERLAY_CLASS_NAME;

    ATOM a1 = RegisterClassExW(&wcex);

    WNDCLASSEXW wcm = { 0 };
    wcm.cbSize = sizeof(WNDCLASSEXW);
    wcm.style = CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
    wcm.lpfnWndProc = OverlayWindow::WndProcRightMask;
    wcm.cbClsExtra = 0;
    wcm.cbWndExtra = sizeof(OverlayWindow*);
    wcm.hInstance = hInstance;
    wcm.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wcm.hbrBackground = NULL;
    wcm.lpszMenuName = NULL;
    wcm.lpszClassName = RIGHT_MASK_CLASS_NAME;

    ATOM a2 = RegisterClassExW(&wcm);

    return (a1 != 0 && a2 != 0);
}

void OverlayWindow::UnregisterOverlayClass(HINSTANCE hInstance) {
    UnregisterClassW(OVERLAY_CLASS_NAME, hInstance);
    UnregisterClassW(RIGHT_MASK_CLASS_NAME, hInstance);
}

OverlayWindow::OverlayWindow(HWND targetHwnd, HINSTANCE hInstance)
    : m_targetHwnd(targetHwnd), m_hInstance(hInstance) {
    m_lastDpi = GetTargetDpi();
}

OverlayWindow::~OverlayWindow() {
    Destroy();
}

bool OverlayWindow::Create() {
    if (m_hwnd) return true;
    if (!m_targetHwnd || !IsWindow(m_targetHwnd)) return false;

    // Layered, tool window (not in Alt+Tab or taskbar), no-activate (doesn't steal focus)
    // Only inherit WS_EX_TOPMOST if target window is genuinely topmost
    DWORD exStyle = WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE;
    if (GetWindowLongPtrW(m_targetHwnd, GWL_EXSTYLE) & WS_EX_TOPMOST) {
        exStyle |= WS_EX_TOPMOST;
    }
    DWORD style = WS_POPUP;

    // 1. Left traffic lights overlay
    m_hwnd = CreateWindowExW(
        exStyle,
        OVERLAY_CLASS_NAME,
        L"MacTrafficLights_Overlay",
        style,
        0, 0, 100, 30,
        NULL,
        NULL,
        m_hInstance,
        this
    );

    if (!m_hwnd) {
        LOG_ERROR(L"Failed to create left overlay window. Error: " + std::to_wstring(GetLastError()));
        return false;
    }

    SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    // 2. Right mask overlay (covers standard Windows buttons)
    m_hRightMaskWnd = CreateWindowExW(
        exStyle,
        RIGHT_MASK_CLASS_NAME,
        L"MacTrafficLights_RightMask",
        style,
        0, 0, 140, 32,
        NULL,
        NULL,
        m_hInstance,
        this
    );

    if (m_hRightMaskWnd) {
        SetWindowLongPtrW(m_hRightMaskWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
    }

    UpdatePosition();
    Render();
    ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);

    if (m_hRightMaskWnd && ConfigManager::Instance().GetConfig().hideRightButtons) {
        RenderRightMask();
        ShowWindow(m_hRightMaskWnd, SW_SHOWNOACTIVATE);
    }

    return true;
}

void OverlayWindow::Destroy() {
    if (m_hwnd) {
        SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, 0);
        DestroyWindow(m_hwnd);
        m_hwnd = NULL;
    }
    if (m_hRightMaskWnd) {
        SetWindowLongPtrW(m_hRightMaskWnd, GWLP_USERDATA, 0);
        DestroyWindow(m_hRightMaskWnd);
        m_hRightMaskWnd = NULL;
    }
}

void OverlayWindow::Show(bool show) {
    if (m_hwnd) {
        ShowWindow(m_hwnd, show ? SW_SHOWNOACTIVATE : SW_HIDE);
    }
    if (m_hRightMaskWnd) {
        bool hideRight = ConfigManager::Instance().GetConfig().hideRightButtons;
        ShowWindow(m_hRightMaskWnd, (show && hideRight) ? SW_SHOWNOACTIVATE : SW_HIDE);
    }
}

void OverlayWindow::SetTargetActive(bool active) {
    if (m_isTargetActive != active) {
        m_isTargetActive = active;
        Render();
        if (m_hRightMaskWnd) {
            RenderRightMask();
        }
    }
}

UINT OverlayWindow::GetTargetDpi() const {
    return SafeGetDpiForWindow(m_targetHwnd);
}

void OverlayWindow::CalculateMetrics(int& buttonSize, int& spacing, int& leftMargin, int& topMargin, int& totalW, int& totalH) const {
    const auto& config = ConfigManager::Instance().GetConfig();
    UINT dpi = SafeGetDpiForWindow(m_targetHwnd);

    buttonSize = MulDiv(config.buttonSize, dpi, 96);
    spacing = MulDiv(config.buttonSpacing, dpi, 96);
    leftMargin = MulDiv(config.leftMargin, dpi, 96);
    topMargin = MulDiv(config.topMargin, dpi, 96);

    if (buttonSize < 8) buttonSize = 8;
    if (spacing < 2) spacing = 2;

    totalW = leftMargin + (3 * buttonSize) + (2 * spacing) + MulDiv(10, dpi, 96);
    totalH = buttonSize + (2 * topMargin);
}

COLORREF OverlayWindow::DetectTitleBarColor() const {
    // 1. Try official DWM caption color attribute
    COLORREF dwmColor = 0xFFFFFFFF;
    if (SUCCEEDED(DwmGetWindowAttribute(m_targetHwnd, DWMWA_CAPTION_COLOR, &dwmColor, sizeof(dwmColor)))) {
        if (dwmColor != 0xFFFFFFFF && dwmColor != 0xFFFFFFFE && dwmColor != 0) {
            return dwmColor;
        }
    }

    // 2. Base palette depending on dark/light mode and active/inactive state
    BOOL darkMode = FALSE;
    DwmGetWindowAttribute(m_targetHwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &darkMode, sizeof(darkMode));

    COLORREF color = darkMode ? RGB(32, 32, 32) : RGB(243, 243, 243);
    if (!m_isTargetActive) {
        color = darkMode ? RGB(40, 40, 40) : RGB(248, 248, 248);
    }

    // 3. Pixel sampling: sample to the left of the right buttons to avoid window title text
    HDC hdcScreen = GetDC(NULL);
    if (hdcScreen) {
        UINT dpi = SafeGetDpiForWindow(m_targetHwnd);
        int sampleX = m_lastTargetRect.right - MulDiv(155, dpi, 96);
        int sampleY = m_lastTargetRect.top + MulDiv(14, dpi, 96);
        if (IsZoomed(m_targetHwnd)) {
            sampleX -= GetSystemMetrics(SM_CXSIZEFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
            sampleY += GetSystemMetrics(SM_CYSIZEFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
        }
        if (sampleX > m_lastTargetRect.left + 50) {
            COLORREF sampled = GetPixel(hdcScreen, sampleX, sampleY);
            if (sampled != CLR_INVALID && sampled != 0 && sampled != RGB(255, 255, 255)) {
                color = sampled;
            }
        }
        ReleaseDC(NULL, hdcScreen);
    }
    return color;
}

void OverlayWindow::RenderRightMask() {
    if (!m_hRightMaskWnd) return;

    UINT dpi = SafeGetDpiForWindow(m_targetHwnd);
    int rightW = MulDiv(142, dpi, 96);
    int rightH = MulDiv(34, dpi, 96);

    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = rightW;
    bmi.bmiHeader.biHeight = rightH;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pBits = nullptr;
    HBITMAP hBitmap = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

    COLORREF bgCol = DetectTitleBarColor();
    BYTE r = GetRValue(bgCol);
    BYTE g = GetGValue(bgCol);
    BYTE b = GetBValue(bgCol);

    BYTE* pixels = static_cast<BYTE*>(pBits);
    int totalPixels = rightW * rightH;
    for (int i = 0; i < totalPixels; ++i) {
        pixels[i * 4 + 0] = b;
        pixels[i * 4 + 1] = g;
        pixels[i * 4 + 2] = r;
        pixels[i * 4 + 3] = 255; // Fully opaque mask
    }

    POINT ptSrc = { 0, 0 };
    SIZE size = { rightW, rightH };
    BLENDFUNCTION blend = { 0 };
    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA;

    POINT ptDst = { 0, 0 };
    RECT rc;
    GetWindowRect(m_hRightMaskWnd, &rc);
    ptDst.x = rc.left;
    ptDst.y = rc.top;

    UpdateLayeredWindow(m_hRightMaskWnd, hdcScreen, &ptDst, &size, hdcMem, &ptSrc, 0, &blend, ULW_ALPHA);

    SelectObject(hdcMem, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
}

void OverlayWindow::UpdatePosition() {
    if (!m_hwnd || !m_targetHwnd || !IsWindow(m_targetHwnd)) return;

    if (IsIconic(m_targetHwnd) || !IsWindowVisible(m_targetHwnd)) {
        ShowWindow(m_hwnd, SW_HIDE);
        if (m_hRightMaskWnd) ShowWindow(m_hRightMaskWnd, SW_HIDE);
        return;
    }

    RECT targetRect = { 0 };
    HRESULT hr = DwmGetWindowAttribute(m_targetHwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &targetRect, sizeof(targetRect));
    if (FAILED(hr) || (targetRect.right - targetRect.left <= 0)) {
        GetWindowRect(m_targetHwnd, &targetRect);
    }
    m_lastTargetRect = targetRect;

    int buttonSize, spacing, leftMargin, topMargin, totalW, totalH;
    CalculateMetrics(buttonSize, spacing, leftMargin, topMargin, totalW, totalH);

    int overlayX = targetRect.left + leftMargin;
    int overlayY = targetRect.top + topMargin;

    int cxBorder = 0;
    int cyBorder = 0;
    if (IsZoomed(m_targetHwnd)) {
        cxBorder = GetSystemMetrics(SM_CXSIZEFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
        cyBorder = GetSystemMetrics(SM_CYSIZEFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
        overlayX += cxBorder;
        overlayY += cyBorder;
    }

    // Precise Z-Order Placement: Attach directly above targetHwnd
    bool isTargetTopmost = (GetWindowLongPtrW(m_targetHwnd, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;
    HWND insertAfter = HWND_TOP;
    if (isTargetTopmost) {
        insertAfter = HWND_TOPMOST;
    } else if (m_targetHwnd != GetForegroundWindow()) {
        HWND hPrev = GetWindow(m_targetHwnd, GW_HWNDPREV);
        while (hPrev && (hPrev == m_hwnd || hPrev == m_hRightMaskWnd)) {
            hPrev = GetWindow(hPrev, GW_HWNDPREV);
        }
        if (hPrev) {
            insertAfter = hPrev;
        }
    }

    // 1. Position Left Traffic Lights
    SetWindowPos(
        m_hwnd,
        insertAfter,
        overlayX,
        overlayY,
        totalW,
        totalH,
        SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_SHOWWINDOW
    );

    // 2. Position Right Mask Overlay (if enabled)
    const auto& config = ConfigManager::Instance().GetConfig();
    if (config.hideRightButtons) {
        if (!m_hRightMaskWnd) {
            DWORD exStyle = WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE;
            if (isTargetTopmost) exStyle |= WS_EX_TOPMOST;
            m_hRightMaskWnd = CreateWindowExW(
                exStyle, RIGHT_MASK_CLASS_NAME, L"MacTrafficLights_RightMask",
                WS_POPUP, 0, 0, 140, 32, NULL, NULL, m_hInstance, this);
            if (m_hRightMaskWnd) {
                SetWindowLongPtrW(m_hRightMaskWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));
            }
        }

        if (m_hRightMaskWnd) {
            UINT dpi = SafeGetDpiForWindow(m_targetHwnd);
            int rightW = MulDiv(142, dpi, 96);
            int rightH = MulDiv(34, dpi, 96);

            int rightX = targetRect.right - rightW;
            int rightY = targetRect.top;

            if (IsZoomed(m_targetHwnd)) {
                rightX -= cxBorder;
                rightY += cyBorder;
            }

            SetWindowPos(
                m_hRightMaskWnd,
                m_hwnd,
                rightX,
                rightY,
                rightW,
                rightH,
                SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_SHOWWINDOW
            );
            RenderRightMask();
        }
    } else if (m_hRightMaskWnd) {
        ShowWindow(m_hRightMaskWnd, SW_HIDE);
    }
}

ButtonType OverlayWindow::HitTest(int x, int y) const {
    int buttonSize, spacing, leftMargin, topMargin, totalW, totalH;
    CalculateMetrics(buttonSize, spacing, leftMargin, topMargin, totalW, totalH);

    int radius = buttonSize / 2;
    int centerY = topMargin + radius;

    for (int i = 0; i < 3; ++i) {
        int centerX = leftMargin + (i * (buttonSize + spacing)) + radius;
        int dx = x - centerX;
        int dy = y - centerY;

        if ((dx * dx + dy * dy) <= ((radius + 1) * (radius + 1))) {
            return static_cast<ButtonType>(i);
        }
    }

    return ButtonType::None;
}

void OverlayWindow::DrawTrafficLightButtons(Gdiplus::Graphics& g, int buttonSize, int spacing, int leftMargin, int topMargin) {
    using namespace Gdiplus;

    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetPixelOffsetMode(PixelOffsetModeHighQuality);

    const auto& config = ConfigManager::Instance().GetConfig();
    bool dim = config.dimWhenInactive && !m_isTargetActive;

    Color redFill(255, 255, 95, 86);
    Color redBorder(255, 224, 68, 62);
    Color redHover(255, 255, 115, 106);
    Color redPressed(255, 191, 72, 66);

    Color yellowFill(255, 255, 189, 46);
    Color yellowBorder(255, 222, 161, 35);
    Color yellowHover(255, 255, 204, 82);
    Color yellowPressed(255, 194, 143, 35);

    Color greenFill(255, 39, 201, 63);
    Color greenBorder(255, 26, 171, 41);
    Color greenHover(255, 60, 220, 84);
    Color greenPressed(255, 29, 151, 48);

    Color inactiveFill(180, 128, 128, 128);
    Color inactiveBorder(200, 100, 100, 100);

    struct ButtonDef {
        ButtonType type;
        Color fill;
        Color border;
        Color hover;
        Color pressed;
    };

    ButtonDef buttons[3] = {
        { ButtonType::Close,    redFill,    redBorder,    redHover,    redPressed },
        { ButtonType::Minimize, yellowFill, yellowBorder, yellowHover, yellowPressed },
        { ButtonType::Maximize, greenFill,  greenBorder,  greenHover,  greenPressed }
    };

    for (int i = 0; i < 3; ++i) {
        int btnX = leftMargin + (i * (buttonSize + spacing));
        int btnY = topMargin;

        Color currentFill = buttons[i].fill;
        Color currentBorder = buttons[i].border;

        if (dim && m_hoverButton != buttons[i].type) {
            currentFill = inactiveFill;
            currentBorder = inactiveBorder;
        } else if (m_pressedButton == buttons[i].type) {
            currentFill = buttons[i].pressed;
        } else if (m_hoverButton == buttons[i].type) {
            currentFill = buttons[i].hover;
        }

        SolidBrush brush(currentFill);
        g.FillEllipse(&brush, btnX, btnY, buttonSize, buttonSize);

        Pen pen(currentBorder, 1.0f);
        g.DrawEllipse(&pen, (REAL)btnX, (REAL)btnY, (REAL)buttonSize, (REAL)buttonSize);

        if (config.showHoverSymbols && (m_hoverButton != ButtonType::None)) {
            REAL centerX = (REAL)btnX + (REAL)buttonSize / 2.0f;
            REAL centerY = (REAL)btnY + (REAL)buttonSize / 2.0f;
            REAL symHalf = (REAL)buttonSize * 0.22f;

            if (buttons[i].type == ButtonType::Close) {
                Pen symPen(Color(220, 77, 0, 0), 1.2f);
                g.DrawLine(&symPen, centerX - symHalf, centerY - symHalf, centerX + symHalf, centerY + symHalf);
                g.DrawLine(&symPen, centerX + symHalf, centerY - symHalf, centerX - symHalf, centerY + symHalf);
            } else if (buttons[i].type == ButtonType::Minimize) {
                Pen symPen(Color(220, 102, 68, 0), 1.4f);
                g.DrawLine(&symPen, centerX - symHalf, centerY, centerX + symHalf, centerY);
            } else if (buttons[i].type == ButtonType::Maximize) {
                Pen symPen(Color(220, 0, 77, 0), 1.2f);
                if (NativeActions::IsWindowMaximized(m_targetHwnd)) {
                    g.DrawLine(&symPen, centerX - symHalf, centerY - symHalf, centerX + symHalf, centerY + symHalf);
                } else {
                    g.DrawLine(&symPen, centerX - symHalf, centerY, centerX + symHalf, centerY);
                    g.DrawLine(&symPen, centerX, centerY - symHalf, centerX, centerY + symHalf);
                }
            }
        }
    }
}

void OverlayWindow::Render() {
    if (!m_hwnd) return;

    int buttonSize, spacing, leftMargin, topMargin, totalW, totalH;
    CalculateMetrics(buttonSize, spacing, leftMargin, topMargin, totalW, totalH);

    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = totalW;
    bmi.bmiHeader.biHeight = totalH;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pBits = nullptr;
    HBITMAP hBitmap = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

    {
        Gdiplus::Graphics graphics(hdcMem);
        graphics.Clear(Gdiplus::Color(0, 0, 0, 0));
        DrawTrafficLightButtons(graphics, buttonSize, spacing, leftMargin, topMargin);
    }

    BYTE* pixels = static_cast<BYTE*>(pBits);
    int totalPixels = totalW * totalH;
    for (int i = 0; i < totalPixels; ++i) {
        BYTE a = pixels[i * 4 + 3];
        if (a == 0) {
            pixels[i * 4 + 0] = 0;
            pixels[i * 4 + 1] = 0;
            pixels[i * 4 + 2] = 0;
        } else if (a < 255) {
            pixels[i * 4 + 0] = (pixels[i * 4 + 0] * a) / 255;
            pixels[i * 4 + 1] = (pixels[i * 4 + 1] * a) / 255;
            pixels[i * 4 + 2] = (pixels[i * 4 + 2] * a) / 255;
        }
    }

    POINT ptSrc = { 0, 0 };
    SIZE size = { totalW, totalH };
    BLENDFUNCTION blend = { 0 };
    blend.BlendOp = AC_SRC_OVER;
    blend.BlendFlags = 0;
    blend.SourceConstantAlpha = 255;
    blend.AlphaFormat = AC_SRC_ALPHA;

    POINT ptDst = { 0, 0 };
    RECT rc;
    GetWindowRect(m_hwnd, &rc);
    ptDst.x = rc.left;
    ptDst.y = rc.top;

    UpdateLayeredWindow(m_hwnd, hdcScreen, &ptDst, &size, hdcMem, &ptSrc, 0, &blend, ULW_ALPHA);

    SelectObject(hdcMem, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);
}

void OverlayWindow::OnMouseMove(int x, int y) {
    if (!m_isMouseTracking) {
        TRACKMOUSEEVENT tme = { 0 };
        tme.cbSize = sizeof(TRACKMOUSEEVENT);
        tme.dwFlags = TME_LEAVE;
        tme.hwndTrack = m_hwnd;
        TrackMouseEvent(&tme);
        m_isMouseTracking = true;
    }

    ButtonType hit = HitTest(x, y);
    if (hit != m_hoverButton) {
        m_hoverButton = hit;
        Render();
    }
}

void OverlayWindow::OnMouseLeave() {
    m_isMouseTracking = false;
    if (m_hoverButton != ButtonType::None || m_pressedButton != ButtonType::None) {
        m_hoverButton = ButtonType::None;
        m_pressedButton = ButtonType::None;
        Render();
    }
}

void OverlayWindow::OnLButtonDown(int x, int y) {
    ButtonType hit = HitTest(x, y);
    if (hit != ButtonType::None) {
        m_pressedButton = hit;
        SetCapture(m_hwnd);
        Render();
    } else {
        POINT pt = { x, y };
        ClientToScreen(m_hwnd, &pt);
        POINTS pts = { (SHORT)pt.x, (SHORT)pt.y };
        NativeActions::ForwardTitleBarDrag(m_targetHwnd, pts);
    }
}

void OverlayWindow::OnLButtonUp(int x, int y) {
    if (GetCapture() == m_hwnd) {
        ReleaseCapture();
    }

    ButtonType hit = HitTest(x, y);
    ButtonType wasPressed = m_pressedButton;
    m_pressedButton = ButtonType::None;
    Render();

    if (hit != ButtonType::None && hit == wasPressed) {
        switch (hit) {
            case ButtonType::Close:
                NativeActions::CloseWindow(m_targetHwnd);
                break;
            case ButtonType::Minimize:
                NativeActions::MinimizeWindow(m_targetHwnd);
                break;
            case ButtonType::Maximize:
                NativeActions::ToggleMaximizeRestore(m_targetHwnd);
                break;
            default:
                break;
        }
    }
}

LRESULT CALLBACK OverlayWindow::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    OverlayWindow* pThis = reinterpret_cast<OverlayWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (pThis) {
        return pThis->HandleMessage(msg, wParam, lParam);
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT OverlayWindow::HandleMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_MOUSEMOVE:
            OnMouseMove(LOWORD(lParam), HIWORD(lParam));
            return 0;

        case WM_MOUSELEAVE:
            OnMouseLeave();
            return 0;

        case WM_LBUTTONDOWN:
            OnLButtonDown(LOWORD(lParam), HIWORD(lParam));
            return 0;

        case WM_LBUTTONDBLCLK:
            NativeActions::ToggleMaximizeRestore(m_targetHwnd);
            return 0;

        case WM_RBUTTONUP: {
            POINT pt = { LOWORD(lParam), HIWORD(lParam) };
            ClientToScreen(m_hwnd, &pt);
            SendMessageW(m_targetHwnd, WM_CONTEXTMENU, (WPARAM)m_targetHwnd, MAKELPARAM(pt.x, pt.y));
            return 0;
        }

        case WM_LBUTTONUP:
            OnLButtonUp(LOWORD(lParam), HIWORD(lParam));
            return 0;

        case WM_NCHITTEST:
            return HTCLIENT;

        case WM_SETCURSOR:
            if (m_hoverButton != ButtonType::None) {
                SetCursor(LoadCursorW(NULL, IDC_HAND));
                return TRUE;
            }
            SetCursor(LoadCursorW(NULL, IDC_ARROW));
            return TRUE;

        case WM_DESTROY:
            m_hwnd = NULL;
            return 0;
    }
    return DefWindowProcW(m_hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK OverlayWindow::WndProcRightMask(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    OverlayWindow* pThis = reinterpret_cast<OverlayWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    if (pThis) {
        return pThis->HandleRightMaskMessage(msg, wParam, lParam);
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

LRESULT OverlayWindow::HandleRightMaskMessage(UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_LBUTTONDOWN: {
            // Forward title bar drag when clicked on right mask
            POINT pt = { LOWORD(lParam), HIWORD(lParam) };
            ClientToScreen(m_hRightMaskWnd, &pt);
            POINTS pts = { (SHORT)pt.x, (SHORT)pt.y };
            NativeActions::ForwardTitleBarDrag(m_targetHwnd, pts);
            return 0;
        }

        case WM_LBUTTONDBLCLK: {
            // Double clicking title bar area maximizes / restores window
            NativeActions::ToggleMaximizeRestore(m_targetHwnd);
            return 0;
        }

        case WM_RBUTTONUP: {
            // Right-clicking title bar area opens system window menu
            POINT pt = { LOWORD(lParam), HIWORD(lParam) };
            ClientToScreen(m_hRightMaskWnd, &pt);
            SendMessageW(m_targetHwnd, WM_CONTEXTMENU, (WPARAM)m_targetHwnd, MAKELPARAM(pt.x, pt.y));
            return 0;
        }

        case WM_SETCURSOR:
            SetCursor(LoadCursorW(NULL, IDC_ARROW));
            return TRUE;

        case WM_NCHITTEST:
            return HTCLIENT;

        case WM_DESTROY:
            m_hRightMaskWnd = NULL;
            return 0;
    }
    return DefWindowProcW(m_hRightMaskWnd, msg, wParam, lParam);
}

} // namespace MacTrafficLights
