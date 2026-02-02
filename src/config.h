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

    bool operator==(const HotkeyBinding& other) const {
        return modifiers == other.modifiers && key == other.key;
    }
    bool operator!=(const HotkeyBinding& other) const {
        return !(*this == other);
    }
};

// Utility functions for hotkey string conversion
namespace HotkeyUtils {
    // Convert HotkeyBinding to display string (e.g., "Ctrl+Alt+Up")
    std::string bindingToString(const HotkeyBinding& binding);

    // Parse string to HotkeyBinding (e.g., "Ctrl+Alt+Up")
    bool stringToBinding(const std::string& str, HotkeyBinding& out);

    // Get display name for a virtual key code (e.g., VK_UP -> "Up")
    std::string keyToString(UINT vk);

    // Parse key name to VK code (e.g., "Up" -> VK_UP)
    UINT stringToKey(const std::string& str);

    // Key info for UI dropdowns
    struct KeyInfo {
        UINT vk;
        const char* name;
    };
    const std::vector<KeyInfo>& getBindableKeys();
}

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
    HotkeyBinding hotkey_toggle = { MOD_CONTROL | MOD_ALT, 'G' };

    // Window behavior
    bool minimize_to_tray_on_close = true;

private:
    std::filesystem::path getConfigDir();
    std::filesystem::path getConfigPath();
};

} // namespace lumos
