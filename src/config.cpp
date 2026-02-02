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

namespace lumos {

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
        // TODO: Parse hotkey bindings in future version
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
    file << "; Hotkey customization coming in v1.0\n";
    file << "; Defaults: Ctrl+Alt+Up (increase), Ctrl+Alt+Down (decrease), Ctrl+Alt+R (reset)\n";

    return true;
}

} // namespace lumos
