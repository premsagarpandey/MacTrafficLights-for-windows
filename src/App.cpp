#include "App.h"
#include "Config.h"
#include "Logger.h"
#include "OverlayManager.h"
#include "TrayIcon.h"

#pragma comment(lib, "gdiplus.lib")

namespace MacTrafficLights {

static const wchar_t* MUTEX_NAME = L"Local\\MacTrafficLights_SingleInstance_Mutex";

typedef BOOL (WINAPI *SetProcessDpiAwarenessContextProc)(DPI_AWARENESS_CONTEXT);

App& App::Instance() {
    static App instance;
    return instance;
}

App::App() {}

App::~App() {
    if (m_gdiplusToken) {
        Gdiplus::GdiplusShutdown(m_gdiplusToken);
        m_gdiplusToken = 0;
    }
    if (m_hSingleInstanceMutex) {
        CloseHandle(m_hSingleInstanceMutex);
        m_hSingleInstanceMutex = NULL;
    }
}

bool App::EnsureSingleInstance() {
    m_hSingleInstanceMutex = CreateMutexW(NULL, TRUE, MUTEX_NAME);
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        if (m_hSingleInstanceMutex) {
            CloseHandle(m_hSingleInstanceMutex);
            m_hSingleInstanceMutex = NULL;
        }
        return false;
    }
    return true;
}

bool App::InitializeDpiAwareness() {
    // Attempt Per-Monitor V2 DPI awareness (Windows 10 Creators Update / Windows 11)
    HMODULE hUser32 = GetModuleHandleW(L"user32.dll");
    if (hUser32) {
        SetProcessDpiAwarenessContextProc pSetDpi = 
            (SetProcessDpiAwarenessContextProc)GetProcAddress(hUser32, "SetProcessDpiAwarenessContext");
        if (pSetDpi) {
            return pSetDpi(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2) != FALSE;
        }
    }
    return false;
}

int App::Run(HINSTANCE hInstance, int nCmdShow) {
    m_hInstance = hInstance;

    // 1. Single instance check
    if (!EnsureSingleInstance()) {
        MessageBoxW(
            NULL,
            L"MacTrafficLights is already running. Check your system tray icons (near the clock).",
            L"MacTrafficLights",
            MB_OK | MB_ICONINFORMATION
        );
        return 0;
    }

    // 2. High-DPI Awareness
    InitializeDpiAwareness();

    // 3. Initialize GDI+
    Gdiplus::GdiplusStartupInput gdiplusStartupInput;
    Gdiplus::Status gdiStatus = Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);
    if (gdiStatus != Gdiplus::Ok) {
        MessageBoxW(NULL, L"Failed to initialize GDI+ graphics library.", L"Error", MB_OK | MB_ICONERROR);
        return 1;
    }

    // 4. Initialize Logger
    Logger::Instance().Init();
    LOG_INFO(L"=== MacTrafficLights Starting on Windows 11 ===");

    // 5. Initialize Configuration & Auto-Start with Windows
    ConfigManager::Instance().Load();
    ConfigManager::Instance().EnableAutoStart(true);

    // 6. Initialize System Tray
    if (!TrayIcon::Instance().Initialize(hInstance)) {
        LOG_ERROR(L"Failed to initialize system tray icon");
        return 1;
    }

    // 7. Initialize Overlay Manager (WinEvent hooks & initial window scan)
    if (!OverlayManager::Instance().Initialize(hInstance)) {
        LOG_ERROR(L"Failed to initialize Overlay Manager");
        return 1;
    }

    LOG_INFO(L"MacTrafficLights active and running in background");

    // 8. Main Message Loop
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    // 9. Graceful clean shutdown
    LOG_INFO(L"MacTrafficLights shutting down...");
    OverlayManager::Instance().Shutdown();
    TrayIcon::Instance().Shutdown();

    if (m_gdiplusToken) {
        Gdiplus::GdiplusShutdown(m_gdiplusToken);
        m_gdiplusToken = 0;
    }

    LOG_INFO(L"MacTrafficLights cleanly exited. Windows restored to default.");
    return static_cast<int>(msg.wParam);
}

} // namespace MacTrafficLights
