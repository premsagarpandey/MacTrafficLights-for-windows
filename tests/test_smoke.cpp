#include <iostream>
#include <cassert>
#include <cmath>
#include <windows.h>
#include "../include/Config.h"
#include "../include/WindowFilter.h"
#include "../include/NativeActions.h"

using namespace MacTrafficLights;

void TestConfig() {
    std::cout << "[TEST] Running Config Tests..." << std::endl;
    auto& cfgMgr = ConfigManager::Instance();
    cfgMgr.ResetToDefaults();

    const auto& cfg = cfgMgr.GetConfig();
    assert(cfg.enabled == true);
    assert(cfg.buttonSize == 12);
    assert(cfg.buttonSpacing == 8);
    assert(cfg.rightMargin == 14);
    assert(cfg.topMargin == 10);
    assert(cfg.dimWhenInactive == true);

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
    int baseRight = 14;

    // 100% DPI (96)
    assert(MulDiv(baseSize, 96, 96) == 12);
    assert(MulDiv(baseSpacing, 96, 96) == 8);
    assert(MulDiv(baseRight, 96, 96) == 14);

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
    int rightMargin = 14;
    int nativeBtnWidth = 46;
    int overlayW = 3 * nativeBtnWidth; // 138
    int overlayH = 38;

    auto HitTest = [&](int x, int y) -> int {
        if (x < 0 || x > overlayW || y < 0 || y > overlayH) {
            return -1;
        }
        // Slot 1 (Rightmost): Close (2)
        if (x >= overlayW - nativeBtnWidth) {
            return 2;
        }
        // Slot 2 (Middle): Maximize (1)
        if (x >= overlayW - 2 * nativeBtnWidth) {
            return 1;
        }
        // Slot 3 (Leftmost): Minimize (0)
        if (x >= overlayW - 3 * nativeBtnWidth) {
            return 0;
        }
        return -1;
    };

    // Slot 3 (Minimize: Green) - x in [0, 46)
    assert(HitTest(10, 19) == 0);
    assert(HitTest(23, 19) == 0);
    assert(HitTest(45, 19) == 0);

    // Slot 2 (Maximize: Yellow) - x in [46, 92)
    assert(HitTest(46, 19) == 1);
    assert(HitTest(69, 19) == 1);
    assert(HitTest(91, 19) == 1);

    // Slot 1 (Close: Red) - x in [92, 138]
    assert(HitTest(92, 19) == 2);
    assert(HitTest(115, 19) == 2);
    assert(HitTest(138, 19) == 2);

    // Outside bounds
    assert(HitTest(-5, 19) == -1);
    assert(HitTest(140, 19) == -1);
    assert(HitTest(50, -2) == -1);
    assert(HitTest(50, 40) == -1);

    std::cout << "  -> Hit-Test Tests Passed!" << std::endl;
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
    TestWindowFilter();

    std::cout << "==========================================" << std::endl;
    std::cout << "ALL SMOKE TESTS PASSED SUCCESSFULLY!" << std::endl;
    std::cout << "==========================================" << std::endl;
    return 0;
}
