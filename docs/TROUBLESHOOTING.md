# MacTrafficLights Troubleshooting Guide

## 1. Known Limitations & Expected Behaviors

### 1.1. Elevated (Administrator) Windows & UIPI
- **Behavior**: If an application (e.g. Command Prompt or Registry Editor) is launched with **"Run as Administrator"**, the traffic lights may not appear on its title bar when MacTrafficLights is running under a standard user account.
- **Cause**: Windows enforces **User Interface Privilege Isolation (UIPI)**. Standard user processes are prevented by Windows security from positioning windows or sending messages to higher-integrity processes.
- **Solution**: To decorate elevated windows, launch `MacTrafficLights.exe` with administrator privileges. However, for normal daily usage, running as standard user is recommended for maximum security.

### 1.2. Exclusive Fullscreen Games
- **Behavior**: Games running in exclusive fullscreen mode bypass standard Desktop Window Manager (DWM) composition.
- **Solution**: Add the game's executable name (e.g. `cyberpunk2077.exe`) to the **Excluded Processes** list in the **Settings** dialog.

### 1.3. Proprietary Borderless Window Themes
- Some applications (e.g. Spotify, custom game launchers) employ custom client-area drag regions with irregular title-bar heights.
- **Solution**: Use the **Settings** dialog to fine-tune the **Vertical Title Bar Offset** or **Left Margin** to match your preferred position.

---

## 2. Common Issues & Solutions

### Issue: The application says "MacTrafficLights is already running"
- **Explanation**: MacTrafficLights enforces a single instance via a system mutex to prevent duplicate overlays.
- **Solution**: Check the Windows Notification Area (System Tray, near the taskbar clock). If the icon is hidden, click the small up-arrow (`^`) on the taskbar.

### Issue: Overlays lag slightly when moving a window very quickly
- **Explanation**: Standard Win32 `SetWindowPos` asynchronously follows the target window frame.
- **Solution**: Ensure your graphics drivers are up to date and Hardware-Accelerated GPU Scheduling is enabled in Windows Settings -> Display -> Graphics.

### Issue: Checking CPU and RAM usage
- Open the System Tray menu -> Click **Diagnostics...** (or double-click the tray icon).
- The diagnostic window will display real-time CPU percentage, Working Set RAM, and the count of active tracked windows. Idle CPU should remain virtually 0.00%.

### Issue: How to view diagnostic log files
- Logs are automatically recorded to `MacTrafficLights.log` in the application directory.
- Review this file to inspect detected window events and any handled exceptions.
