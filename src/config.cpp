// Lumos - Configuration module
// Copyright (C) 2026 Ian Dirk Armstrong
// License: GPL v2

#include "config.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <shlobj.h>
#include <fstream>
#include <string>
#include <algorithm>
#include <sstream>
#include <cctype>

namespace lumos {

// Static key mapping table
static const std::vector<HotkeyUtils::KeyInfo> s_bindable_keys = {
    // Arrow keys
    { VK_UP, "Up" },
    { VK_DOWN, "Down" },
    { VK_LEFT, "Left" },
    { VK_RIGHT, "Right" },
    // Function keys
    { VK_F1, "F1" },
    { VK_F2, "F2" },
    { VK_F3, "F3" },
    { VK_F4, "F4" },
    { VK_F5, "F5" },
    { VK_F6, "F6" },
    { VK_F7, "F7" },
    { VK_F8, "F8" },
    { VK_F9, "F9" },
    { VK_F10, "F10" },
    { VK_F11, "F11" },
    { VK_F12, "F12" },
    // Letters
    { 'A', "A" }, { 'B', "B" }, { 'C', "C" }, { 'D', "D" }, { 'E', "E" },
    { 'F', "F" }, { 'G', "G" }, { 'H', "H" }, { 'I', "I" }, { 'J', "J" },
    { 'K', "K" }, { 'L', "L" }, { 'M', "M" }, { 'N', "N" }, { 'O', "O" },
    { 'P', "P" }, { 'Q', "Q" }, { 'R', "R" }, { 'S', "S" }, { 'T', "T" },
    { 'U', "U" }, { 'V', "V" }, { 'W', "W" }, { 'X', "X" }, { 'Y', "Y" },
    { 'Z', "Z" },
    // Numbers
    { '0', "0" }, { '1', "1" }, { '2', "2" }, { '3', "3" }, { '4', "4" },
    { '5', "5" }, { '6', "6" }, { '7', "7" }, { '8', "8" }, { '9', "9" },
    // Special keys
    { VK_HOME, "Home" },
    { VK_END, "End" },
    { VK_PRIOR, "PageUp" },
    { VK_NEXT, "PageDown" },
    { VK_INSERT, "Insert" },
    { VK_DELETE, "Delete" },
};

const std::vector<HotkeyUtils::KeyInfo>& HotkeyUtils::getBindableKeys()
{
    return s_bindable_keys;
}

std::string HotkeyUtils::keyToString(UINT vk)
{
    for (const auto& key : s_bindable_keys) {
        if (key.vk == vk) {
            return key.name;
        }
    }
    return "Unknown";
}

UINT HotkeyUtils::stringToKey(const std::string& str)
{
    // Case-insensitive comparison
    std::string upper = str;
    for (auto& c : upper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

    for (const auto& key : s_bindable_keys) {
        std::string keyUpper = key.name;
        for (auto& c : keyUpper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        if (keyUpper == upper) {
            return key.vk;
        }
    }
    return 0;
}

std::string HotkeyUtils::bindingToString(const HotkeyBinding& binding)
{
    std::string result;

    if (binding.modifiers & MOD_CONTROL) {
        result += "Ctrl+";
    }
    if (binding.modifiers & MOD_ALT) {
        result += "Alt+";
    }
    if (binding.modifiers & MOD_SHIFT) {
        result += "Shift+";
    }
    if (binding.modifiers & MOD_WIN) {
        result += "Win+";
    }

    result += keyToString(binding.key);
    return result;
}

bool HotkeyUtils::stringToBinding(const std::string& str, HotkeyBinding& out)
{
    HotkeyBinding result;
    result.modifiers = 0;
    result.key = 0;

    std::istringstream stream(str);
    std::string token;

    while (std::getline(stream, token, '+')) {
        // Trim whitespace
        while (!token.empty() && std::isspace(static_cast<unsigned char>(token.front()))) {
            token.erase(0, 1);
        }
        while (!token.empty() && std::isspace(static_cast<unsigned char>(token.back()))) {
            token.pop_back();
        }

        if (token.empty()) continue;

        // Case-insensitive comparison
        std::string upper = token;
        for (auto& c : upper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

        if (upper == "CTRL" || upper == "CONTROL") {
            result.modifiers |= MOD_CONTROL;
        } else if (upper == "ALT") {
            result.modifiers |= MOD_ALT;
        } else if (upper == "SHIFT") {
            result.modifiers |= MOD_SHIFT;
        } else if (upper == "WIN" || upper == "WINDOWS") {
            result.modifiers |= MOD_WIN;
        } else {
            // Must be the key
            result.key = stringToKey(token);
        }
    }

    if (result.key == 0) {
        return false; // No valid key found
    }

    out = result;
    return true;
}

std::filesystem::path Config::getConfigDir()
{
    wchar_t* path = nullptr;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_RoamingAppData, 0, nullptr, &path))) {
        std::filesystem::path result = path;
        CoTaskMemFree(path);
        return result / L"Lumos";
    }
    return std::filesystem::path{};
}

std::filesystem::path Config::getConfigPath()
{
    return getConfigDir() / L"lumos.ini";
}

bool Config::load()
{
    auto config_path = getConfigPath();
    if (!std::filesystem::exists(config_path)) {
        return save();
    }

    std::ifstream file(config_path);
    if (!file.is_open()) return false;

    std::string line;
    while (std::getline(file, line)) {
        if (line.starts_with("LastValue=")) {
            try {
                last_gamma = std::stod(line.substr(10));
                if (last_gamma < 0.1) last_gamma = 0.1;
                if (last_gamma > 9.0) last_gamma = 9.0;
            } catch (...) {
                last_gamma = 1.0;
            }
        }
        else if (line.starts_with("TransferFunction=")) {
            transfer_function = line.substr(17);
        }
        else if (line.starts_with("Custom=")) {
            // Parse custom curve points: "x1:y1,x2:y2,x3:y3,..."
            custom_curve_points.clear();
            std::string curve_data = line.substr(7);

            size_t pos = 0;
            while (pos < curve_data.size()) {
                // Find next comma (or end of string)
                size_t comma_pos = curve_data.find(',', pos);
                if (comma_pos == std::string::npos) {
                    comma_pos = curve_data.size();
                }

                // Extract point pair "x:y"
                std::string pair = curve_data.substr(pos, comma_pos - pos);
                size_t colon_pos = pair.find(':');

                if (colon_pos != std::string::npos) {
                    try {
                        double x = std::stod(pair.substr(0, colon_pos));
                        double y = std::stod(pair.substr(colon_pos + 1));

                        // Validate range
                        if (x >= 0.0 && x <= 1.0 && y >= 0.0 && y <= 1.0) {
                            custom_curve_points.push_back({x, y});
                        }
                    } catch (...) {
                        // Skip invalid points
                    }
                }

                pos = comma_pos + 1;
            }

            // Ensure at least 2 points, otherwise reset to default linear
            if (custom_curve_points.size() < 2) {
                custom_curve_points = {{0.0, 0.0}, {1.0, 1.0}};
            }

            // Sort by x-coordinate
            std::sort(custom_curve_points.begin(), custom_curve_points.end());
        }
        // Hotkey bindings
        else if (line.starts_with("Increase=")) {
            HotkeyUtils::stringToBinding(line.substr(9), hotkey_increase);
        }
        else if (line.starts_with("Decrease=")) {
            HotkeyUtils::stringToBinding(line.substr(9), hotkey_decrease);
        }
        else if (line.starts_with("Reset=")) {
            HotkeyUtils::stringToBinding(line.substr(6), hotkey_reset);
        }
        else if (line.starts_with("Toggle=")) {
            HotkeyUtils::stringToBinding(line.substr(7), hotkey_toggle);
        }
    }

    return true;
}

bool Config::save()
{
    auto config_dir = getConfigDir();
    if (config_dir.empty()) return false;

    std::filesystem::create_directories(config_dir);

    auto config_path = getConfigPath();
    std::ofstream file(config_path);
    if (!file.is_open()) return false;

    file << "[Gamma]\n";
    file << "LastValue=" << last_gamma << "\n";
    file << "TransferFunction=" << transfer_function << "\n";
    file << "\n";
    file << "[Curves]\n";

    // Save custom curve points
    if (!custom_curve_points.empty()) {
        file << "Custom=";
        for (size_t i = 0; i < custom_curve_points.size(); ++i) {
            if (i > 0) file << ",";
            file << custom_curve_points[i].x << ":" << custom_curve_points[i].y;
        }
        file << "\n";
    }

    file << "\n";
    file << "[Hotkeys]\n";
    file << "Increase=" << HotkeyUtils::bindingToString(hotkey_increase) << "\n";
    file << "Decrease=" << HotkeyUtils::bindingToString(hotkey_decrease) << "\n";
    file << "Reset=" << HotkeyUtils::bindingToString(hotkey_reset) << "\n";
    file << "Toggle=" << HotkeyUtils::bindingToString(hotkey_toggle) << "\n";

    return true;
}

} // namespace lumos
