# MacTrafficLights Architecture Documentation

## 1. System-Wide Title Bar Customization Feasibility Analysis

A core architectural question for any Windows title-bar customization utility is:
> *Does Windows allow safe system-wide title-bar customization without modifying system files or injecting code into other processes?*

### Windows Window Frame Rendering Internals
1. **Desktop Window Manager (DWM)**:
   - For standard Win32 windows, non-client caption elements (Minimize, Maximize, Close buttons) are drawn and composited directly by the Desktop Window Manager (`dwmcore.dll` / `dwm.exe`).
   - The non-client area is traditionally managed via messages like `WM_NCPAINT`, `WM_NCCALCSIZE`, and `WM_NCHITTEST`.
2. **Modern Custom Title Bars**:
   - Modern Windows 11 applications (such as Windows Terminal, Chrome, Microsoft Edge, VS Code, and WinUI 3 XAML Islands in File Explorer and Settings) do not use traditional non-client buttons. Instead, they extend their client area into the frame using `DwmExtendFrameIntoClientArea` and paint their own custom title bars.
3. **Absence of Public Inter-Process Theming APIs**:
   - Microsoft Windows provides **no public, supported API** for a third-party process to instruct DWM or external applications to change the position, geometry, or appearance of caption buttons across processes.
4. **Forbidden Approaches**:
   - *DLL Injection*: Using `SetWindowsHookEx` with a global hook DLL or `CreateRemoteThread` to inject code into other processes violates security, risks anti-cheat bans in games, and triggers antivirus heuristics.
   - *System File Patching*: Modifying `uxtheme.dll` or `dwmcore.dll` breaks Windows integrity checks, triggers SFC / DISM repairs, and risks system instability upon Windows updates.
   - *Kernel Drivers*: Installing a kernel-mode driver introduces unnecessary system vulnerability and blue-screen risk.

### The Safest Feasible Solution: Event-Driven Non-Invasive Overlay Architecture
Because system files cannot be modified and code cannot be injected into other processes, the safest, production-grade approach is an **out-of-process, non-invasive layered overlay window** synchronized in real time with eligible application windows.

---

## 2. Architecture Overview

```
+-------------------------------------------------------------------------------+
|                             MacTrafficLights.exe                              |
|                                                                               |
|  +----------------------+    +----------------------+    +-----------------+  |
|  |     App Lifecycle    |    |    ConfigManager     |    |  DiagnosticMgr  |  |
|  | (Mutex, DPI, GDI+)   |    | (settings.ini, HKCU) |    | (CPU, RAM, Win) |  |
|  +----------+-----------+    +----------+-----------+    +--------+--------+  |
|             |                           |                         |           |
|  +----------v---------------------------v-------------------------v--------+  |
|  |                           OverlayManager                                |  |
|  |  - WinEvent Hook (WINEVENT_OUTOFCONTEXT)                                |  |
|  |  - Active Overlays Map: std::unordered_map<HWND, OverlayWindow>         |  |
|  +----------+-----------------------------------------------------+--------+  |
|             |                                                     |           |
|  +----------v-----------+                             +-----------v--------+  |
|  |     WindowFilter     |                             |   OverlayWindow    |  |
|  | - Class/Style checks |                             | - Layered WS_EX    |  |
|  | - Exclusions filter  |                             | - GDI+ ARGB render |  |
|  | - Cloaking detection |                             | - Hit-testing      |  |
|  +----------------------+                             +-----------+--------+  |
|                                                                   |           |
|                                                       +-----------v--------+  |
|                                                       |   NativeActions    |  |
|                                                       | - SC_CLOSE         |  |
|                                                       | - SC_MINIMIZE      |  |
|                                                       | - SC_MAXIMIZE      |  |
|                                                       +--------------------+  |
+-------------------------------------------------------------------------------+
```

---

## 3. Key Components & Mechanisms

### 3.1. Out-of-Process WinEvent Hooks (`SetWinEventHook`)
Windows provides `SetWinEventHook` with the `WINEVENT_OUTOFCONTEXT` flag.
- **Out-of-Context Execution**: The hook callback is invoked strictly within `MacTrafficLights.exe`'s own message loop thread. **Zero code or DLL is injected into target processes**.
- **Skip Own Process**: `WINEVENT_SKIPOWNPROCESS` prevents recursive event storms from `MacTrafficLights`'s own overlay windows.
- **Hooked Events**:
  - `EVENT_OBJECT_LOCATIONCHANGE`: Dispatched when target windows move, snap, or resize.
  - `EVENT_SYSTEM_FOREGROUND`: Dispatched when active focus changes, allowing visual transitions (vibrant colors for active windows, subtle dimmed colors for inactive windows).
  - `EVENT_OBJECT_SHOW` / `EVENT_OBJECT_HIDE`: Manages overlay visibility.
  - `EVENT_OBJECT_DESTROY`: Cleans up overlay instances when windows close.
  - `EVENT_SYSTEM_MINIMIZESTART` / `EVENT_SYSTEM_MINIMIZEEND`: Hides overlays during minimize, restores upon restore.
  - `EVENT_OBJECT_CLOAKED` / `EVENT_OBJECT_UNCLOAKED`: Handles Windows 11 virtual desktops and UWP suspension.

### 3.2. Layered Overlay Window (`OverlayWindow`)
Each tracked window has a dedicated, lightweight overlay window:
- **Window Styles**:
  - `WS_POPUP`: Borderless and titleless.
  - `WS_EX_LAYERED`: Enables 32-bit ARGB per-pixel alpha blending via `UpdateLayeredWindow`.
  - `WS_EX_TOOLWINDOW`: Prevents the overlay from appearing in Alt+Tab, Task View, or the taskbar.
  - `WS_EX_NOACTIVATE`: Clicking the traffic lights does **not** steal keyboard focus or window activation from the underlying application.
  - `WS_EX_TOPMOST` / Managed Z-Order: Positioned immediately above the target window.

### 3.3. High-DPI & Frame Geometry
- **Per-Monitor V2 DPI Scaling**:
  The application queries `GetDpiForWindow(targetHwnd)` to dynamically scale button diameter, spacing, and left margin:
  $$\text{scaledPixels} = \text{MulDiv}(\text{basePixels}, \text{dpi}, 96)$$
- **Maximized Window Margins**:
  On Windows 11, maximized windows extend slightly beyond visible display bounds (`SM_CXSIZEFRAME` + `SM_CXPADDEDBORDER`). The overlay automatically offsets its coordinates so buttons remain perfectly aligned with the visible title bar.

### 3.4. Native Window Actions (`NativeActions`)
The traffic light buttons trigger standard, genuine Windows messages:
- **🔴 Red (Close)**:
  Dispatches `WM_SYSCOMMAND` with `SC_CLOSE` to the target window. If the application has unsaved files (e.g. Notepad, Word, VS Code), it prompts the user normally.
- **🟡 Yellow (Minimize)**:
  Dispatches `WM_SYSCOMMAND` with `SC_MINIMIZE`.
- **🟢 Green (Maximize / Restore)**:
  Checks `IsZoomed(targetHwnd)`. If maximized, dispatches `SC_RESTORE`; otherwise, dispatches `SC_MAXIMIZE`.
- **Title Bar Drag Passthrough**:
  If the user clicks on the overlay surface outside the 3 button circles, `NativeActions::ForwardTitleBarDrag` forwards the mouse interaction to the target window with `WM_NCLBUTTONDOWN` and `HTCAPTION`, preserving native window dragging and snap layouts!

---

## 4. Performance & Resource Characteristics

- **Event-Driven Architecture**: When windows are stationary and idle, `SetWinEventHook` generates zero events. The message loop blocks in `GetMessageW`, consuming negligible CPU.
- **Double-Buffered Alpha Rendering**: Overlays update their bitmap using in-memory 32-bit DIB sections only when moving, resizing, or changing hover/focus states.
- **Memory Footprint**: Typically 6 to 12 MB working set for tracking dozens of open application windows.
