// Lumos - Configuration module
// Copyright (C) 2026 Ian Dirk Armstrong
// License: GPL v2

#pragma once

#include <string>
#include <filesystem>

namespace lumos {

class Config {
public:
    Config() = default;

    // Load config from file (creates default if missing)
    bool load();

    // Save config to file
    bool save();

    // Settings
    double last_gamma = 1.0;

private:
    std::filesystem::path getConfigDir();
    std::filesystem::path getConfigPath();
};

} // namespace lumos
