#pragma once

#include <windows.h>
#include <gdiplus.h>

namespace MacTrafficLights {

class App {
public:
    static App& Instance();

    int Run(HINSTANCE hInstance, int nCmdShow);

private:
    App();
    ~App();

    bool InitializeDpiAwareness();
    bool EnsureSingleInstance();

    HINSTANCE m_hInstance = NULL;
    HANDLE m_hSingleInstanceMutex = NULL;
    ULONG_PTR m_gdiplusToken = 0;
};

} // namespace MacTrafficLights
