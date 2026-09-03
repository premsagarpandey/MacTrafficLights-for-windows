#include "OverlayManager.h"
#include "WindowFilter.h"
#include "Config.h"
#include "Logger.h"

namespace MacTrafficLights {

OverlayManager& OverlayManager::Instance() {
    static OverlayManager instance;
    return instance;
}

OverlayManager::OverlayManager() {
    m_enabled = ConfigManager::Instance().GetConfig().enabled;
}

OverlayManager::~OverlayManager() {
    Shutdown();
}

BOOL CALLBACK EnumWindowsProc(HWND hwnd, LPARAM lParam) {
    OverlayManager* pMgr = reinterpret_cast<OverlayManager*>(lParam);
    if (pMgr && WindowFilter::IsEligibleWindow(hwnd)) {
        pMgr->HandleWindowShow(hwnd);
    }
    return TRUE;
}

void CALLBACK OverlayManager::TimerCallback(HWND /*hwnd*/, UINT /*msg*/, UINT_PTR /*id*/, DWORD /*time*/) {
    OverlayManager::Instance().OnTimerTick();
}

void OverlayManager::OnTimerTick() {
    if (!m_enabled) return;

    // 1. High-priority watchdog: ensure the currently active foreground window has its overlay visible and on top
    HWND fg = GetForegroundWindow();
    if (fg && WindowFilter::IsEligibleWindow(fg)) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_overlays.find(fg);
        if (it == m_overlays.end()) {
            AddOverlayForWindow(fg);
        } else {
            if (!IsWindowVisible(it->second->GetHwnd())) {
                it->second->Show(true);
            }
            it->second->SetTargetActive(true);
            it->second->UpdatePosition();
        }
    }

    // 2. Low-frequency sweep every 2 seconds to pick up any new background windows
    static DWORD lastSweep = 0;
    DWORD now = GetTickCount();
    if (now - lastSweep > 2000) {
        lastSweep = now;
        ScanExistingWindows();
    }
}

bool OverlayManager::Initialize(HINSTANCE hInstance) {
    m_hInstance = hInstance;

    if (!OverlayWindow::RegisterOverlayClass(hInstance)) {
        LOG_ERROR(L"Failed to register OverlayWindow class");
        return false;
    }

    // Install out-of-process WinEvent hooks
    m_hHookLocation = SetWinEventHook(
        EVENT_OBJECT_LOCATIONCHANGE,
        EVENT_OBJECT_LOCATIONCHANGE,
        NULL,
        WinEventProc,
        0, 0,
        WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS
    );

    m_hHookState = SetWinEventHook(
        EVENT_SYSTEM_FOREGROUND,
        EVENT_OBJECT_HIDE,
        NULL,
        WinEventProc,
        0, 0,
        WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS
    );

    m_hHookCloaked = SetWinEventHook(
        EVENT_OBJECT_CLOAKED,
        EVENT_OBJECT_UNCLOAKED,
        NULL,
        WinEventProc,
        0, 0,
        WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS
    );

    if (!m_hHookLocation || !m_hHookState) {
        LOG_ERROR(L"Failed to install WinEvent hooks");
    } else {
        LOG_INFO(L"WinEvent hooks successfully installed");
    }

    // 250ms periodic watchdog timer for 100% overlay reliability
    m_timerId = SetTimer(NULL, 0, 250, TimerCallback);

    if (m_enabled) {
        ScanExistingWindows();
    }

    m_lastForegroundHwnd = GetForegroundWindow();
    HandleForegroundChange(m_lastForegroundHwnd);

    return true;
}

void OverlayManager::Shutdown() {
    if (m_timerId) {
        KillTimer(NULL, m_timerId);
        m_timerId = 0;
    }

    if (m_hHookLocation) {
        UnhookWinEvent(m_hHookLocation);
        m_hHookLocation = NULL;
    }
    if (m_hHookState) {
        UnhookWinEvent(m_hHookState);
        m_hHookState = NULL;
    }
    if (m_hHookCloaked) {
        UnhookWinEvent(m_hHookCloaked);
        m_hHookCloaked = NULL;
    }

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_overlays.clear();
    }

    if (m_hInstance) {
        OverlayWindow::UnregisterOverlayClass(m_hInstance);
    }

    LOG_INFO(L"OverlayManager cleanly shut down");
}

void OverlayManager::ScanExistingWindows() {
    EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(this));
}

void OverlayManager::SetEnabled(bool enabled) {
    if (m_enabled == enabled) return;

    m_enabled = enabled;
    ConfigManager::Instance().GetMutableConfig().enabled = enabled;
    ConfigManager::Instance().Save();

    if (!m_enabled) {
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& pair : m_overlays) {
            pair.second->Show(false);
        }
    } else {
        ScanExistingWindows();
        std::lock_guard<std::mutex> lock(m_mutex);
        for (auto& pair : m_overlays) {
            pair.second->Show(true);
            pair.second->UpdatePosition(true);
        }
    }
}

bool OverlayManager::AddOverlayForWindow(HWND hwnd) {
    if (!m_enabled) return false;
    if (!WindowFilter::IsEligibleWindow(hwnd)) return false;

    if (m_overlays.find(hwnd) != m_overlays.end()) {
        return true;
    }

    auto overlay = std::make_unique<OverlayWindow>(hwnd, m_hInstance);
    if (overlay->Create()) {
        overlay->SetTargetActive(hwnd == GetForegroundWindow());
        m_overlays[hwnd] = std::move(overlay);
        return true;
    }
    return false;
}

void OverlayManager::RemoveOverlayForWindow(HWND hwnd) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_overlays.find(hwnd);
    if (it != m_overlays.end()) {
        it->second->Destroy();
        m_overlays.erase(it);
    }
}

void OverlayManager::HandleLocationChange(HWND hwnd) {
    if (!m_enabled) return;

    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_overlays.find(hwnd);
    if (it != m_overlays.end()) {
        it->second->UpdatePosition();
    } else if (WindowFilter::IsEligibleWindow(hwnd)) {
        AddOverlayForWindow(hwnd);
    }
}

void OverlayManager::HandleForegroundChange(HWND newActiveHwnd) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // Deactivate previous foreground window overlay
    if (m_lastForegroundHwnd && m_lastForegroundHwnd != newActiveHwnd) {
        auto it = m_overlays.find(m_lastForegroundHwnd);
        if (it != m_overlays.end()) {
            it->second->SetTargetActive(false);
            it->second->UpdatePosition();
        }
    }

    // Activate and bring forward new active window overlay
    if (newActiveHwnd) {
        auto it = m_overlays.find(newActiveHwnd);
        if (it != m_overlays.end()) {
            it->second->Show(true);
            it->second->SetTargetActive(true);
            it->second->UpdatePosition(true);
        } else if (WindowFilter::IsEligibleWindow(newActiveHwnd)) {
            AddOverlayForWindow(newActiveHwnd);
        }
    }

    m_lastForegroundHwnd = newActiveHwnd;
}

void OverlayManager::HandleWindowDestroy(HWND hwnd) {
    RemoveOverlayForWindow(hwnd);
}

void OverlayManager::HandleWindowShow(HWND hwnd) {
    if (!m_enabled) return;

    if (!WindowFilter::IsEligibleWindow(hwnd)) {
        return;
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_overlays.find(hwnd);
    if (it != m_overlays.end()) {
        it->second->Show(true);
        it->second->UpdatePosition(true);
        return;
    }

    AddOverlayForWindow(hwnd);
}

void OverlayManager::HandleWindowHide(HWND hwnd) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_overlays.find(hwnd);
    if (it != m_overlays.end()) {
        // Only hide if the window is genuinely invisible or minimized
        if (!IsWindowVisible(hwnd) || IsIconic(hwnd)) {
            it->second->Show(false);
        }
    }
}

void OverlayManager::HandleWindowMinimize(HWND hwnd, bool minimized) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_overlays.find(hwnd);
    if (it != m_overlays.end()) {
        it->second->Show(!minimized);
        if (!minimized) {
            it->second->UpdatePosition(true);
        }
    } else if (!minimized && WindowFilter::IsEligibleWindow(hwnd)) {
        AddOverlayForWindow(hwnd);
    }
}

void OverlayManager::HandleWindowCloak(HWND hwnd, bool cloaked) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_overlays.find(hwnd);
    if (it != m_overlays.end()) {
        it->second->Show(!cloaked);
        if (!cloaked) {
            it->second->UpdatePosition(true);
        }
    } else if (!cloaked && WindowFilter::IsEligibleWindow(hwnd)) {
        AddOverlayForWindow(hwnd);
    }
}

void CALLBACK OverlayManager::WinEventProc(
    HWINEVENTHOOK /*hWinEventHook*/,
    DWORD event,
    HWND hwnd,
    LONG idObject,
    LONG idChild,
    DWORD /*idEventThread*/,
    DWORD /*dwmsEventTime*/
) {
    // Only process top-level window events
    if (idObject != OBJID_WINDOW || idChild != CHILDID_SELF || !hwnd) {
        return;
    }

    OverlayManager& mgr = OverlayManager::Instance();

    switch (event) {
        case EVENT_OBJECT_LOCATIONCHANGE:
            mgr.HandleLocationChange(hwnd);
            break;

        case EVENT_SYSTEM_FOREGROUND:
            mgr.HandleForegroundChange(hwnd);
            break;

        case EVENT_OBJECT_DESTROY:
            mgr.HandleWindowDestroy(hwnd);
            break;

        case EVENT_OBJECT_SHOW:
            mgr.HandleWindowShow(hwnd);
            break;

        case EVENT_OBJECT_HIDE:
            mgr.HandleWindowHide(hwnd);
            break;

        case EVENT_SYSTEM_MINIMIZESTART:
            mgr.HandleWindowMinimize(hwnd, true);
            break;

        case EVENT_SYSTEM_MINIMIZEEND:
            mgr.HandleWindowMinimize(hwnd, false);
            break;

        case EVENT_OBJECT_CLOAKED:
            mgr.HandleWindowCloak(hwnd, true);
            break;

        case EVENT_OBJECT_UNCLOAKED:
            mgr.HandleWindowCloak(hwnd, false);
            break;

        default:
            break;
    }
}

} // namespace MacTrafficLights
