# Lumos

A lightweight Windows tray application for adjusting monitor gamma in real-time.

Lumos is a modern C++ reimplementation of [Gamminator](http://sourceforge.net/projects/gamminator), originally created by Wolfgang Freiler, with multi-monitor support by Lady Eklipse.

## Features

- **Real-time gamma adjustment** - Smooth slider control from 0.1 to 9.0
- **System tray integration** - Runs quietly in the background
- **Multi-monitor support** - Applies gamma to all connected displays
- **Global hotkeys** - Adjust gamma without opening the window
- **CLI interface** - Set gamma from the command line
- **Crash-safe** - Automatically restores original gamma on unexpected exit
- **Persistent settings** - Remembers your last gamma value
- **Minimize to tray** - Stays out of your way when not needed

## Installation

1. Download the latest release from the [Releases](https://github.com/IanDirkArmstrong/lumos/releases) page
2. Extract the ZIP file
3. Run `lumos.exe`

## Usage

### GUI Mode

- **Adjust gamma**: Use the slider in the main window
- **Reset to default**: Click "Reset to Default" or right-click the tray icon
- **Hide window**: Click the X button or minimize (app stays in tray)
- **Show window**: Double-click the tray icon or right-click → "Open Lumos"
- **Exit completely**: Right-click tray icon → "Exit"

### Global Hotkeys

| Hotkey | Action |
| ------ | ------ |
| Ctrl+Alt+Up | Increase gamma |
| Ctrl+Alt+Down | Decrease gamma |
| Ctrl+Alt+R | Reset to default |

### Command Line

```bash
lumos              # Open the GUI
lumos 1.2          # Set gamma to 1.2 and exit
lumos --help       # Show help
lumos --version    # Show version
```

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

## Attribution

Lumos is a reimplementation of **Gamminator**, a gamma adjustment utility for Windows:

- **Original author**: Wolfgang Freiler (2005)
- **Multi-monitor mod**: Lady Eklipse (v0.5.7)
- **Original project**: [SourceForge](http://sourceforge.net/projects/gamminator)

## License

This program is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation; either version 2 of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

See [LICENSE](LICENSE) for the full GPL v2 text.

## Copyright

- Original Gamminator: Copyright (C) 2005 Wolfgang Freiler
- Lumos reimplementation: Copyright (C) 2026 Ian Dirk Armstrong
