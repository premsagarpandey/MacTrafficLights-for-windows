# MacTrafficLights Security & Safety Documentation

MacTrafficLights was designed from the ground up with strict security and system stability guarantees.

---

## 1. Core Safety Guarantees

| Requirement | Guarantee | Technical Implementation |
| :--- | :--- | :--- |
| **No System DLL Modification** | **100% Guaranteed** | Never modifies, patches, or deletes any Windows DLLs (`uxtheme.dll`, `dwmcore.dll`, `user32.dll`, etc.). |
| **No System File Modification** | **100% Guaranteed** | All operational files reside solely in the application's own directory. |
| **No DLL Injection** | **100% Guaranteed** | Uses `SetWinEventHook` with `WINEVENT_OUTOFCONTEXT`. No hook DLLs are loaded into target processes. |
| **No Code Injection** | **100% Guaranteed** | Never calls `CreateRemoteThread`, `WriteProcessMemory`, or `VirtualAllocEx`. |
| **No Kernel Driver** | **100% Guaranteed** | Operates strictly in user mode. No `.sys` driver files are installed or bundled. |
| **No Unsafe Registry Hacks** | **100% Guaranteed** | Only optionally writes to standard `HKCU\Software\Microsoft\Windows\CurrentVersion\Run` when explicitly toggled by user. |
| **No Administrator Privileges** | **100% Guaranteed** | Manifest declares `<requestedExecutionLevel level="asInvoker" />`. Runs under standard user rights. |
| **No Telemetry or Internet Calls** | **100% Guaranteed** | Does not link against `wininet.dll`, `winhttp.dll`, or socket libraries. Never connects to the internet. |

---

## 2. Zero-Injection Verification Guide

To independently verify that `MacTrafficLights.exe` does not perform DLL injection or code injection:

### 1. Verify Imported Libraries
Run `llvm-readobj` or `dumpbin` on `MacTrafficLights.exe`:
```cmd
llvm-readobj --needed-libs MacTrafficLights.exe
```
Expected output shows only standard Windows libraries:
- `KERNEL32.dll`
- `USER32.dll`
- `GDI32.dll`
- `SHELL32.dll`
- `dwmapi.dll`
- `gdiplus.dll`
- `comctl32.dll`
- `psapi.dll`
- `ADVAPI32.dll`

Notice the absence of network libraries (`ws2_32.dll`, `wininet.dll`, `winhttp.dll`).

### 2. Verify WinEvent Hook Context
Inspect `src/OverlayManager.cpp`:
```cpp
m_hHookLocation = SetWinEventHook(
    EVENT_OBJECT_LOCATIONCHANGE,
    EVENT_OBJECT_LOCATIONCHANGE,
    NULL,                     // No DLL module handle!
    WinEventProc,             // Local callback in our process!
    0, 0,
    WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS // Out of context execution
);
```
Passing `NULL` as the module handle and `WINEVENT_OUTOFCONTEXT` ensures Windows invokes the callback solely on `MacTrafficLights.exe`'s thread.

### 3. Verify Process Integrity with Process Explorer
1. Open Sysinternals **Process Explorer**.
2. Select any running application (e.g. `notepad.exe` or `chrome.exe`).
3. View the list of loaded DLLs (`Ctrl+D`).
4. Search for `MacTrafficLights`: **0 matching DLLs will be found**.

---

## 3. Privacy Policy
- **Zero data collection**: MacTrafficLights collects no telemetry, analytics, personal information, or usage data.
- **Zero internet access**: The binary makes no outbound network connections.
- **Local diagnostic logs**: If enabled, logs are written exclusively to `MacTrafficLights.log` on the local machine.
