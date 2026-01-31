# Lumos Design Document

**Date:** 2026-01-31
**Project:** Lumos - C++ reimplementation of Gamminator
**License:** GPL v2
**Target:** Windows 10+

## Overview

Lumos is a Windows system tray utility for monitor gamma adjustment, reimplementing the Delphi-based Gamminator project in modern C++ with ImGui.

### Goals

- Direct feature parity with Gamminator
- Modern C++ codebase (C++20)
- Single executable, no installer required
- Foundation for future features (NVIDIA hooks, LUTs, HDR)

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

### v0.1 "It works" - Core foundation

- Tray app launches (no console window)
- System tray icon with popup menu (Open UI, Reset, Exit)
- ImGui window opens from tray
- Gamma slider (0.1 - 9.0 range) adjusts gamma live
- Applies to primary display only
- Stores original gamma ramp on startup
- Restores original gamma on exit (and Reset button)
- INI config stores last gamma value
- Crash safety: attempt restore via `SetUnhandledExceptionFilter`

### v0.2 "Usable daily" - Power user features

- Global hotkeys (increase/decrease/reset)
- Multi-monitor support (enumerate displays, apply to all)
- CLI: `lumos 1.2` sets gamma and exits, `lumos` opens GUI

### v0.3 "Polish" - Verification tools

- Test pattern display (striped black/white)
- Real-time gamma curve visualization

### v1.0 "Parity" - Full Gamminator feature set

- Auto-calibration (detect system's valid gamma range)
- All remaining INI settings (hotkey bindings, calibration bounds)

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
│   ├── config.cpp/h          # INI read/write
│   ├── platform/
│   │   └── win32/
│   │       ├── gamma.cpp/h   # GetDeviceGammaRamp/SetDeviceGammaRamp
│   │       └── tray.cpp/h    # Shell_NotifyIcon, popup menu
│   └── ui/
│       └── main_window.cpp/h # ImGui slider, reset button
├── third_party/
│   └── imgui/                # Git submodule
├── resources/
│   ├── icon.ico              # App icon
│   └── lumos.rc              # Windows resource file
└── .github/
    └── workflows/
        └── build.yml         # GitHub Actions CI
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

### Module Interfaces

#### Gamma Module (`platform/win32/gamma.cpp`)

```cpp
namespace lumos::platform {

struct GammaRamp {
    WORD red[256];
    WORD green[256];
    WORD blue[256];
};

class Gamma {
public:
    bool captureOriginal();          // Store current ramp on startup
    bool restoreOriginal();          // Restore saved ramp
    bool apply(double value);        // Set gamma (0.1 - 9.0)
    double read();                   // Read current gamma from primary

private:
    GammaRamp original_ramp_;
    bool has_original_ = false;
};

}
```

**Gamma calculation:** For a gamma value `g`, each ramp entry `i` (0-255) is:

```
value = pow(i / 255.0, 1.0 / g) * 65535.0
```

#### Tray Module (`platform/win32/tray.cpp`)

```cpp
namespace lumos::platform {

class Tray {
public:
    bool create(HWND parent, UINT callback_msg);
    void destroy();
    void showMenu();                 // Popup: Open UI, Reset, Exit

    std::function<void()> on_open;
    std::function<void()> on_reset;
    std::function<void()> on_exit;
};

}
```

#### App Class (`app.cpp`)

```cpp
namespace lumos {

class App {
public:
    bool initialize();               // Load config, capture gamma, create tray
    void shutdown();                 // Restore gamma, save config
    void run();                      // Main loop

    void setGamma(double value);     // Apply + update state
    void resetGamma();               // Restore original
    double getGamma() const;

    void showWindow();
    void hideWindow();
    void requestExit();

private:
    double current_gamma_ = 1.0;
    bool window_visible_ = false;
    bool running_ = true;

    Config config_;
    platform::Gamma gamma_;
    platform::Tray tray_;
};

}
```

#### Config Module (`config.cpp`)

```cpp
namespace lumos {

class Config {
public:
    bool load();                     // From %APPDATA%\Lumos\lumos.ini
    bool save();

    double last_gamma = 1.0;

private:
    std::filesystem::path getConfigPath();
};

}
```

**Config location:** `%APPDATA%\Lumos\lumos.ini`

**INI format (v0.1):**

```ini
[Gamma]
LastValue=1.0
```

### UI Layout (v0.1)

```
┌─────────────────────────────────┐
│  Lumos                      [X] │
├─────────────────────────────────┤
│                                 │
│  Gamma: [=======|====] 1.45     │
│                                 │
│  [ Reset to Default ]           │
│                                 │
│  Status: Applied to 1 display   │
└─────────────────────────────────┘
```

- Window size: ~350x200 pixels
- Slider: `ImGui::SliderFloat` with range 0.1 - 9.0
- Applies gamma on slider change (immediate feedback)
- Closing window hides it (doesn't exit app)

## Safety & Crash Handling

### The Problem

If Lumos crashes without restoring the original gamma ramp, the user's display stays at the modified gamma until reboot.

### Safety Measures

1. **Structured cleanup:**
   - `std::atexit()` handler
   - `std::signal(SIGTERM)` handler

2. **Unhandled exception filter:**
   - `SetUnhandledExceptionFilter` for best-effort restore before crash

3. **Console Ctrl handler (dev builds):**
   - `SetConsoleCtrlHandler` for Ctrl+C

4. **Defensive coding:**
   - Always capture original ramp before any modification
   - Validate gamma values are within 0.1 - 9.0 before applying
   - If `SetDeviceGammaRamp` fails, don't update internal state

### Limitations

- `taskkill /F` or power loss cannot be caught
- User's only recourse: reboot (which resets gamma automatically)

## Build System

### CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.20)
project(lumos VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(third_party/imgui)

add_executable(lumos WIN32
    src/main.cpp
    src/app.cpp
    src/config.cpp
    src/platform/win32/gamma.cpp
    src/platform/win32/tray.cpp
    src/ui/main_window.cpp
    resources/lumos.rc
)

target_link_libraries(lumos PRIVATE
    imgui
    d3d11 dxgi
    dwmapi
)
```

### GitHub Actions

```yaml
on: [push, pull_request]
jobs:
  build:
    runs-on: windows-latest
    steps:
      - uses: actions/checkout@v4
        with: { submodules: true }
      - uses: ilammy/msvc-dev-cmd@v1
      - run: cmake -B build -G "Visual Studio 17 2022"
      - run: cmake --build build --config Release
      - uses: actions/upload-artifact@v4
        with:
          name: lumos-windows
          path: build/Release/lumos.exe
```

## References

- Original project: Gamminator (Delphi/Object Pascal)
- Source location: `/Users/ian/Development/gamminator`
- License: GPL v2
