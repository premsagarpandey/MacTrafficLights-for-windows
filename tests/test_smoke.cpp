#include <iostream>
#include <cassert>
#include <cmath>
#include <windows.h>
#include "../include/Config.h"
#include "../include/WindowFilter.h"
#include "../include/NativeActions.h"
#include "../include/Diagnostic.h"

using namespace MacTrafficLights;

void TestConfig() {
    std::cout << "[TEST] Running Config Tests..." << std::endl;
    auto& cfgMgr = ConfigManager::Instance();
    cfgMgr.ResetToDefaults();

    const auto& cfg = cfgMgr.GetConfig();
    assert(cfg.enabled == true);
    assert(cfg.buttonSize == 12);
    assert(cfg.buttonSpacing == 8);
    assert(cfg.leftMargin == 14);
    assert(cfg.topMargin == 10);
    assert(cfg.dimWhenInactive == true);
    assert(cfg.showHoverSymbols == true);
    assert(cfg.hideRightButtons == true);

    // Test exclusions
    assert(cfgMgr.IsProcessExcluded(L"dwm.exe") == true);
    assert(cfgMgr.IsProcessExcluded(L"DWM.EXE") == true); // Case insensitivity
    assert(cfgMgr.IsProcessExcluded(L"notepad.exe") == false);

    cfgMgr.AddExclusion(L"customgame.exe");
    assert(cfgMgr.IsProcessExcluded(L"customgame.exe") == true);
    cfgMgr.RemoveExclusion(L"customgame.exe");
    assert(cfgMgr.IsProcessExcluded(L"customgame.exe") == false);

    std::cout << "  -> Config Tests Passed!" << std::endl;
}

void TestDpiCalculations() {
    std::cout << "[TEST] Running DPI Scaling Tests..." << std::endl;

    int baseSize = 12;
    int baseSpacing = 8;
    int baseLeft = 14;

    // 100% DPI (96)
    assert(MulDiv(baseSize, 96, 96) == 12);
    assert(MulDiv(baseSpacing, 96, 96) == 8);
    assert(MulDiv(baseLeft, 96, 96) == 14);

    // 125% DPI (120)
    assert(MulDiv(baseSize, 120, 96) == 15);
    assert(MulDiv(baseSpacing, 120, 96) == 10);

    // 150% DPI (144)
    assert(MulDiv(baseSize, 144, 96) == 18);
    assert(MulDiv(baseSpacing, 144, 96) == 12);

    // 200% DPI (192)
    assert(MulDiv(baseSize, 192, 96) == 24);
    assert(MulDiv(baseSpacing, 192, 96) == 16);

    std::cout << "  -> DPI Scaling Tests Passed!" << std::endl;
}

void TestHitTesting() {
    std::cout << "[TEST] Running Hit-Test Geometric Tests..." << std::endl;

    int buttonSize = 12;
    int spacing = 8;
    int leftMargin = 14;
    int topMargin = 10;
    int radius = buttonSize / 2; // 6
    int centerY = topMargin + radius; // 16

    auto HitTest = [&](int x, int y) -> int {
        for (int i = 0; i < 3; ++i) {
            int centerX = leftMargin + (i * (buttonSize + spacing)) + radius;
            int dx = x - centerX;
            int dy = y - centerY;
            if ((dx * dx + dy * dy) <= ((radius + 1) * (radius + 1))) {
                return i; // 0 = Red, 1 = Yellow, 2 = Green
            }
        }
        return -1;
    };

    // Button 0 (Red) center: leftMargin (14) + radius (6) = 20, y = 16
    assert(HitTest(20, 16) == 0);
    assert(HitTest(22, 16) == 0); // slightly off-center
    assert(HitTest(20, 18) == 0);

    // Button 1 (Yellow) center: 14 + 1*(12 + 8) + 6 = 40, y = 16
    assert(HitTest(40, 16) == 1);

    // Button 2 (Green) center: 14 + 2*(12 + 8) + 6 = 60, y = 16
    assert(HitTest(60, 16) == 2);

    // Gap between button 0 and 1 (x = 30) -> Should hit nothing (-1)
    assert(HitTest(30, 16) == -1);

    // Outside bounds
    assert(HitTest(0, 0) == -1);
    assert(HitTest(20, 0) == -1);

    std::cout << "  -> Hit-Test Tests Passed!" << std::endl;
}

void TestDiagnostics() {
    std::cout << "[TEST] Running Diagnostics Tests..." << std::endl;

    auto& diag = DiagnosticManager::Instance();
    diag.SetCounts(10, 5, true);
    diag.UpdateMetrics();

    const auto& m = diag.GetMetrics();
    assert(m.trackedWindowsCount == 10);
    assert(m.activeOverlaysCount == 5);
    assert(m.hooksInstalled == true);
    assert(m.memoryWorkingSetMB > 0.0); // Should be non-zero for active process
    assert(m.cpuUsagePercent >= 0.0 && m.cpuUsagePercent <= 100.0);

    std::cout << "  -> Diagnostics Tests Passed! Working set: " << m.memoryWorkingSetMB << " MB" << std::endl;
}

void TestWindowFilter() {
    std::cout << "[TEST] Running WindowFilter Safety Tests..." << std::endl;

    // NULL HWND must never be eligible
    assert(WindowFilter::IsEligibleWindow(NULL) == false);

    // Desktop window (GetDesktopWindow()) must never be eligible
    HWND hDesktop = GetDesktopWindow();
    assert(WindowFilter::IsEligibleWindow(hDesktop) == false);

    // Shell tray window (Taskbar) if present must never be eligible
    HWND hTray = FindWindowW(L"Shell_TrayWnd", NULL);
    if (hTray) {
        assert(WindowFilter::IsEligibleWindow(hTray) == false);
    }

    std::cout << "  -> WindowFilter Safety Tests Passed!" << std::endl;
}

int main() {
    std::cout << "==========================================" << std::endl;
    std::cout << "MacTrafficLights Automated Smoke Test Suite" << std::endl;
    std::cout << "==========================================" << std::endl;

    TestConfig();
    TestDpiCalculations();
    TestHitTesting();
    TestDiagnostics();
    TestWindowFilter();

    std::cout << "==========================================" << std::endl;
    std::cout << "ALL SMOKE TESTS PASSED SUCCESSFULLY!" << std::endl;
    std::cout << "==========================================" << std::endl;
    return 0;
}
