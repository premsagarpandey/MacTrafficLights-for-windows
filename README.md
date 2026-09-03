# MacTrafficLights for Windows 11

[![CI](https://github.com/premsagarpandey/MacTrafficLights-for-windows/actions/workflows/ci.yml/badge.svg)](https://github.com/premsagarpandey/MacTrafficLights-for-windows/actions)
![Platform: Windows 11](https://img.shields.io/badge/Platform-Windows%2011%20x64-blue.svg)
![Language: C++17](https://img.shields.io/badge/Language-C%2B%2B17%20%2F%20C%2B%2B20-brightgreen.svg)
![Architecture: Zero-Injection](https://img.shields.io/badge/Architecture-Zero--Injection-success.svg)
![License: MIT](https://img.shields.io/badge/License-MIT-green.svg)

A lightweight, zero-injection native Windows utility that renders clean circular window control buttons directly over Windows title bars:

```
+---------------------------------------------------------------------------------+
|   Document - Notepad                                            (🟢) (🟡) (🔴)  |
+---------------------------------------------------------------------------------+
|                                                                                 |
|   Top-Right Window Caption Controls:                                            |
|   🟢 Minimize | 🟡 Maximize / Restore | 🔴 Close                                |
|                                                                                 |
+---------------------------------------------------------------------------------+
```

---

## 🎯 Purpose & Scope

MacTrafficLights replaces standard rectangular caption buttons with modern antialiased circular buttons:
- 🟢 **Green Circle**: Minimize (sends native `WM_SYSCOMMAND SC_MINIMIZE`)
- 🟡 **Yellow Circle**: Maximize / Restore (checks `IsZoomed` and toggles `SC_MAXIMIZE` / `SC_RESTORE`)
- 🔴 **Red Circle**: Close (sends native `WM_SYSCOMMAND SC_CLOSE` with unsaved document protection)

### ⚠️ What This Project Is (and Is NOT)
- ✅ **ONLY** customizes the window control buttons.
- ❌ **NOT** a transformation pack.
- ❌ Does **NOT** touch Taskbar, Start Menu, Desktop, icons, fonts, cursor, or wallpapers.
- ❌ Does **NOT** inject DLLs into third-party processes.
- ❌ Does **NOT** patch Windows system files.
- ❌ Does **NOT** require administrator privileges.

---

## 🛡️ Zero-Injection Architecture

1. **Out-of-Process WinEvent Hooks**: Uses `SetWinEventHook` with `WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS`. Callbacks execute solely inside `MacTrafficLights.exe`.
2. **Per-Pixel Layered Windows**: Each tracked window has a single lightweight `WS_EX_LAYERED | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE` window that renders antialiased circular buttons via GDI+ and 32-bit ARGB alpha blending.
3. **No Focus Stealing**: Because overlays use `WS_EX_NOACTIVATE`, clicking buttons executes native commands without interrupting active typing focus.
4. **Drag & Snap Passthrough**: Clicking or dragging on the overlay area outside the three circles forwards `WM_NCLBUTTONDOWN HTCAPTION` to the target window, preserving title bar dragging, snapping, and Windows 11 Snap layouts.

---

## ✨ Features

- **🟢 🟢 🔴 Clean Circular Design**:
  - Green (`#27C93F`), Green (`#27C93F`), Red (`#FF5F56`) with smooth borders.
  - Hover highlights and press effects.
  - Inactive dimming when windows are in the background.
- **Per-Monitor V2 DPI Scaling**:
  - Proportional scaling across all display scale factors (100%, 125%, 150%, 200%).
- **Minimal System Tray**:
  - **Enable / Disable**: Toggle overlays on or off.
  - **Exit**: Cleanly destroys overlays and unhooks events.
- **Negligible Resource Usage**:
  - 0% CPU at idle, ~4-8 MB memory footprint.

---

## 🚀 Getting Started

Launch the standalone executable:
```cmd
MacTrafficLights.exe
```
Look for the icon in your system tray (near the clock). Right-click the tray icon to enable, disable, or exit.

---

## 🛠️ Building from Source

### 1-Click Batch Build
Run `build.bat`:
```cmd
build.bat
```
This automatically compiles resources and compiles `MacTrafficLights.exe` using static linking (`-O3 -static`).

---

## 📄 License

Distributed under the MIT License. See `LICENSE` for details.
