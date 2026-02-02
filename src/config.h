// Lumos - Configuration module
// Copyright (C) 2026 Ian Dirk Armstrong
// License: GPL v2

#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include "platform/win32/gamma.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

namespace lumos {

struct HotkeyBinding {
    UINT modifiers = MOD_CONTROL | MOD_ALT;
    UINT key = 0;
};

class Config {
public:
    Config() = default;

    // Load config from file (creates default if missing)
    bool load();

    // Save config to file
    bool save();

    // Settings
    double last_gamma = 1.0;
    std::string transfer_function = "Power";
    std::vector<platform::CurvePoint> custom_curve_points;

    // Hotkey bindings
    HotkeyBinding hotkey_increase = { MOD_CONTROL | MOD_ALT, VK_UP };
    HotkeyBinding hotkey_decrease = { MOD_CONTROL | MOD_ALT, VK_DOWN };
    HotkeyBinding hotkey_reset = { MOD_CONTROL | MOD_ALT, 'R' };

private:
    std::filesystem::path getConfigDir();
    std::filesystem::path getConfigPath();
};

} // namespace lumos
