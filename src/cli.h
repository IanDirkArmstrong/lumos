// Lumos - Command line interface
// Copyright (C) 2026 Ian Dirk Armstrong
// License: GPL v2

#pragma once

#include <optional>
#include <string>

namespace lumos {

enum class CliAction {
    ShowGui,        // No args or invalid args: launch GUI
    SetGamma,       // Numeric arg: set gamma and exit
    ShowHelp,       // --help: show usage
    ShowVersion     // --version: show version
};

struct CliArgs {
    CliAction action = CliAction::ShowGui;
    double gamma_value = 1.0;
};

class Cli {
public:
    // Parse command line arguments
    static CliArgs parse(int argc, char* argv[]);
    static CliArgs parse(const wchar_t* cmdLine);

    // Print help to stdout (requires console)
    static void printHelp();

    // Print version to stdout (requires console)
    static void printVersion();

private:
    static std::optional<double> parseGammaValue(const std::string& str);
};

} // namespace lumos
