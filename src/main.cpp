#include <windows.h>
#include "App.h"

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    return MacTrafficLights::App::Instance().Run(hInstance, nCmdShow);
}

// Fallback for standard entry point if compiled with console subsystem
int main(int argc, char* argv[]) {
    return MacTrafficLights::App::Instance().Run(GetModuleHandleW(NULL), SW_SHOWDEFAULT);
}
