// Lumos - Native Windows file dialogs
// Copyright (C) 2026 Ian Dirk Armstrong
// License: GPL v2

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include "file_dialog.h"
#include <windows.h>
#include <commdlg.h>
#include <shlobj.h>

namespace lumos::platform {

static std::wstring utf8ToWide(const std::string& str)
{
    if (str.empty()) return L"";
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    std::wstring result(size - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, result.data(), size);
    return result;
}

static std::string wideToUtf8(const wchar_t* wstr)
{
    if (!wstr || !wstr[0]) return "";
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, nullptr, 0, nullptr, nullptr);
    std::string result(size - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr, -1, result.data(), size, nullptr, nullptr);
    return result;
}

std::string openCurveFileDialog(void* hwnd, const std::string& initialDir)
{
    wchar_t filename[MAX_PATH] = L"";
    std::wstring wideDir = utf8ToWide(initialDir);

    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = static_cast<HWND>(hwnd);
    ofn.lpstrFilter = L"Curve Files (*.curve)\0*.curve\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = L"Load Curve";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    ofn.lpstrDefExt = L"curve";
    if (!wideDir.empty()) {
        ofn.lpstrInitialDir = wideDir.c_str();
    }

    if (GetOpenFileNameW(&ofn)) {
        return wideToUtf8(filename);
    }

    return "";
}

std::string saveCurveFileDialog(void* hwnd, const std::string& initialDir)
{
    wchar_t filename[MAX_PATH] = L"";
    std::wstring wideDir = utf8ToWide(initialDir);

    OPENFILENAMEW ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = static_cast<HWND>(hwnd);
    ofn.lpstrFilter = L"Curve Files (*.curve)\0*.curve\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = L"Save Curve";
    ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    ofn.lpstrDefExt = L"curve";
    if (!wideDir.empty()) {
        ofn.lpstrInitialDir = wideDir.c_str();
    }

    if (GetSaveFileNameW(&ofn)) {
        return wideToUtf8(filename);
    }

    return "";
}

std::string getDirectoryFromPath(const std::string& filePath)
{
    // Find last path separator
    size_t pos = filePath.find_last_of("\\/");
    if (pos != std::string::npos) {
        return filePath.substr(0, pos);
    }
    return "";
}

} // namespace lumos::platform
