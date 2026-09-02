# Security Policy

## Supported Versions

| Version | Supported          |
| ------- | ------------------ |
| 1.0.x   | :white_check_mark: |

## Core Safety Guarantees

MacTrafficLights was designed with strict security invariants:
- **Zero DLL injection**: Uses `SetWinEventHook` with `WINEVENT_OUTOFCONTEXT`. No code is injected into foreign processes.
- **Zero system modification**: Does not modify, replace, or patch any Windows system DLLs (`uxtheme.dll`, `dwmcore.dll`, etc.).
- **Zero kernel drivers**: Operates entirely in user mode.
- **No administrator rights required**: Declared as `asInvoker`.
- **Zero telemetry or internet connection**: Does not connect to the network.

For full architectural details, see [docs/SECURITY.md](docs/SECURITY.md) and [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md).

## Reporting a Vulnerability

If you discover a security vulnerability, please do NOT file a public issue.
Instead, email the maintainers or use GitHub's private vulnerability reporting feature.
