// Lumos - Gamma control module
// Copyright (C) 2026 Ian Dirk Armstrong
// License: GPL v2

#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <array>
#include <vector>
#include <string>

namespace lumos::platform {

struct GammaRamp {
    std::array<WORD, 256> red;
    std::array<WORD, 256> green;
    std::array<WORD, 256> blue;
};

struct MonitorInfo {
    HMONITOR handle;
    std::wstring device_name;
    std::wstring friendly_name;
    bool is_primary;
    GammaRamp original_ramp;
    bool has_original;
};

class Gamma {
public:
    Gamma() = default;
    ~Gamma();

    // Enumerate monitors and capture original ramps
    bool initialize();

    // Restore all original gamma ramps
    bool restoreAll();

    // Apply gamma value to all monitors
    bool applyAll(double value);

    // Apply gamma value to specific monitor
    bool apply(size_t monitor_index, double value);

    // Restore specific monitor
    bool restore(size_t monitor_index);

    // Get monitor count
    size_t getMonitorCount() const { return monitors_.size(); }

    // Get monitor info
    const MonitorInfo* getMonitor(size_t index) const;

    // Get primary monitor index
    size_t getPrimaryIndex() const;

    // Legacy single-monitor interface (operates on primary)
    bool captureOriginal();
    bool restoreOriginal();
    bool apply(double value);
    double read() const;
    bool hasOriginal() const;

private:
    std::vector<MonitorInfo> monitors_;

    static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor,
                                          LPRECT lprcMonitor, LPARAM dwData);

    bool captureRamp(MonitorInfo& monitor);
    bool applyRamp(const MonitorInfo& monitor, const GammaRamp& ramp);
    static GammaRamp buildRamp(double gamma);
};

} // namespace lumos::platform
