# Lumos Design Document

**Date:** 2026-01-31 (Updated: post-v0.1 release)
**Project:** Lumos - C++ reimplementation of Gamminator
**License:** GPL v2
**Target:** Windows 10+

## Overview

Lumos is a Windows system tray utility for monitor gamma adjustment, reimplementing the Delphi-based Gamminator project in modern C++ with ImGui.

### Attribution

Lumos is a reimplementation of **Gamminator**, originally created by **Wolfgang Freiler** (2005), with multi-monitor support added by **Lady Eklipse** (v0.5.7). The original project is available at [SourceForge](http://sourceforge.net/projects/gamminator).

As a derivative work of GPL v2 software, Lumos is also licensed under GPL v2.

### Goals

- Direct feature parity with Gamminator 0.5.7
- Modern C++ codebase (C++20)
- Single executable, no installer required
- Foundation for future features (NVIDIA hooks, LUTs, HDR)

## Feature Parity Matrix

| Feature | Gamminator | Lumos v0.1 | Lumos v0.2 | Lumos v1.0 |
|---------|------------|------------|------------|------------|
| System tray icon | ✓ | ✓ | ✓ | ✓ |
| Popup menu (Open/Reset/Exit) | ✓ | ✓ | ✓ | ✓ |
| Gamma slider (0.1-9.0) | ✓ | ✓ | ✓ | ✓ |
| Multi-monitor support | ✓ | - | ✓ | ✓ |
| Global hotkeys | ✓ | - | ✓ | ✓ |
| CLI interface | ✓ | - | ✓ | ✓ |
| Config persistence | ✓ | ✓ | ✓ | ✓ |
| Crash-safe gamma restore | ✓ | ✓ | ✓ | ✓ |
| Hide to tray on close | ✓ | ✓ | ✓ | ✓ |
| Gamma curve visualization | ✓ | - | - | ✓ |
| Test pattern display | ✓ | - | - | ✓ |
| About dialog | ✓ | - | ✓ | ✓ |
| Hotkey customization UI | ✓ | - | - | ✓ |
| Auto-calibration | ✓ | - | - | ✓ |
| Help dialog | ✓ | - | ✓ | ✓ |

## Architecture

```
┌─────────────────────────────────────────────────────────┐
│                      main.cpp                           │
│            Win32 entry point, message loop              │
└─────────────────────┬───────────────────────────────────┘
                      │
┌─────────────────────▼───────────────────────────────────┐
│                      app.cpp                            │
│         Application state, orchestration                │
├─────────────┬─────────────┬─────────────┬───────────────┤
│   config    │   platform/ │     ui/     │               │
│   .cpp/h    │   win32/    │  (ImGui)    │               │
│             │             │             │               │
│ INI read/   │ gamma.cpp   │ main_window │               │
│ write,      │ tray.cpp    │ .cpp/h      │               │
│ settings    │ hotkeys.cpp │             │               │
└─────────────┴─────────────┴─────────────┴───────────────┘
```

**Core principle:** Platform code (Win32 APIs) is isolated in `platform/win32/`. UI code (ImGui) is isolated in `ui/`. The `app` layer coordinates between them but contains no platform-specific code.

## Milestone Roadmap

### v0.1 "It works" - Core foundation ✅ RELEASED

- ✅ Tray app launches (no console window)
- ✅ System tray icon with popup menu (Open UI, Reset, Exit)
- ✅ ImGui window opens from tray
- ✅ Gamma slider (0.1 - 9.0 range) adjusts gamma live
- ✅ Applies to primary display only
- ✅ Stores original gamma ramp on startup
- ✅ Restores original gamma on exit (and Reset button)
- ✅ INI config stores last gamma value
- ✅ Crash safety: attempt restore via `SetUnhandledExceptionFilter`

### v0.2 "Usable daily" - Power user features

- Global hotkeys (Ctrl+Alt+Up/Down/R)
- Multi-monitor support (enumerate displays, apply to all)
- CLI: `lumos 1.2` sets gamma and exits, `lumos` opens GUI
- About dialog (attribution, version, license, project link)
- Help dialog (usage instructions, hotkey reference)

### v0.3 "Polish" - Verification tools

- Test pattern display (striped black/white calibration pattern)
- Real-time gamma curve visualization
- Slider tick marks at common gamma values

### v1.0 "Parity" - Full Gamminator feature set

- Auto-calibration (detect system's valid gamma range)
- Options dialog (hotkey customization UI)
- All INI settings (hotkey bindings, calibration bounds)

### Future (post-v1.0)

- NVIDIA driver hooks
- LUT support / 3D LUTs
- Per-game profiles
- HDR awareness

## Project Structure

```
lumos/
├── CMakeLists.txt
├── LICENSE                    # GPL v2 text
├── README.md
├── .gitignore
├── src/
│   ├── main.cpp              # WinMain, message loop, ImGui init
│   ├── app.cpp/h             # Application state machine
│   ├── cli.cpp/h             # Command line interface (v0.2)
│   ├── config.cpp/h          # INI read/write
│   ├── platform/
│   │   └── win32/
│   │       ├── gamma.cpp/h   # GetDeviceGammaRamp/SetDeviceGammaRamp
│   │       ├── hotkeys.cpp/h # Global hotkey registration (v0.2)
│   │       └── tray.cpp/h    # Shell_NotifyIcon, popup menu
│   └── ui/
│       ├── main_window.cpp/h # Main gamma slider UI
│       ├── about_dialog.cpp/h # About dialog (v0.2)
│       └── help_dialog.cpp/h  # Help dialog (v0.2)
├── third_party/
│   └── imgui/                # Git submodule
├── resources/
│   ├── icon.ico              # App icon
│   └── lumos.rc              # Windows resource file
└── .github/
    └── workflows/
        └── release.yml       # GitHub Actions CI
```

## Technical Specifications

### Dependencies

- **ImGui** (git submodule) - UI rendering
- **Win32 APIs** - gamma, tray, window management
- **DirectX 11** - ImGui backend (lightweight, ships with Windows)

### Build Requirements

- CMake 3.20+
- Visual Studio 2022 Build Tools (or full VS)
- Windows SDK 10.0.19041+

### INI Configuration

**Config location:** `%APPDATA%\Lumos\lumos.ini`

**v0.1 format:**
```ini
[Gamma]
LastValue=1.0
```

**v1.0 format (planned):**
```ini
[Gamma]
LastValue=1.0

[Hotkeys]
Increase=Ctrl+Alt+Up
Decrease=Ctrl+Alt+Down
Reset=Ctrl+Alt+R

[Calibration]
Lower=100
Upper=910
```

## Safety & Crash Handling

### The Problem

If Lumos crashes without restoring the original gamma ramp, the user's display stays at the modified gamma until reboot.

### Safety Measures

1. **Structured cleanup:**
   - `std::atexit()` handler
   - `std::signal(SIGTERM)` handler

2. **Unhandled exception filter:**
   - `SetUnhandledExceptionFilter` for best-effort restore before crash

3. **Defensive coding:**
   - Always capture original ramp before any modification
   - Validate gamma values are within 0.1 - 9.0 before applying
   - If `SetDeviceGammaRamp` fails, don't update internal state

### Limitations

- `taskkill /F` or power loss cannot be caught
- User's only recourse: reboot (which resets gamma automatically)

## References

- **Original project:** Gamminator by Wolfgang Freiler
- **Multi-monitor mod:** Lady Eklipse (v0.5.7)
- **SourceForge:** http://sourceforge.net/projects/gamminator
- **License:** GPL v2 - https://www.gnu.org/licenses/old-licenses/gpl-2.0.html
