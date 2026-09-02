cmake_minimum_required(VERSION 3.15)
project(MacTrafficLights LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DUNICODE -D_UNICODE)

include_directories(include)

set(SOURCES
    src/main.cpp
    src/App.cpp
    src/Config.cpp
    src/Logger.cpp
    src/Diagnostic.cpp
    src/NativeActions.cpp
    src/WindowFilter.cpp
    src/OverlayWindow.cpp
    src/OverlayManager.cpp
    src/TrayIcon.cpp
    src/SettingsDialog.cpp
    src/DiagnosticDialog.cpp
    resources/MacTrafficLights.rc
)

add_executable(MacTrafficLights WIN32 ${SOURCES})

target_link_libraries(MacTrafficLights PRIVATE
    gdi32
    user32
    shell32
    dwmapi
    gdiplus
    comctl32
    psapi
    advapi32
)

# Test target
add_executable(test_smoke
    tests/test_smoke.cpp
    src/Config.cpp
    src/Logger.cpp
    src/Diagnostic.cpp
    src/NativeActions.cpp
    src/WindowFilter.cpp
)

target_link_libraries(test_smoke PRIVATE
    gdi32
    user32
    shell32
    dwmapi
    psapi
    advapi32
)
