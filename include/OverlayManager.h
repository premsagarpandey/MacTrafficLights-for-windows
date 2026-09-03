#pragma once

#include <windows.h>
#include <unordered_map>
#include <memory>
#include <mutex>
#include "OverlayWindow.h"

#ifndef EVENT_OBJECT_CLOAKED
#define EVENT_OBJECT_CLOAKED 0x8017
#endif
#ifndef EVENT_OBJECT_UNCLOAKED
#define EVENT_OBJECT_UNCLOAKED 0x8018
#endif

namespace MacTrafficLights {

class OverlayManager {
public:
    static OverlayManager& Instance();

    bool Initialize(HINSTANCE hInstance);
    void Shutdown();

    void ScanExistingWindows();
    void SetEnabled(bool enabled);
    bool IsEnabled() const { return m_enabled; }

    void HandleLocationChange(HWND hwnd);
    void HandleForegroundChange(HWND newActiveHwnd);
    void HandleWindowDestroy(HWND hwnd);
    void HandleWindowShow(HWND hwnd);
    void HandleWindowHide(HWND hwnd);
    void HandleWindowMinimize(HWND hwnd, bool minimized);
    void HandleWindowCloak(HWND hwnd, bool cloaked);

    void OnTimerTick();

private:
    OverlayManager();
    ~OverlayManager();

    static void CALLBACK WinEventProc(
        HWINEVENTHOOK hWinEventHook,
        DWORD event,
        HWND hwnd,
        LONG idObject,
        LONG idChild,
        DWORD idEventThread,
        DWORD dwmsEventTime
    );

    static void CALLBACK TimerCallback(HWND hwnd, UINT msg, UINT_PTR id, DWORD time);

    bool AddOverlayForWindow(HWND hwnd);
    void RemoveOverlayForWindow(HWND hwnd);

    HINSTANCE m_hInstance = NULL;
    bool m_enabled = true;
    HWND m_lastForegroundHwnd = NULL;
    UINT_PTR m_timerId = 0;

    HWINEVENTHOOK m_hHookLocation = NULL;
    HWINEVENTHOOK m_hHookState = NULL;
    HWINEVENTHOOK m_hHookCloaked = NULL;

    std::unordered_map<HWND, std::unique_ptr<OverlayWindow>> m_overlays;
    mutable std::mutex m_mutex;
};

} // namespace MacTrafficLights
