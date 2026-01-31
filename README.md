# Lumos

A lightweight Windows tray application for adjusting monitor gamma in real-time.

## Features

- **Real-time gamma adjustment** - Smooth slider control from 0.1 to 9.0
- **System tray integration** - Runs quietly in the background
- **Crash-safe** - Automatically restores original gamma on unexpected exit
- **Persistent settings** - Remembers your last gamma value
- **Minimize to tray** - Stays out of your way when not needed

## Installation

1. Download the latest release from the [Releases](https://github.com/IanDirkArmstrong/lumos/releases) page
2. Extract the ZIP file
3. Run `lumos.exe`

## Usage

- **Adjust gamma**: Use the slider in the main window
- **Reset to default**: Click "Reset to Default" or right-click the tray icon
- **Hide window**: Click the X button or minimize (app stays in tray)
- **Show window**: Double-click the tray icon or right-click → "Open Lumos"
- **Exit completely**: Right-click tray icon → "Exit"

## System Requirements

- Windows 10 or later
- DirectX 11 compatible graphics card

## Building from Source

### Prerequisites

- CMake 3.20 or later
- Visual Studio 2022 with C++ development tools
- Git (for submodules)

### Build Steps

```bash
# Clone the repository
git clone https://github.com/IanDirkArmstrong/lumos.git
cd lumos

# Initialize submodules
git submodule update --init --recursive

# Configure and build
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release

# Executable will be at: build/Release/lumos.exe
```

## License

GPL v2 - See [LICENSE](LICENSE) for details

## Copyright

Copyright (C) 2026 Ian Dirk Armstrong
