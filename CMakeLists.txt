cmake_minimum_required(VERSION 3.15)
project(MacTrafficLights LANGUAGES CXX RC)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_definitions(-DUNICODE -D_UNICODE)

# Ensure RC compiler finds headers and icons in include and resources directories
set(CMAKE_RC_FLAGS "${CMAKE_RC_FLAGS} /I \"${CMAKE_CURRENT_SOURCE_DIR}/include\" /I \"${CMAKE_CURRENT_SOURCE_DIR}/resources\"")

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

target_include_directories(MacTrafficLights PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/resources
)

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

# Smoke test suite
add_executable(test_smoke
    tests/test_smoke.cpp
    src/Config.cpp
    src/Logger.cpp
    src/Diagnostic.cpp
    src/NativeActions.cpp
    src/WindowFilter.cpp
)

target_include_directories(test_smoke PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(test_smoke PRIVATE
    gdi32
    user32
    shell32
    dwmapi
    psapi
    advapi32
)
