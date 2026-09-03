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

    return (RegisterClassExW(&wcex) != 0);
}

void OverlayWindow::UnregisterOverlayClass(HINSTANCE hInstance) {
    UnregisterClassW(OVERLAY_CLASS_NAME, hInstance);
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

    DWORD exStyle = WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE;
    if (GetWindowLongPtrW(m_targetHwnd, GWL_EXSTYLE) & WS_EX_TOPMOST) {
        exStyle |= WS_EX_TOPMOST;
    }
    DWORD style = WS_POPUP;

    UINT dpi = SafeGetDpiForWindow(m_targetHwnd);
    m_lastDpi = dpi;
    int overlayW = MulDiv(142, dpi, 96);
    int overlayH = MulDiv(32, dpi, 96);

    m_hwnd = CreateWindowExW(
        exStyle,
        OVERLAY_CLASS_NAME,
        L"MacTrafficLights_Overlay",
        style,
        0, 0, overlayW, overlayH,
        NULL,
        NULL,
        m_hInstance,
        this
    );

    if (!m_hwnd) {
        LOG_ERROR(L"Failed to create overlay window. Error: " + std::to_wstring(GetLastError()));
        return false;
    }

    SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    UpdatePosition(true);
    ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);

    return true;
}

void OverlayWindow::Destroy() {
    if (m_hwnd) {
        SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, 0);
        DestroyWindow(m_hwnd);
        m_hwnd = NULL;
    }
}

void OverlayWindow::Show(bool show) {
    if (m_hwnd) {
        ShowWindow(m_hwnd, show ? SW_SHOWNOACTIVATE : SW_HIDE);
    }
}

void OverlayWindow::SetTargetActive(bool active) {
    if (m_isTargetActive != active) {
        m_isTargetActive = active;
        m_needsRedraw = true;
        UpdatePosition(true);
    }
}

UINT OverlayWindow::GetTargetDpi() const {
    return SafeGetDpiForWindow(m_targetHwnd);
}

void OverlayWindow::CalculateMetrics(int& buttonSize, int& spacing, int& overlayW, int& overlayH, int& minX, int& maxX, int& closeX, int& btnY) const {
    const auto& config = ConfigManager::Instance().GetConfig();
    UINT dpi = SafeGetDpiForWindow(m_targetHwnd);

    buttonSize = MulDiv(14, dpi, 96);
    if (buttonSize < 10) buttonSize = 10;

    // In Windows 11 / Chrome / VS Code, each native button is ~46px wide at 96 DPI
    int nativeBtnWidth = MulDiv(46, dpi, 96);
    overlayW = 3 * nativeBtnWidth;

    // Dynamically calculate the actual non-client title bar height of the target window
    int titlebarH = 0;
    if (m_targetHwnd && IsWindow(m_targetHwnd)) {
        POINT pt = { 0, 0 };
        ClientToScreen(m_targetHwnd, &pt);
        RECT rcWnd = { 0 };
        GetWindowRect(m_targetHwnd, &rcWnd);
        int computedH = pt.y - rcWnd.top;
        if (computedH >= 24 && computedH <= 90) {
            titlebarH = computedH;
        }
    }

    if (titlebarH > 0) {
        overlayH = titlebarH;
    } else {
        overlayH = MulDiv(38, dpi, 96);
    }

    btnY = (overlayH - buttonSize) / 2;
    if (btnY < 2) btnY = 2;

    // Center each circle inside the exact slot of the original Windows buttons:
    // Slot 1 (Close): rightmost slot
    int closeCenter = overlayW - (nativeBtnWidth / 2);
    closeX = closeCenter - (buttonSize / 2);

    // Slot 2 (Maximize): middle slot
    int maxCenter = overlayW - nativeBtnWidth - (nativeBtnWidth / 2);
    maxX = maxCenter - (buttonSize / 2);

    // Slot 3 (Minimize): leftmost slot
    int minCenter = overlayW - (2 * nativeBtnWidth) - (nativeBtnWidth / 2);
    minX = minCenter - (buttonSize / 2);

    spacing = nativeBtnWidth - buttonSize;
}

COLORREF OverlayWindow::DetectTitleBarColor() const {
    // 1. Check official DWM caption color attribute
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
        if (m_cachedActiveColor != 0) {
            BYTE r = (BYTE)((GetRValue(m_cachedActiveColor) * 3 + 128) / 4);
            BYTE g = (BYTE)((GetGValue(m_cachedActiveColor) * 3 + 128) / 4);
            BYTE b = (BYTE)((GetBValue(m_cachedActiveColor) * 3 + 128) / 4);
            return RGB(r, g, b);
        }
        return darkMode ? RGB(40, 40, 40) : RGB(248, 248, 248);
    }

    // 3. Pixel sampling ONLY if target window is currently the foreground window (not occluded!)
    // Sample directly at the left boundary of our overlay window to ensure seamless blending
    if (m_targetHwnd == GetForegroundWindow()) {
        HDC hdcScreen = GetDC(NULL);
        if (hdcScreen) {
            int buttonSize, spacing, overlayW, overlayH, minX, maxX, closeX, btnY;
            CalculateMetrics(buttonSize, spacing, overlayW, overlayH, minX, maxX, closeX, btnY);

            int sampleX = m_lastTargetRect.right - overlayW - 4;
            int sampleY = m_lastTargetRect.top + (overlayH / 2);

            if (sampleX > m_lastTargetRect.left + 50 && sampleY < m_lastTargetRect.bottom) {
                COLORREF sampled = GetPixel(hdcScreen, sampleX, sampleY);
                if (sampled != CLR_INVALID && sampled != 0 && sampled != RGB(255, 255, 255)) {
                    color = sampled;
                    const_cast<OverlayWindow*>(this)->m_cachedActiveColor = sampled;
                }
            }
            ReleaseDC(NULL, hdcScreen);
        }
    } else if (m_cachedActiveColor != 0) {
        color = m_cachedActiveColor;
    }
    return color;
}

ButtonType OverlayWindow::HitTest(int x, int y) const {
    int buttonSize, spacing, overlayW, overlayH, minX, maxX, closeX, btnY;
    CalculateMetrics(buttonSize, spacing, overlayW, overlayH, minX, maxX, closeX, btnY);

    if (x < 0 || x > overlayW || y < 0 || y > overlayH) {
        return ButtonType::None;
    }

    int nativeBtnWidth = overlayW / 3;

    // Slot 1 (Rightmost): Close (Red)
    if (x >= overlayW - nativeBtnWidth) {
        return ButtonType::Close;
    }

    // Slot 2 (Middle): Maximize (Yellow)
    if (x >= overlayW - 2 * nativeBtnWidth) {
        return ButtonType::Maximize;
    }

    // Slot 3 (Leftmost): Minimize (Green)
    if (x >= overlayW - 3 * nativeBtnWidth) {
        return ButtonType::Minimize;
    }

    return ButtonType::None;
}

void OverlayWindow::DrawButtons(Gdiplus::Graphics& g, int buttonSize, int minX, int maxX, int closeX, int btnY) {
    using namespace Gdiplus;

    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetPixelOffsetMode(PixelOffsetModeHighQuality);

    const auto& config = ConfigManager::Instance().GetConfig();
    bool dim = config.dimWhenInactive && !m_isTargetActive;

    // 🟢 Green colors (for Minimize)
    Color greenFill(255, 39, 201, 63);
    Color greenBorder(255, 26, 171, 41);
    Color greenHover(255, 60, 220, 84);
    Color greenPressed(255, 29, 151, 48);

    // 🟡 Yellow colors (for Maximize)
    Color yellowFill(255, 255, 189, 46);
    Color yellowBorder(255, 222, 161, 35);
    Color yellowHover(255, 255, 204, 82);
    Color yellowPressed(255, 194, 143, 35);

    // 🔴 Red colors (for Close)
    Color redFill(255, 255, 95, 86);
    Color redBorder(255, 224, 68, 62);
    Color redHover(255, 255, 115, 106);
    Color redPressed(255, 191, 72, 66);

    Color inactiveFill(180, 128, 128, 128);
    Color inactiveBorder(200, 100, 100, 100);

    struct ButtonItem {
        ButtonType type;
        int x;
        Color fill;
        Color border;
        Color hover;
        Color pressed;
    };

    ButtonItem items[3] = {
        { ButtonType::Minimize, minX,   greenFill,  greenBorder,  greenHover,  greenPressed },
        { ButtonType::Maximize, maxX,   yellowFill, yellowBorder, yellowHover, yellowPressed },
        { ButtonType::Close,    closeX, redFill,    redBorder,    redHover,    redPressed }
    };

    for (int i = 0; i < 3; ++i) {
        Color currentFill = items[i].fill;
        Color currentBorder = items[i].border;

        bool isHovered = (m_hoverButton == items[i].type);
        bool isPressed = (m_pressedButton == items[i].type && isHovered);

        if (dim && !isHovered && !isPressed) {
            currentFill = inactiveFill;
            currentBorder = inactiveBorder;
        } else if (isPressed) {
            currentFill = items[i].pressed;
        } else if (isHovered) {
            currentFill = items[i].hover;
        }

        SolidBrush brush(currentFill);
        g.FillEllipse(&brush, items[i].x, btnY, buttonSize, buttonSize);

        Pen pen(currentBorder, 1.0f);
        g.DrawEllipse(&pen, (REAL)items[i].x, (REAL)btnY, (REAL)buttonSize, (REAL)buttonSize);
    }
}

void OverlayWindow::Render() {
    if (!m_hwnd) return;

    int buttonSize, spacing, overlayW, overlayH, minX, maxX, closeX, btnY;
    CalculateMetrics(buttonSize, spacing, overlayW, overlayH, minX, maxX, closeX, btnY);

    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = overlayW;
    bmi.bmiHeader.biHeight = overlayH;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pBits = nullptr;
    HBITMAP hBitmap = CreateDIBSection(hdcMem, &bmi, DIB_RGB_COLORS, &pBits, NULL, 0);
    HBITMAP hOldBitmap = (HBITMAP)SelectObject(hdcMem, hBitmap);

    // 1. Fill background with matching titlebar color (fully opaque to cover Windows native buttons)
    COLORREF bgCol = DetectTitleBarColor();
    BYTE r = GetRValue(bgCol);
    BYTE gVal = GetGValue(bgCol);
    BYTE b = GetBValue(bgCol);

    BYTE* pixels = static_cast<BYTE*>(pBits);
    int totalPixels = overlayW * overlayH;
    for (int i = 0; i < totalPixels; ++i) {
        pixels[i * 4 + 0] = b;
        pixels[i * 4 + 1] = gVal;
        pixels[i * 4 + 2] = r;
        pixels[i * 4 + 3] = 255;
    }

    // 2. Draw the 3 circular buttons on top of background
    {
        Gdiplus::Graphics graphics(hdcMem);
        DrawButtons(graphics, buttonSize, minX, maxX, closeX, btnY);
    }

    POINT ptSrc = { 0, 0 };
    SIZE size = { overlayW, overlayH };
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

void OverlayWindow::UpdatePosition(bool forceRender) {
    if (!m_hwnd || !m_targetHwnd || !IsWindow(m_targetHwnd)) return;

    if (IsIconic(m_targetHwnd) || !IsWindowVisible(m_targetHwnd)) {
        ShowWindow(m_hwnd, SW_HIDE);
        return;
    }

    UINT currentDpi = SafeGetDpiForWindow(m_targetHwnd);
    bool dpiChanged = (currentDpi != (UINT)m_lastDpi);
    if (dpiChanged) {
        m_lastDpi = currentDpi;
    }

    RECT targetRect = { 0 };
    if (IsZoomed(m_targetHwnd)) {
        HMONITOR hMon = MonitorFromWindow(m_targetHwnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFO mi = { sizeof(mi) };
        if (hMon && GetMonitorInfoW(hMon, &mi)) {
            targetRect = mi.rcWork;
        } else {
            GetWindowRect(m_targetHwnd, &targetRect);
        }
    } else {
        HRESULT hr = DwmGetWindowAttribute(m_targetHwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &targetRect, sizeof(targetRect));
        if (FAILED(hr) || (targetRect.right - targetRect.left <= 0)) {
            GetWindowRect(m_targetHwnd, &targetRect);
        }
    }
    m_lastTargetRect = targetRect;

    int buttonSize, spacing, overlayW, overlayH, minX, maxX, closeX, btnY;
    CalculateMetrics(buttonSize, spacing, overlayW, overlayH, minX, maxX, closeX, btnY);

    int targetW = targetRect.right - targetRect.left;
    if (overlayW > targetW) {
        overlayW = targetW;
    }

    int overlayX = targetRect.right - overlayW;
    if (overlayX < targetRect.left) {
        overlayX = targetRect.left;
    }
    int overlayY = targetRect.top;

    // Precise Z-Order Placement: Directly above targetHwnd, skipping any existing overlays
    bool isTargetTopmost = (GetWindowLongPtrW(m_targetHwnd, GWL_EXSTYLE) & WS_EX_TOPMOST) != 0;
    HWND insertAfter = HWND_TOP;
    if (isTargetTopmost) {
        insertAfter = HWND_TOPMOST;
    } else if (m_targetHwnd == GetForegroundWindow()) {
        insertAfter = HWND_TOP;
    } else {
        HWND hPrev = GetWindow(m_targetHwnd, GW_HWNDPREV);
        while (hPrev) {
            if (hPrev == m_hwnd) {
                hPrev = GetWindow(hPrev, GW_HWNDPREV);
                continue;
            }
            wchar_t clsName[64] = { 0 };
            if (GetClassNameW(hPrev, clsName, 64) > 0 && wcscmp(clsName, OVERLAY_CLASS_NAME) == 0) {
                hPrev = GetWindow(hPrev, GW_HWNDPREV);
                continue;
            }
            break;
        }
        if (hPrev) {
            insertAfter = hPrev;
        }
    }

    bool sizeChanged = (overlayW != m_lastW || overlayH != m_lastH);
    m_lastW = overlayW;
    m_lastH = overlayH;

    UINT swpFlags = SWP_NOACTIVATE | SWP_SHOWWINDOW;
    if (!sizeChanged) {
        swpFlags |= SWP_NOSIZE;
    }

    SetWindowPos(
        m_hwnd,
        insertAfter,
        overlayX,
        overlayY,
        overlayW,
        overlayH,
        swpFlags
    );

    if (!IsWindowVisible(m_hwnd)) {
        ShowWindow(m_hwnd, SW_SHOWNOACTIVATE);
    }

    // Only redraw bitmap when size, DPI, active state, or explicit render request occurs
    if (forceRender || dpiChanged || sizeChanged || m_needsRedraw) {
        Render();
        m_needsRedraw = false;
    }
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
        // Forward title bar drag when clicked on background outside buttons
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
            case ButtonType::Minimize:
                NativeActions::MinimizeWindow(m_targetHwnd);
                break;
            case ButtonType::Maximize:
                NativeActions::ToggleMaximizeRestore(m_targetHwnd);
                break;
            case ButtonType::Close:
                NativeActions::CloseWindow(m_targetHwnd);
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

        case WM_LBUTTONDBLCLK: {
            ButtonType hit = HitTest(LOWORD(lParam), HIWORD(lParam));
            if (hit == ButtonType::None) {
                NativeActions::ToggleMaximizeRestore(m_targetHwnd);
            } else {
                switch (hit) {
                    case ButtonType::Minimize:
                        NativeActions::MinimizeWindow(m_targetHwnd);
                        break;
                    case ButtonType::Maximize:
                        NativeActions::ToggleMaximizeRestore(m_targetHwnd);
                        break;
                    case ButtonType::Close:
                        NativeActions::CloseWindow(m_targetHwnd);
                        break;
                    default:
                        break;
                }
            }
            return 0;
        }

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

} // namespace MacTrafficLights
