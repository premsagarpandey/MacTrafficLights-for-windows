# MacTrafficLights Uninstallation Guide

MacTrafficLights does not modify any system files, does not register system services, and does not install kernel drivers. Uninstallation is completely clean, risk-free, and leaves Windows in its 100% original state.

---

## Method 1: Automated Uninstallation Script

Run `uninstall.bat` directly from the application folder.

```cmd
uninstall.bat
```

The script performs the following operations:
1. Gracefully terminates running instances of `MacTrafficLights.exe`.
2. Removes the startup registry key from `HKCU\Software\Microsoft\Windows\CurrentVersion\Run\MacTrafficLights` if enabled.
3. Removes temporary configuration and log files (`config\settings.ini`, `MacTrafficLights.log`).
4. Confirms that no system files or settings were altered.

---

## Method 2: Manual Uninstallation Steps

If you prefer to uninstall manually:

### Step 1: Exit the Application
Right-click the **MacTrafficLights** icon in the system tray and select **Exit**.
All traffic light overlays will immediately close, restoring standard Windows title bars.

### Step 2: Remove the Startup Entry (if enabled)
If you enabled "Start with Windows":
1. Press `Win + R`, type `regedit`, and press Enter.
2. Navigate to:
   ```
   HKEY_CURRENT_USER\Software\Microsoft\Windows\CurrentVersion\Run
   ```
3. Delete the value named `MacTrafficLights`.

Alternatively, via PowerShell:
```powershell
Remove-ItemProperty -Path "HKCU:\Software\Microsoft\Windows\CurrentVersion\Run" -Name "MacTrafficLights" -ErrorAction SilentlyContinue
```

### Step 3: Delete the Application Folder
Simply delete the directory containing `MacTrafficLights.exe`.
No leftover files, driver services, or background daemons will remain on your computer.
