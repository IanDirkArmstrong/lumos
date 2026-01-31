// Lumos - Monitor gamma adjustment utility
// Copyright (C) 2026 Ian Dirk Armstrong
// License: GPL v2

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>

int WINAPI WinMain(
    _In_ HINSTANCE hInstance,
    _In_opt_ HINSTANCE hPrevInstance,
    _In_ LPSTR lpCmdLine,
    _In_ int nCmdShow)
{
    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;

    MessageBoxW(nullptr, L"Lumos v0.1 - Scaffolding works!", L"Lumos", MB_OK);
    return 0;
}
