#include "OverlayWindow.h"
#include "Config.h"
#include "NativeActions.h"
#include "Logger.h"
#include <dwmapi.h>

#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "gdiplus.lib")

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
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = OverlayWindow::WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = sizeof(OverlayWindow*);
    wcex.hInstance = hInstance;
    wcex.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wcex.hbrBackground = NULL; // Transparent
    wcex.lpszMenuName = NULL;
    wcex.lpszClassName = OVERLAY_CLASS_NAME;

    ATOM atom = RegisterClassExW(&wcex);
    return atom != 0;
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

    // Layered, tool window (not in Alt+Tab or taskbar), no-activate (doesn't steal focus)
    DWORD exStyle = WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_TOPMOST;
    DWORD style = WS_POPUP;

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
        LOG_ERROR(L"Failed to create overlay window. Error: " + std::to_wstring(GetLastError()));
        return false;
    }

    SetWindowLongPtrW(m_hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    UpdatePosition();
    Render();
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
        Render();
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

    // Ensure sensible minimums
    if (buttonSize < 8) buttonSize = 8;
    if (spacing < 2) spacing = 2;

    totalW = leftMargin + (3 * buttonSize) + (2 * spacing) + MulDiv(10, dpi, 96);
    totalH = buttonSize + (2 * topMargin);
}

void OverlayWindow::UpdatePosition() {
    if (!m_hwnd || !m_targetHwnd || !IsWindow(m_targetHwnd)) return;

    if (IsIconic(m_targetHwnd) || !IsWindowVisible(m_targetHwnd)) {
        ShowWindow(m_hwnd, SW_HIDE);
        return;
    }

    RECT targetRect = { 0 };
    HRESULT hr = DwmGetWindowAttribute(m_targetHwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &targetRect, sizeof(targetRect));
    if (FAILED(hr) || (targetRect.right - targetRect.left <= 0)) {
        GetWindowRect(m_targetHwnd, &targetRect);
    }

    int buttonSize, spacing, leftMargin, topMargin, totalW, totalH;
    CalculateMetrics(buttonSize, spacing, leftMargin, topMargin, totalW, totalH);

    int overlayX = targetRect.left + leftMargin;
    int overlayY = targetRect.top + topMargin;

    // Adjust for Windows 11 maximized window border overflow
    if (IsZoomed(m_targetHwnd)) {
        int cxBorder = GetSystemMetrics(SM_CXSIZEFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);
        int cyBorder = GetSystemMetrics(SM_CYSIZEFRAME) + GetSystemMetrics(SM_CXPADDEDBORDER);

        // When maximized, targetRect starts off-screen (e.g. -8, -8), so compensate:
        overlayX += cxBorder;
        overlayY += cyBorder;
    }

    // Position overlay immediately above target window without stealing focus
    SetWindowPos(
        m_hwnd,
        HWND_TOP,
        overlayX,
        overlayY,
        totalW,
        totalH,
        SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_SHOWWINDOW
    );

    m_lastTargetRect = targetRect;
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

        // Circular hit-test: dx^2 + dy^2 <= (radius + 1)^2
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

    // Colors matching clean macOS aesthetic
    // Red: #FF5F56, Border: #E0443E
    // Yellow: #FFBD2E, Border: #DEA123
    // Green: #27C93F, Border: #1AAB29
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

    // Dimmed inactive colors (subtle translucent/neutral)
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

        // 1. Draw button circular body
        SolidBrush brush(currentFill);
        g.FillEllipse(&brush, btnX, btnY, buttonSize, buttonSize);

        // 2. Draw border
        Pen pen(currentBorder, 1.0f);
        g.DrawEllipse(&pen, (REAL)btnX, (REAL)btnY, (REAL)buttonSize, (REAL)buttonSize);

        // 3. Draw subtle symbols on hover
        if (config.showHoverSymbols && (m_hoverButton != ButtonType::None)) {
            REAL centerX = (REAL)btnX + (REAL)buttonSize / 2.0f;
            REAL centerY = (REAL)btnY + (REAL)buttonSize / 2.0f;
            REAL symHalf = (REAL)buttonSize * 0.22f;

            if (buttons[i].type == ButtonType::Close) {
                // 🔴 'x' symbol
                Pen symPen(Color(220, 77, 0, 0), 1.2f);
                g.DrawLine(&symPen, centerX - symHalf, centerY - symHalf, centerX + symHalf, centerY + symHalf);
                g.DrawLine(&symPen, centerX + symHalf, centerY - symHalf, centerX - symHalf, centerY + symHalf);
            } else if (buttons[i].type == ButtonType::Minimize) {
                // 🟡 '–' symbol
                Pen symPen(Color(220, 102, 68, 0), 1.4f);
                g.DrawLine(&symPen, centerX - symHalf, centerY, centerX + symHalf, centerY);
            } else if (buttons[i].type == ButtonType::Maximize) {
                // 🟢 '+' symbol or diagonal arrows
                Pen symPen(Color(220, 0, 77, 0), 1.2f);
                if (NativeActions::IsWindowMaximized(m_targetHwnd)) {
                    // Restore symbol: two opposite diagonal triangles
                    g.DrawLine(&symPen, centerX - symHalf, centerY - symHalf, centerX + symHalf, centerY + symHalf);
                } else {
                    // Maximize symbol: '+'
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

    // Render using GDI+
    {
        Gdiplus::Graphics graphics(hdcMem);
        graphics.Clear(Gdiplus::Color(0, 0, 0, 0)); // Pure transparent background
        DrawTrafficLightButtons(graphics, buttonSize, spacing, leftMargin, topMargin);
    }

    // Apply 32-bit premultiplied alpha for UpdateLayeredWindow
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
        // Clicked outside buttons: Forward title bar drag to target window
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
