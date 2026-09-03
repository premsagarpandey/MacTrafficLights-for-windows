#include "TrayIcon.h"
#include "Resource.h"
#include "OverlayManager.h"
#include "Config.h"
#include "Logger.h"

namespace MacTrafficLights {

static const wchar_t* TRAY_MSG_CLASS = L"MacTrafficLights_TrayMsgWnd";

TrayIcon& TrayIcon::Instance() {
    static TrayIcon instance;
    return instance;
}

TrayIcon::TrayIcon() {}

TrayIcon::~TrayIcon() {
    Shutdown();
}

bool TrayIcon::Initialize(HINSTANCE hInstance) {
    m_hInstance = hInstance;

    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = TrayIcon::MsgWndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = TRAY_MSG_CLASS;

    RegisterClassExW(&wc);

    m_hMsgWnd = CreateWindowExW(
        WS_EX_TOOLWINDOW, TRAY_MSG_CLASS, L"MacTrafficLights_TrayMsg",
        WS_POPUP, 0, 0, 0, 0,
        NULL, NULL, hInstance, NULL
    );

    if (!m_hMsgWnd) {
        LOG_ERROR(L"Failed to create tray message window");
        return false;
    }

    SetWindowLongPtrW(m_hMsgWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this));

    // Try loading custom icon from resources, fallback to standard application icon
    HICON hIcon = LoadIconW(hInstance, MAKEINTRESOURCEW(IDI_APP_ICON));
    if (!hIcon) {
        hIcon = LoadIconW(NULL, IDI_APPLICATION);
    }

    m_nid.cbSize = sizeof(NOTIFYICONDATAW);
    m_nid.hWnd = m_hMsgWnd;
    m_nid.uID = 1;
    m_nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    m_nid.uCallbackMessage = WM_APP_TRAYMSG;
    m_nid.hIcon = hIcon;
    wcscpy_s(m_nid.szTip, L"MacTrafficLights for Windows");

    if (Shell_NotifyIconW(NIM_ADD, &m_nid)) {
        m_initialized = true;
        LOG_INFO(L"System tray icon initialized successfully");
        return true;
    }

    LOG_WARN(L"Shell_NotifyIconW NIM_ADD returned false, will continue in background mode");
    m_initialized = true;
    return true;
}

void TrayIcon::Shutdown() {
    if (m_initialized) {
        Shell_NotifyIconW(NIM_DELETE, &m_nid);
        m_initialized = false;
    }
    if (m_hMsgWnd) {
        DestroyWindow(m_hMsgWnd);
        m_hMsgWnd = NULL;
    }
}

void TrayIcon::ShowContextMenu() {
    POINT pt;
    GetCursorPos(&pt);

    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) return;

    bool isEnabled = OverlayManager::Instance().IsEnabled();

    // 1. Header item
    AppendMenuW(hMenu, MF_STRING | MF_DISABLED | MF_GRAYED, 0, L"MacTrafficLights");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);

    // 2. Enable / Disable
    AppendMenuW(hMenu, MF_STRING | (isEnabled ? MF_CHECKED : MF_UNCHECKED), IDM_TRAY_ENABLE, L"&Enable");
    AppendMenuW(hMenu, MF_STRING | (!isEnabled ? MF_CHECKED : MF_UNCHECKED), IDM_TRAY_DISABLE, L"&Disable");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);

    // 3. Start with Windows
    bool isAutoStart = ConfigManager::Instance().IsAutoStartEnabled();
    AppendMenuW(hMenu, MF_STRING | (isAutoStart ? MF_CHECKED : MF_UNCHECKED), IDM_TRAY_START_WITH_WINDOWS, L"Start with &Windows");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, NULL);

    // 4. Exit
    AppendMenuW(hMenu, MF_STRING, IDM_TRAY_EXIT, L"E&xit");

    // Required before TrackPopupMenu so clicking outside dismisses the menu
    SetForegroundWindow(m_hMsgWnd);

    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_BOTTOMALIGN | TPM_LEFTALIGN, pt.x, pt.y, 0, m_hMsgWnd, NULL);
    DestroyMenu(hMenu);
}

LRESULT CALLBACK TrayIcon::MsgWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    TrayIcon* pThis = reinterpret_cast<TrayIcon*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));

    switch (msg) {
        case WM_APP_TRAYMSG: {
            if (lParam == WM_RBUTTONUP || lParam == WM_LBUTTONUP) {
                if (pThis) pThis->ShowContextMenu();
            }
            return 0;
        }

        case WM_COMMAND: {
            int wmId = LOWORD(wParam);
            switch (wmId) {
                case IDM_TRAY_ENABLE:
                    OverlayManager::Instance().SetEnabled(true);
                    break;

                case IDM_TRAY_DISABLE:
                    OverlayManager::Instance().SetEnabled(false);
                    break;

                case IDM_TRAY_START_WITH_WINDOWS: {
                    bool current = ConfigManager::Instance().IsAutoStartEnabled();
                    ConfigManager::Instance().EnableAutoStart(!current);
                    break;
                }

                case IDM_TRAY_EXIT:
                    PostQuitMessage(0);
                    break;
            }
            return 0;
        }
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

} // namespace MacTrafficLights
