// Lumos - Command line interface
// Copyright (C) 2026 Ian Dirk Armstrong
// License: GPL v2

#include "cli.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <shellapi.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

namespace lumos {

std::optional<double> Cli::parseGammaValue(const std::string& str)
{
    try {
        size_t pos = 0;
        double value = std::stod(str, &pos);

        // Ensure entire string was consumed
        if (pos != str.length()) {
            return std::nullopt;
        }

        // Validate range
        if (value < 0.1 || value > 9.0) {
            return std::nullopt;
        }

        return value;
    } catch (...) {
        return std::nullopt;
    }
}

CliArgs Cli::parse(int argc, char* argv[])
{
    CliArgs args;

    if (argc < 2) {
        args.action = CliAction::ShowGui;
        return args;
    }

    std::string arg1 = argv[1];

    if (arg1 == "--help" || arg1 == "-h") {
        args.action = CliAction::ShowHelp;
        return args;
    }

    if (arg1 == "--version" || arg1 == "-v") {
        args.action = CliAction::ShowVersion;
        return args;
    }

    // Try to parse as gamma value
    auto gamma = parseGammaValue(arg1);
    if (gamma.has_value()) {
        args.action = CliAction::SetGamma;
        args.gamma_value = gamma.value();
        return args;
    }

    // Unknown argument, show GUI
    args.action = CliAction::ShowGui;
    return args;
}

CliArgs Cli::parse(const wchar_t* cmdLine)
{
    int argc = 0;
    wchar_t** argv = CommandLineToArgvW(cmdLine, &argc);

    if (!argv || argc < 2) {
        if (argv) LocalFree(argv);
        return CliArgs{};
    }

    // Convert first argument to narrow string
    std::string arg1;
    int len = WideCharToMultiByte(CP_UTF8, 0, argv[1], -1, nullptr, 0, nullptr, nullptr);
    if (len > 0) {
        arg1.resize(len - 1);
        WideCharToMultiByte(CP_UTF8, 0, argv[1], -1, arg1.data(), len, nullptr, nullptr);
    }

    LocalFree(argv);

    // Parse using string version
    const char* args[] = { "lumos", arg1.c_str() };
    return parse(2, const_cast<char**>(args));
}

void Cli::printHelp()
{
    // Attach to parent console if available
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        FILE* fp = nullptr;
        freopen_s(&fp, "CONOUT$", "w", stdout);
    }

    std::printf("\nLumos - Monitor Gamma Adjustment Utility\n\n");
    std::printf("Usage:\n");
    std::printf("  lumos              Open the GUI\n");
    std::printf("  lumos <value>      Set gamma (0.1-9.0) and exit\n");
    std::printf("  lumos --help       Show this help message\n");
    std::printf("  lumos --version    Show version information\n");
    std::printf("\n");
    std::printf("Hotkeys (when running):\n");
    std::printf("  Ctrl+Alt+Up        Increase gamma by 0.1\n");
    std::printf("  Ctrl+Alt+Down      Decrease gamma by 0.1\n");
    std::printf("  Ctrl+Alt+R         Reset to default (1.0)\n");
    std::printf("\n");
    std::printf("Examples:\n");
    std::printf("  lumos 1.2          Set gamma to 1.2\n");
    std::printf("  lumos 0.8          Set gamma to 0.8\n");
    std::printf("\n");
}

void Cli::printVersion()
{
    if (AttachConsole(ATTACH_PARENT_PROCESS)) {
        FILE* fp = nullptr;
        freopen_s(&fp, "CONOUT$", "w", stdout);
    }

    std::printf("\nLumos v0.2.0\n");
    std::printf("Copyright (C) 2026 Ian Dirk Armstrong\n");
    std::printf("License: GPL v2\n\n");
}

} // namespace lumos
