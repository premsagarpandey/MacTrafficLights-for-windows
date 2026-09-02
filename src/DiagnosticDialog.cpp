#include "DiagnosticDialog.h"
#include "Diagnostic.h"
#include "OverlayManager.h"
#include <iomanip>
#include <sstream>

namespace MacTrafficLights {

HWND DiagnosticDialog::s_hwnd = NULL;
static const wchar_t* DIAG_CLASS = L"MacTrafficLights_DiagWindow";

enum DiagCtrlId {
    ID_LBL_CPU = 201,
    ID_LBL_RAM_WS,
    ID_LBL_RAM_PRIV,
    ID_LBL_TRACKED,
    ID_LBL_OVERLAYS,
    ID_LBL_SAFETY,
    ID_BTN_CLOSE_DIAG
};

void DiagnosticDialog::Show(HINSTANCE hInstance, HWND hParent) {
    if (s_hwnd && IsWindow(s_hwnd)) {
        SetForegroundWindow(s_hwnd);
        return;
    }

    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = DiagnosticDialog::WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = DIAG_CLASS;

    RegisterClassExW(&wc);

    int width = 420;
    int height = 360;
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int x = (screenW - width) / 2;
    int y = (screenH - height) / 2;

    s_hwnd = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        DIAG_CLASS,
        L"MacTrafficLights Diagnostics",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        x, y, width, height,
        hParent, NULL, hInstance, NULL
    );

    InitializeControls(s_hwnd);
    UpdateDisplay(s_hwnd);
    SetTimer(s_hwnd, 1, 1000, NULL);

    ShowWindow(s_hwnd, SW_SHOW);
    UpdateWindow(s_hwnd);
}

void DiagnosticDialog::InitializeControls(HWND hwnd) {
    HINSTANCE hInst = (HINSTANCE)GetWindowLongPtrW(hwnd, GWLP_HINSTANCE);
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    auto AddLabel = [&](const wchar_t* text, int x, int y, int w, int h, int id = -1) -> HWND {
        HWND hLbl = CreateWindowW(L"STATIC", text, WS_CHILD | WS_VISIBLE, x, y, w, h, hwnd, (HMENU)(INT_PTR)id, hInst, NULL);
        SendMessageW(hLbl, WM_SETFONT, (WPARAM)hFont, TRUE);
        return hLbl;
    };

    int y = 20;
    AddLabel(L"CPU Usage (Process):", 25, y, 180, 20);
    AddLabel(L"0.00 %", 220, y, 160, 20, ID_LBL_CPU);
    y += 30;

    AddLabel(L"RAM (Working Set):", 25, y, 180, 20);
    AddLabel(L"0.00 MB", 220, y, 160, 20, ID_LBL_RAM_WS);
    y += 30;

    AddLabel(L"RAM (Private Bytes):", 25, y, 180, 20);
    AddLabel(L"0.00 MB", 220, y, 160, 20, ID_LBL_RAM_PRIV);
    y += 30;

    AddLabel(L"Tracked Windows:", 25, y, 180, 20);
    AddLabel(L"0", 220, y, 160, 20, ID_LBL_TRACKED);
    y += 30;

    AddLabel(L"Active Overlay Windows:", 25, y, 180, 20);
    AddLabel(L"0", 220, y, 160, 20, ID_LBL_OVERLAYS);
    y += 35;

    AddLabel(L"Safety Architecture:", 25, y, 180, 20);
    AddLabel(L"Zero Injection (Out-of-Process Hook)", 220, y, 175, 40, ID_LBL_SAFETY);
    y += 50;

    HWND btnClose = CreateWindowW(L"BUTTON", L"Close", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 160, y, 90, 28, hwnd, (HMENU)(INT_PTR)ID_BTN_CLOSE_DIAG, hInst, NULL);
    SendMessageW(btnClose, WM_SETFONT, (WPARAM)hFont, TRUE);
}

void DiagnosticDialog::UpdateDisplay(HWND hwnd) {
    auto& diag = DiagnosticManager::Instance();
    auto& mgr = OverlayManager::Instance();

    diag.SetCounts(mgr.GetTrackedCount(), mgr.GetActiveOverlayCount(), mgr.AreHooksInstalled());
    diag.UpdateMetrics();

    const auto& metrics = diag.GetMetrics();

    std::wstringstream ssCpu;
    ssCpu << std::fixed << std::setprecision(2) << metrics.cpuUsagePercent << L" %";
    SetWindowTextW(GetDlgItem(hwnd, ID_LBL_CPU), ssCpu.str().c_str());

    std::wstringstream ssWs;
    ssWs << std::fixed << std::setprecision(2) << metrics.memoryWorkingSetMB << L" MB";
    SetWindowTextW(GetDlgItem(hwnd, ID_LBL_RAM_WS), ssWs.str().c_str());

    std::wstringstream ssPriv;
    ssPriv << std::fixed << std::setprecision(2) << metrics.memoryPrivateBytesMB << L" MB";
    SetWindowTextW(GetDlgItem(hwnd, ID_LBL_RAM_PRIV), ssPriv.str().c_str());

    SetWindowTextW(GetDlgItem(hwnd, ID_LBL_TRACKED), std::to_wstring(metrics.trackedWindowsCount).c_str());
    SetWindowTextW(GetDlgItem(hwnd, ID_LBL_OVERLAYS), std::to_wstring(metrics.activeOverlaysCount).c_str());
}

LRESULT CALLBACK DiagnosticDialog::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_TIMER:
            UpdateDisplay(hwnd);
            return 0;

        case WM_COMMAND:
            if (LOWORD(wParam) == ID_BTN_CLOSE_DIAG) {
                DestroyWindow(hwnd);
            }
            return 0;

        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            KillTimer(hwnd, 1);
            s_hwnd = NULL;
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

} // namespace MacTrafficLights
