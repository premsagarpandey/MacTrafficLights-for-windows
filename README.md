# MacTrafficLights for Windows 11

[![CI](https://github.com/OWNER/MacTrafficLights-for-windows/actions/workflows/ci.yml/badge.svg)](https://github.com/OWNER/MacTrafficLights-for-windows/actions)
![Platform: Windows 11](https://img.shields.io/badge/Platform-Windows%2011%20x64-blue.svg)
![Language: C++17](https://img.shields.io/badge/Language-C%2B%2B17%20%2F%20C%2B%2B20-brightgreen.svg)
![Architecture: Zero-Injection](https://img.shields.io/badge/Architecture-Zero--Injection-success.svg)
![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)

A production-ready, ultra-lightweight native Windows 11 utility that renders macOS-inspired traffic-light window controls on the **left side** of application title bars while strictly adhering to Windows system integrity and zero-injection safety standards.

```
+---------------------------------------------------------------------------------+
|  (🔴) (🟡) (🟢)   Document - Notepad                                    [—] [🗖] [✕]  |
+---------------------------------------------------------------------------------+
|                                                                                 |
|   Left Side: macOS Traffic Lights                Right Side: Standard Windows   |
|   🔴 Close | 🟡 Minimize | 🟢 Maximize/Restore   (Preserved & Passthrough)      |
|                                                                                 |
+---------------------------------------------------------------------------------+
```

---

## 🎯 Purpose & Scope

MacTrafficLights provides clean, modern circular window control buttons on the **left side** of desktop application windows:
- 🔴 **Red**: Close (sends native `WM_SYSCOMMAND SC_CLOSE` with unsaved document protection)
- 🟡 **Yellow**: Minimize (sends native `WM_SYSCOMMAND SC_MINIMIZE`)
- 🟢 **Green**: Maximize / Restore (checks `IsZoomed` and toggles `SC_MAXIMIZE` / `SC_RESTORE`)

### ⚠️ What This Project Is (and Is NOT)
- ✅ **ONLY** customizes the window control buttons.
- ❌ **NOT** a macOS transformation pack.
- ❌ Does **NOT** touch the Taskbar, Start Menu, Desktop, icons, File Explorer shell themes, fonts, cursor, or wallpapers.
- ❌ Does **NOT** inject DLLs into third-party processes.
- ❌ Does **NOT** patch or modify Windows system files (`uxtheme.dll`, `dwmcore.dll`, etc.).
- ❌ Does **NOT** install kernel drivers or require administrator privileges.

---

## 🛡️ Security & Zero-Injection Architecture

Windows 11 does not expose a public API for third-party processes to reposition or replace caption buttons drawn by the Desktop Window Manager (DWM) or modern applications. 

To achieve the desired appearance **without violating security boundaries**, MacTrafficLights implements an **Out-of-Process Non-Invasive Layered Overlay Architecture**:

1. **Out-of-Process WinEvent Hooks**: Uses `SetWinEventHook` with `WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS`. The hook callbacks execute solely inside `MacTrafficLights.exe`'s own message thread.
2. **Per-Pixel Layered Windows**: Each tracked window has an associated lightweight, borderless `WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE` window that renders antialiased circular buttons via GDI+ and 32-bit ARGB alpha blending.
3. **No Focus Stealing**: Because overlays use `WS_EX_NOACTIVATE`, clicking traffic light buttons executes native commands without interrupting typing focus or active window states.
4. **Drag & Snap Passthrough**: Clicking on the overlay area outside the three circles forwards `WM_NCLBUTTONDOWN HTCAPTION` to the target window, maintaining natural title bar dragging, snapping, and Windows 11 Snap layouts.

---

## ✨ Features

- **🔴 🟡 🟢 Clean macOS-Inspired Design**:
  - Red (`#FF5F56`), Yellow (`#FFBD2E`), Green (`#27C93F`) with subtle borders.
  - Hover states with micro-symbols (`x`, `–`, `+`).
  - Active vs. Inactive dimming (subtle neutral tones when window is in background).
- **Per-Monitor V2 DPI Scaling**:
  - Automatically queries `GetDpiForWindow` to scale diameters, spacing, and margins proportionally across displays (100%, 125%, 150%, 175%, 200%).
- **System Tray Management**:
  - **Enable / Disable**: Instant toggle of all overlays.
  - **Settings**: Real-time sliders for button size, button spacing, left margin, and vertical offset.
  - **Diagnostics**: Real-time CPU %, Working Set RAM, Private Bytes RAM, tracked window count, and active overlay count.
  - **Start with Windows**: Toggleable startup entry in `HKCU\Software\Microsoft\Windows\CurrentVersion\Run`.
  - **Exit**: Cleanly destroys all overlays, unhooks events, and restores 100% normal Windows behavior.
- **Process Exclusions**:
  - Custom exclusion list (e.g. `dwm.exe`, games, or specific tools) configurable via the Settings dialog or `config\settings.ini`.
- **Negligible Idle Resource Usage**:
  - Zero continuous polling or busy loops. When windows are idle, CPU usage sits at `0.00%`. Typical memory working set is ~6 to 12 MB.

---

## 🚀 Getting Started

### Pre-Built Portable Binary
A pre-compiled standalone x64 release executable is available in the root directory:
```
MacTrafficLights.exe
```
Just double-click to launch! Look for the icon in your system tray (near the clock).

---

## 🛠️ Building from Source

### Prerequisites
- Windows 10 (1809+) or Windows 11 (tested on Windows 11 25H2, build 26200+).
- Visual Studio 2022 (C++ Desktop Development workload) **OR** LLVM-MinGW (installed via `winget install MartinStorsjo.LLVM-MinGW.UCRT`).

### Option 1: 1-Click Batch Build (Recommended)
Run `build.bat`:
```cmd
build.bat
```
This automatically compiles resources, compiles `MacTrafficLights.exe` with static linking (`-O3 -static`), and runs the automated smoke test suite.

### Option 2: Visual Studio
1. Open `MacTrafficLights.sln` in Visual Studio 2022.
2. Select **Release** configuration and **x64** platform.
3. Press `Ctrl + Shift + B` to build.

### Option 3: CMake
```cmd
mkdir build
cd build
cmake ..
cmake --build . --config Release
```

---

## 🧪 Testing & Compatibility Matrix

MacTrafficLights was tested and verified across desktop applications:

| Application | Technology | Close (🔴) | Minimize (🟡) | Maximize / Restore (🟢) | Dragging / Snap |
| :--- | :--- | :---: | :---: | :---: | :---: |
| **Notepad** | Modern WinUI / XAML | ✅ | ✅ | ✅ | ✅ |
| **File Explorer** | WinUI 3 Tabs Island | ✅ | ✅ | ✅ | ✅ |
| **Windows Terminal** | DirectX / Custom Frame | ✅ | ✅ | ✅ | ✅ |
| **Google Chrome** | Chromium / Skia | ✅ | ✅ | ✅ | ✅ |
| **Microsoft Edge** | Chromium / WebUI | ✅ | ✅ | ✅ | ✅ |
| **VS Code** | Electron | ✅ | ✅ | ✅ | ✅ |
| **Calculator** | UWP / WinUI | ✅ | ✅ | ✅ | ✅ |
| **Paint** | WinUI 3 / Ribbon | ✅ | ✅ | ✅ | ✅ |
| **Settings** | WinUI 3 Island | ✅ | ✅ | ✅ | ✅ |

### Running Smoke Tests
To run the automated smoke test suite:
```cmd
bin\test_smoke.exe
```

---

## 📂 Project Structure

```
c:\MacTrafficLights-for-windows\
├── MacTrafficLights.exe           # Built standalone Release x64 binary
├── build.bat                      # Automated build & test script
├── uninstall.bat                  # Clean uninstallation script
├── CMakeLists.txt                 # CMake build configuration
├── MacTrafficLights.sln           # Visual Studio solution
├── MacTrafficLights.vcxproj       # Visual Studio project
├── include\                       # C++ header files
│   ├── App.h                      # Application lifecycle & message pump
│   ├── Config.h                   # INI configuration & registry management
│   ├── Diagnostic.h               # CPU, RAM, and window metric collector
│   ├── DiagnosticDialog.h         # Diagnostic GUI dialog
│   ├── Logger.h                   # Local thread-safe file logging
│   ├── NativeActions.h            # Win32 SC_CLOSE, SC_MINIMIZE, SC_MAXIMIZE
│   ├── OverlayManager.h           # WinEvent hook & overlay synchronization
│   ├── OverlayWindow.h            # Layered per-pixel GDI+ rendering & hit-testing
│   ├── Resource.h                 # Resource identifiers
│   ├── SettingsDialog.h           # Settings GUI dialog
│   ├── TrayIcon.h                 # Shell_NotifyIcon system tray handling
│   └── WindowFilter.h             # Window eligibility & safety exclusion rules
├── src\                           # C++ implementations
│   ├── App.cpp
│   ├── Config.cpp
│   ├── Diagnostic.cpp
│   ├── DiagnosticDialog.cpp
│   ├── Logger.cpp
│   ├── main.cpp
│   ├── NativeActions.cpp
│   ├── OverlayManager.cpp
│   ├── OverlayWindow.cpp
│   ├── SettingsDialog.cpp
│   ├── TrayIcon.cpp
│   └── WindowFilter.cpp
├── resources\                     # Icons, manifests, and resource scripts
│   ├── app.ico
│   ├── app.manifest
│   └── MacTrafficLights.rc
├── config\
│   └── settings.ini               # User settings
├── docs\                          # Documentation
│   ├── ARCHITECTURE.md            # In-depth architectural analysis
│   ├── SECURITY.md                # Threat model and zero-injection proof
│   ├── TROUBLESHOOTING.md         # Diagnostic and troubleshooting guide
│   └── UNINSTALL.md               # Clean uninstallation instructions
├── tests\
│   └── test_smoke.cpp             # Automated smoke test suite
├── LICENSE                        # MIT License
└── README.md                      # Documentation
```

---

## 🗑️ Uninstallation

Uninstallation is 100% clean and non-destructive:
1. Run `uninstall.bat` (or right-click the tray icon and click **Exit**).
2. Delete the application folder.

Windows is immediately restored to its default state.

---

## 📜 License
MIT License. See [LICENSE](file:///c:/MacTrafficLights-for-windows/LICENSE) for details.
