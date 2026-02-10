// Lumos - Native Windows file dialogs
// Copyright (C) 2026 Ian Dirk Armstrong
// License: GPL v2

#pragma once

#include <string>

namespace lumos::platform {

// Open a file dialog to load a curve file
// Returns empty string if cancelled
// initialDir: optional starting directory (empty = system default)
std::string openCurveFileDialog(void* hwnd, const std::string& initialDir = "");

// Open a file dialog to save a curve file
// Returns empty string if cancelled
// initialDir: optional starting directory (empty = system default)
std::string saveCurveFileDialog(void* hwnd, const std::string& initialDir = "");

// Extract the directory portion from a file path
std::string getDirectoryFromPath(const std::string& filePath);

} // namespace lumos::platform
