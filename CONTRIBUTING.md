# Contributing to MacTrafficLights

Thank you for your interest in contributing to **MacTrafficLights**!

## Safety & Architectural Rules (Mandatory)

All contributions must strictly adhere to the project's core safety invariants:
1. **Never inject DLLs or code into other processes**.
2. **Never modify Windows system files or system DLLs**.
3. **Never require administrator privileges** (`asInvoker` must be preserved).
4. **Never install kernel drivers**.
5. **Never collect telemetry or send data over the internet**.
6. **Remain event-driven** (no busy loops or continuous polling).

## Development Setup

### Option 1: Batch Build (LLVM-MinGW)
```cmd
build.bat
```

### Option 2: Visual Studio 2022
Open `MacTrafficLights.sln`, select `Release x64`, and press `Ctrl+Shift+B`.

### Option 3: CMake
```cmd
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

## Running Tests
Always verify that the automated smoke test suite passes:
```cmd
bin\test_smoke.exe
```

## Submitting a Pull Request
1. Fork the repository.
2. Create a feature branch: `git checkout -b feature/my-new-feature`.
3. Commit your changes: `git commit -am 'Add new feature'`.
4. Push to the branch: `git push origin feature/my-new-feature`.
5. Open a Pull Request.
