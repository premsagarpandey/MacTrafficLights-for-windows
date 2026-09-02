@echo off
setlocal

echo ===================================================
echo     MacTrafficLights Clean Uninstallation
echo ===================================================

:: 1. Terminate running instance
echo [1/3] Closing running instances of MacTrafficLights.exe...
taskkill /f /im MacTrafficLights.exe >nul 2>&1

:: 2. Remove startup entry from HKCU Run
echo [2/3] Removing startup registry entry if present...
reg delete "HKCU\Software\Microsoft\Windows\CurrentVersion\Run" /v "MacTrafficLights" /f >nul 2>&1

:: 3. Clean up config and log files
echo [3/3] Removing application configuration and logs...
if exist "config\settings.ini" del /q "config\settings.ini"
if exist "MacTrafficLights.log" del /q "MacTrafficLights.log"

echo.
echo ===================================================
echo   Uninstallation complete!
echo   All normal Windows window behavior is restored.
echo   You may now delete this folder.
echo ===================================================
pause
