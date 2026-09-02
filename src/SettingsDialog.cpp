#include "SettingsDialog.h"
#include "Config.h"
#include "OverlayManager.h"
#include "Resource.h"
#include <commctrl.h>
#include <sstream>
#include <vector>

#pragma comment(lib, "comctl32.lib")

namespace MacTrafficLights {

HWND SettingsDialog::s_hwnd = NULL;

static const wchar_t* SETTINGS_CLASS = L"MacTrafficLights_SettingsWindow";

// Control IDs
enum CtrlId {
    ID_CHK_ENABLED = 101,
    ID_SLD_SIZE,
    ID_LBL_SIZE,
    ID_SLD_SPACING,
    ID_LBL_SPACING,
    ID_SLD_LEFT_MARGIN,
    ID_LBL_LEFT_MARGIN,
    ID_SLD_TOP_MARGIN,
    ID_LBL_TOP_MARGIN,
    ID_CHK_DIM,
    ID_CHK_HOVER,
    ID_CHK_HIDE_RIGHT,
    ID_CHK_STARTUP,
    ID_EDT_EXCLUSIONS,
    ID_BTN_SAVE,
    ID_BTN_RESET,
    ID_BTN_CANCEL
};

void SettingsDialog::Show(HINSTANCE hInstance, HWND hParent) {
    if (s_hwnd && IsWindow(s_hwnd)) {
        SetForegroundWindow(s_hwnd);
        return;
    }

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(INITCOMMONCONTROLSEX);
    icex.dwICC = ICC_BAR_CLASSES | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);

    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.lpfnWndProc = SettingsDialog::WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.lpszClassName = SETTINGS_CLASS;

    RegisterClassExW(&wc);

    int width = 450;
    int height = 580;
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int x = (screenW - width) / 2;
    int y = (screenH - height) / 2;

    s_hwnd = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        SETTINGS_CLASS,
        L"MacTrafficLights Settings",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        x, y, width, height,
        hParent, NULL, hInstance, NULL
    );

    InitializeControls(s_hwnd);
    ShowWindow(s_hwnd, SW_SHOW);
    UpdateWindow(s_hwnd);
}

void SettingsDialog::InitializeControls(HWND hwnd) {
    HINSTANCE hInst = (HINSTANCE)GetWindowLongPtrW(hwnd, GWLP_HINSTANCE);
    HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    const auto& cfg = ConfigManager::Instance().GetConfig();

    auto AddLabel = [&](const wchar_t* text, int x, int y, int w, int h, int id = -1) -> HWND {
        HWND hLbl = CreateWindowW(L"STATIC", text, WS_CHILD | WS_VISIBLE, x, y, w, h, hwnd, (HMENU)(INT_PTR)id, hInst, NULL);
        SendMessageW(hLbl, WM_SETFONT, (WPARAM)hFont, TRUE);
        return hLbl;
    };

    auto AddCheckbox = [&](const wchar_t* text, int x, int y, int w, int h, int id, bool checked) -> HWND {
        HWND hChk = CreateWindowW(L"BUTTON", text, WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX, x, y, w, h, hwnd, (HMENU)(INT_PTR)id, hInst, NULL);
        SendMessageW(hChk, WM_SETFONT, (WPARAM)hFont, TRUE);
        SendMessageW(hChk, BM_SETCHECK, checked ? BST_CHECKED : BST_UNCHECKED, 0);
        return hChk;
    };

    auto AddSlider = [&](int x, int y, int w, int h, int id, int minVal, int maxVal, int curVal) -> HWND {
        HWND hSld = CreateWindowW(TRACKBAR_CLASSW, L"", WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_AUTOTICKS, x, y, w, h, hwnd, (HMENU)(INT_PTR)id, hInst, NULL);
        SendMessageW(hSld, TBM_SETRANGE, TRUE, MAKELONG(minVal, maxVal));
        SendMessageW(hSld, TBM_SETPOS, TRUE, curVal);
        return hSld;
    };

    int y = 15;

    // Enable / Disable Checkbox
    AddCheckbox(L"Enable MacTrafficLights", 20, y, 380, 24, ID_CHK_ENABLED, cfg.enabled);
    y += 35;

    // Button Size
    AddLabel(L"Button Size:", 20, y, 150, 20);
    HWND lblSize = AddLabel((std::to_wstring(cfg.buttonSize) + L" px").c_str(), 350, y, 60, 20, ID_LBL_SIZE);
    y += 20;
    AddSlider(20, y, 390, 28, ID_SLD_SIZE, 8, 24, cfg.buttonSize);
    y += 35;

    // Spacing
    AddLabel(L"Button Spacing:", 20, y, 150, 20);
    HWND lblSpacing = AddLabel((std::to_wstring(cfg.buttonSpacing) + L" px").c_str(), 350, y, 60, 20, ID_LBL_SPACING);
    y += 20;
    AddSlider(20, y, 390, 28, ID_SLD_SPACING, 4, 20, cfg.buttonSpacing);
    y += 35;

    // Left Margin
    AddLabel(L"Left Margin (Title Bar Offset):", 20, y, 200, 20);
    HWND lblLeft = AddLabel((std::to_wstring(cfg.leftMargin) + L" px").c_str(), 350, y, 60, 20, ID_LBL_LEFT_MARGIN);
    y += 20;
    AddSlider(20, y, 390, 28, ID_SLD_LEFT_MARGIN, 2, 50, cfg.leftMargin);
    y += 35;

    // Top Margin / Vertical Offset
    AddLabel(L"Vertical Title Bar Offset:", 20, y, 200, 20);
    HWND lblTop = AddLabel((std::to_wstring(cfg.topMargin) + L" px").c_str(), 350, y, 60, 20, ID_LBL_TOP_MARGIN);
    y += 20;
    AddSlider(20, y, 390, 28, ID_SLD_TOP_MARGIN, 2, 35, cfg.topMargin);
    y += 35;

    // Visual options
    AddCheckbox(L"Dim buttons when target window is inactive", 20, y, 380, 22, ID_CHK_DIM, cfg.dimWhenInactive);
    y += 26;
    AddCheckbox(L"Show subtle symbols (x, -, +) on button hover", 20, y, 380, 22, ID_CHK_HOVER, cfg.showHoverSymbols);
    y += 26;
    AddCheckbox(L"Hide standard Windows buttons on right side", 20, y, 380, 22, ID_CHK_HIDE_RIGHT, cfg.hideRightButtons);
    y += 26;
    AddCheckbox(L"Start with Windows (Run on startup)", 20, y, 380, 22, ID_CHK_STARTUP, ConfigManager::Instance().IsStartWithWindowsEnabled());
    y += 35;

    // Excluded applications
    AddLabel(L"Excluded Processes (comma-separated, e.g. game.exe):", 20, y, 380, 18);
    y += 22;
    std::wstring excStr;
    for (size_t i = 0; i < cfg.excludedProcesses.size(); ++i) {
        if (i > 0) excStr += L", ";
        excStr += cfg.excludedProcesses[i];
    }
    HWND hEdt = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", excStr.c_str(), 
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL, 20, y, 390, 24, hwnd, (HMENU)(INT_PTR)ID_EDT_EXCLUSIONS, hInst, NULL);
    SendMessageW(hEdt, WM_SETFONT, (WPARAM)hFont, TRUE);
    y += 40;

    // Action Buttons
    HWND btnSave = CreateWindowW(L"BUTTON", L"Save", WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON, 130, y, 80, 28, hwnd, (HMENU)(INT_PTR)ID_BTN_SAVE, hInst, NULL);
    HWND btnReset = CreateWindowW(L"BUTTON", L"Defaults", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 220, y, 80, 28, hwnd, (HMENU)(INT_PTR)ID_BTN_RESET, hInst, NULL);
    HWND btnCancel = CreateWindowW(L"BUTTON", L"Cancel", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, 310, y, 80, 28, hwnd, (HMENU)(INT_PTR)ID_BTN_CANCEL, hInst, NULL);

    SendMessageW(btnSave, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessageW(btnReset, WM_SETFONT, (WPARAM)hFont, TRUE);
    SendMessageW(btnCancel, WM_SETFONT, (WPARAM)hFont, TRUE);
}

void SettingsDialog::SaveFromControls(HWND hwnd) {
    auto& cfg = ConfigManager::Instance().GetMutableConfig();

    cfg.enabled = (SendMessageW(GetDlgItem(hwnd, ID_CHK_ENABLED), BM_GETCHECK, 0, 0) == BST_CHECKED);
    cfg.buttonSize = (int)SendMessageW(GetDlgItem(hwnd, ID_SLD_SIZE), TBM_GETPOS, 0, 0);
    cfg.buttonSpacing = (int)SendMessageW(GetDlgItem(hwnd, ID_SLD_SPACING), TBM_GETPOS, 0, 0);
    cfg.leftMargin = (int)SendMessageW(GetDlgItem(hwnd, ID_SLD_LEFT_MARGIN), TBM_GETPOS, 0, 0);
    cfg.topMargin = (int)SendMessageW(GetDlgItem(hwnd, ID_SLD_TOP_MARGIN), TBM_GETPOS, 0, 0);
    cfg.dimWhenInactive = (SendMessageW(GetDlgItem(hwnd, ID_CHK_DIM), BM_GETCHECK, 0, 0) == BST_CHECKED);
    cfg.showHoverSymbols = (SendMessageW(GetDlgItem(hwnd, ID_CHK_HOVER), BM_GETCHECK, 0, 0) == BST_CHECKED);
    cfg.hideRightButtons = (SendMessageW(GetDlgItem(hwnd, ID_CHK_HIDE_RIGHT), BM_GETCHECK, 0, 0) == BST_CHECKED);
    cfg.startWithWindows = (SendMessageW(GetDlgItem(hwnd, ID_CHK_STARTUP), BM_GETCHECK, 0, 0) == BST_CHECKED);

    wchar_t buf[2048] = { 0 };
    GetWindowTextW(GetDlgItem(hwnd, ID_EDT_EXCLUSIONS), buf, 2048);
    cfg.excludedProcesses.clear();
    std::wstringstream ss(buf);
    std::wstring item;
    while (std::getline(ss, item, L',')) {
        item.erase(0, item.find_first_not_of(L" \t\r\n"));
        item.erase(item.find_last_not_of(L" \t\r\n") + 1);
        if (!item.empty()) {
            std::transform(item.begin(), item.end(), item.begin(), ::towlower);
            cfg.excludedProcesses.push_back(item);
        }
    }

    ConfigManager::Instance().Save();
    OverlayManager::Instance().SetEnabled(cfg.enabled);
    OverlayManager::Instance().RefreshAllOverlays();
}

void SettingsDialog::ResetDefaults(HWND hwnd) {
    ConfigManager::Instance().ResetToDefaults();
    const auto& cfg = ConfigManager::Instance().GetConfig();

    SendMessageW(GetDlgItem(hwnd, ID_CHK_ENABLED), BM_SETCHECK, cfg.enabled ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(GetDlgItem(hwnd, ID_SLD_SIZE), TBM_SETPOS, TRUE, cfg.buttonSize);
    SendMessageW(GetDlgItem(hwnd, ID_SLD_SPACING), TBM_SETPOS, TRUE, cfg.buttonSpacing);
    SendMessageW(GetDlgItem(hwnd, ID_SLD_LEFT_MARGIN), TBM_SETPOS, TRUE, cfg.leftMargin);
    SendMessageW(GetDlgItem(hwnd, ID_SLD_TOP_MARGIN), TBM_SETPOS, TRUE, cfg.topMargin);
    SendMessageW(GetDlgItem(hwnd, ID_CHK_DIM), BM_SETCHECK, cfg.dimWhenInactive ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(GetDlgItem(hwnd, ID_CHK_HOVER), BM_SETCHECK, cfg.showHoverSymbols ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(GetDlgItem(hwnd, ID_CHK_HIDE_RIGHT), BM_SETCHECK, cfg.hideRightButtons ? BST_CHECKED : BST_UNCHECKED, 0);
    SendMessageW(GetDlgItem(hwnd, ID_CHK_STARTUP), BM_SETCHECK, cfg.startWithWindows ? BST_CHECKED : BST_UNCHECKED, 0);

    SetWindowTextW(GetDlgItem(hwnd, ID_LBL_SIZE), (std::to_wstring(cfg.buttonSize) + L" px").c_str());
    SetWindowTextW(GetDlgItem(hwnd, ID_LBL_SPACING), (std::to_wstring(cfg.buttonSpacing) + L" px").c_str());
    SetWindowTextW(GetDlgItem(hwnd, ID_LBL_LEFT_MARGIN), (std::to_wstring(cfg.leftMargin) + L" px").c_str());
    SetWindowTextW(GetDlgItem(hwnd, ID_LBL_TOP_MARGIN), (std::to_wstring(cfg.topMargin) + L" px").c_str());
}

LRESULT CALLBACK SettingsDialog::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_HSCROLL: {
            HWND hScroll = (HWND)lParam;
            int pos = (int)SendMessageW(hScroll, TBM_GETPOS, 0, 0);
            if (hScroll == GetDlgItem(hwnd, ID_SLD_SIZE)) {
                SetWindowTextW(GetDlgItem(hwnd, ID_LBL_SIZE), (std::to_wstring(pos) + L" px").c_str());
            } else if (hScroll == GetDlgItem(hwnd, ID_SLD_SPACING)) {
                SetWindowTextW(GetDlgItem(hwnd, ID_LBL_SPACING), (std::to_wstring(pos) + L" px").c_str());
            } else if (hScroll == GetDlgItem(hwnd, ID_SLD_LEFT_MARGIN)) {
                SetWindowTextW(GetDlgItem(hwnd, ID_LBL_LEFT_MARGIN), (std::to_wstring(pos) + L" px").c_str());
            } else if (hScroll == GetDlgItem(hwnd, ID_SLD_TOP_MARGIN)) {
                SetWindowTextW(GetDlgItem(hwnd, ID_LBL_TOP_MARGIN), (std::to_wstring(pos) + L" px").c_str());
            }
            return 0;
        }

        case WM_COMMAND: {
            int id = LOWORD(wParam);
            if (id == ID_BTN_SAVE) {
                SaveFromControls(hwnd);
                DestroyWindow(hwnd);
            } else if (id == ID_BTN_RESET) {
                ResetDefaults(hwnd);
            } else if (id == ID_BTN_CANCEL) {
                DestroyWindow(hwnd);
            }
            return 0;
        }

        case WM_CLOSE:
            DestroyWindow(hwnd);
            return 0;

        case WM_DESTROY:
            s_hwnd = NULL;
            return 0;
    }
    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

} // namespace MacTrafficLights
