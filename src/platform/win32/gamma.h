// limos - Gamma control module
// Copyright (C) 2026 Ian Dirk Armstrong
// License: GPL v2

#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <windows.h>
#include <array>
#include <vector>
#include <string>

namespace lumos::platform {

// Custom curve control point
struct CurvePoint {
    double x;  // Input (0.0 - 1.0)
    double y;  // Output (0.0 - 1.0)

    // For sorting by x-coordinate
    bool operator<(const CurvePoint& other) const { return x < other.x; }
};

// Tone curve presets for GPU output remapping
// NOTE: These are NOT calibrated color-space transforms - they are simple
// 1D LUTs applied globally via SetDeviceGammaRamp, affecting the entire desktop.
enum class ToneCurve {
    Linear,       // Identity curve (no adjustment)
    Power,        // Simple power-law gamma curve
    ShadowLift,   // Lifts shadow detail (sRGB-like shape, NOT actual sRGB)
    SoftContrast, // Gentle S-curve contrast (Rec.709-like shape)
    Cinema,       // Aggressive gamma 2.6 curve
    Custom,       // User-defined curve with control points
};

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

    // Apply tone curve to all monitors
    bool applyAll(double value);
    bool applyAll(ToneCurve curve, double strength,
                  const std::vector<CurvePoint>* custom_curve = nullptr);

    // Apply tone curve to specific monitor
    bool apply(size_t monitor_index, double value);
    bool apply(size_t monitor_index, ToneCurve curve, double strength,
               const std::vector<CurvePoint>* custom_curve = nullptr);

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
    static GammaRamp buildRamp(ToneCurve curve, double strength,
                               const std::vector<CurvePoint>* custom_curve = nullptr);
};

} // namespace lumos::platform
