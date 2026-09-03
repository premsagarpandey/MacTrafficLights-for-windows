@echo off
setlocal enabledelayedexpansion

echo ===================================================
echo   Building MacTrafficLights for Windows 11 (x64)
echo ===================================================

:: 1. Locate 64-bit LLVM MinGW compiler
set "COMPILER="
set "WINDRES="

set "WINGET_LLVM=%LOCALAPPDATA%\Microsoft\WinGet\Packages\MartinStorsjo.LLVM-MinGW.UCRT_Microsoft.Winget.Source_8wekyb3d8bbwe\llvm-mingw-20260616-ucrt-x86_64\bin"

if exist "%WINGET_LLVM%\x86_64-w64-mingw32-clang++.exe" (
    set "COMPILER=%WINGET_LLVM%\x86_64-w64-mingw32-clang++.exe"
    set "WINDRES=%WINGET_LLVM%\llvm-windres.exe"
)

if not defined COMPILER (
    for /f "delims=" %%F in ('dir /b /s "%LOCALAPPDATA%\Microsoft\WinGet\Packages\x86_64-w64-mingw32-clang++.exe" 2^>nul') do (
        set "COMPILER=%%F"
        set "WINDRES=%%~dpFllvm-windres.exe"
    )
)

if not defined COMPILER (
    where x86_64-w64-mingw32-clang++ >nul 2>&1
    if !errorlevel! equ 0 (
        set "COMPILER=x86_64-w64-mingw32-clang++"
        set "WINDRES=llvm-windres"
    )
)

if not defined COMPILER (
    echo [ERROR] No suitable 64-bit C++ compiler found!
    exit /b 1
)

echo [INFO] Using compiler: %COMPILER%
echo [INFO] Using windres:  %WINDRES%

if not exist bin mkdir bin
if not exist config mkdir config

:: 2. Compile Windows resources
echo [INFO] Compiling Windows resource script...
"%WINDRES%" -I include -i resources/MacTrafficLights.rc -O coff -o bin/MacTrafficLights.res.o
if %errorlevel% neq 0 (
    echo [ERROR] Resource compilation failed.
    exit /b %errorlevel%
)

:: 3. Compile MacTrafficLights executable (Standalone / Portable static build)
echo [INFO] Compiling standalone MacTrafficLights.exe (Release x64)...
"%COMPILER%" -std=c++17 -O3 -static -mwindows -DUNICODE -D_UNICODE ^
    -I include ^
    src/main.cpp ^
    src/App.cpp ^
    src/Config.cpp ^
    src/Logger.cpp ^
    src/NativeActions.cpp ^
    src/WindowFilter.cpp ^
    src/OverlayWindow.cpp ^
    src/OverlayManager.cpp ^
    src/TrayIcon.cpp ^
    bin/MacTrafficLights.res.o ^
    -lgdi32 -luser32 -lshell32 -ldwmapi -lgdiplus -lcomctl32 -lpsapi -ladvapi32 ^
    -o bin/MacTrafficLights.exe

if %errorlevel% neq 0 (
    echo [ERROR] Build failed!
    exit /b %errorlevel%
)

copy /y bin\MacTrafficLights.exe MacTrafficLights.exe >nul
echo [SUCCESS] Built standalone MacTrafficLights.exe successfully!

:: 4. Build and run smoke tests
echo.
echo [INFO] Compiling and running Smoke Test Suite...
"%COMPILER%" -std=c++17 -O2 -static -DUNICODE -D_UNICODE ^
    -I include ^
    tests/test_smoke.cpp ^
    src/Config.cpp ^
    src/Logger.cpp ^
    src/NativeActions.cpp ^
    src/WindowFilter.cpp ^
    -lgdi32 -luser32 -lshell32 -ldwmapi -lpsapi -ladvapi32 ^
    -o bin/test_smoke.exe

if %errorlevel% equ 0 (
    bin\test_smoke.exe
) else (
    echo [WARN] Test compilation failed.
)

echo.
echo ===================================================
echo   Build complete! Output: MacTrafficLights.exe
echo ===================================================
