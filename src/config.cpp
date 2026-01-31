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
        // Create default config
        return save();
    }

    std::ifstream file(config_path);
    if (!file.is_open()) return false;

    std::string line;
    while (std::getline(file, line)) {
        // Simple INI parsing
        if (line.starts_with("LastValue=")) {
            try {
                last_gamma = std::stod(line.substr(10));
                if (last_gamma < 0.1) last_gamma = 0.1;
                if (last_gamma > 9.0) last_gamma = 9.0;
            } catch (...) {
                last_gamma = 1.0;
            }
        }
    }

    return true;
}

bool Config::save()
{
    auto config_dir = getConfigDir();
    if (config_dir.empty()) return false;

    // Create directory if needed
    std::filesystem::create_directories(config_dir);

    auto config_path = getConfigPath();
    std::ofstream file(config_path);
    if (!file.is_open()) return false;

    file << "[Gamma]\n";
    file << "LastValue=" << last_gamma << "\n";

    return true;
}

} // namespace lumos
