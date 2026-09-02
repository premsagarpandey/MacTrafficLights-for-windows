#pragma once

#include <windows.h>
#include <shellapi.h>
#include <string>

namespace MacTrafficLights {

class TrayIcon {
public:
    static TrayIcon& Instance();

    bool Initialize(HINSTANCE hInstance);
    void Shutdown();

    void UpdateTooltip(const std::wstring& text);
    void ShowBalloonNotification(const std::wstring& title, const std::wstring& message);

    HWND GetMessageHwnd() const { return m_hMsgWnd; }

private:
    TrayIcon();
    ~TrayIcon();

    static LRESULT CALLBACK MsgWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    void ShowContextMenu();

    HINSTANCE m_hInstance = NULL;
    HWND m_hMsgWnd = NULL;
    NOTIFYICONDATAW m_nid = { 0 };
    bool m_initialized = false;
};

} // namespace MacTrafficLights
