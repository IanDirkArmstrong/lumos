// Lumos - Gamma control module
// Copyright (C) 2026 Ian Dirk Armstrong
// License: GPL v2

#include "gamma.h"
#include <cmath>

namespace lumos::platform {

Gamma::~Gamma()
{
    restoreAll();
}

BOOL CALLBACK Gamma::MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor,
                                      LPRECT lprcMonitor, LPARAM dwData)
{
    (void)hdcMonitor;
    (void)lprcMonitor;

    auto* monitors = reinterpret_cast<std::vector<MonitorInfo>*>(dwData);

    MONITORINFOEXW mi = {};
    mi.cbSize = sizeof(mi);
    if (!GetMonitorInfoW(hMonitor, &mi)) {
        return TRUE; // Continue enumeration
    }

    MonitorInfo info = {};
    info.handle = hMonitor;
    info.device_name = mi.szDevice;
    info.friendly_name = mi.szDevice; // TODO: Get friendly name from registry
    info.is_primary = (mi.dwFlags & MONITORINFOF_PRIMARY) != 0;
    info.has_original = false;

    monitors->push_back(info);
    return TRUE;
}

bool Gamma::initialize()
{
    monitors_.clear();

    // Enumerate all monitors
    EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc,
                        reinterpret_cast<LPARAM>(&monitors_));

    if (monitors_.empty()) {
        return false;
    }

    // Capture original ramps for all monitors
    bool success = true;
    for (auto& monitor : monitors_) {
        if (!captureRamp(monitor)) {
            success = false;
        }
    }

    return success;
}

bool Gamma::captureRamp(MonitorInfo& monitor)
{
    HDC hdc = CreateDCW(L"DISPLAY", monitor.device_name.c_str(), nullptr, nullptr);
    if (!hdc) return false;

    BOOL result = GetDeviceGammaRamp(hdc, &monitor.original_ramp);
    DeleteDC(hdc);

    if (result) {
        monitor.has_original = true;
    }
    return result != FALSE;
}

bool Gamma::applyRamp(const MonitorInfo& monitor, const GammaRamp& ramp)
{
    HDC hdc = CreateDCW(L"DISPLAY", monitor.device_name.c_str(), nullptr, nullptr);
    if (!hdc) return false;

    // Need non-const for SetDeviceGammaRamp
    GammaRamp ramp_copy = ramp;
    BOOL result = SetDeviceGammaRamp(hdc, &ramp_copy);
    DeleteDC(hdc);

    return result != FALSE;
}

GammaRamp Gamma::buildRamp(double gamma)
{
    if (gamma < 0.1) gamma = 0.1;
    if (gamma > 9.0) gamma = 9.0;

    GammaRamp ramp{};
    for (int i = 0; i < 256; ++i) {
        double normalized = i / 255.0;
        double corrected = std::pow(normalized, 1.0 / gamma);
        WORD val = static_cast<WORD>(corrected * 65535.0);

        ramp.red[i] = val;
        ramp.green[i] = val;
        ramp.blue[i] = val;
    }
    return ramp;
}

bool Gamma::restoreAll()
{
    bool success = true;
    for (const auto& monitor : monitors_) {
        if (monitor.has_original) {
            if (!applyRamp(monitor, monitor.original_ramp)) {
                success = false;
            }
        }
    }
    return success;
}

bool Gamma::applyAll(double value)
{
    GammaRamp ramp = buildRamp(value);
    bool success = true;
    for (const auto& monitor : monitors_) {
        if (!applyRamp(monitor, ramp)) {
            success = false;
        }
    }
    return success;
}

bool Gamma::apply(size_t monitor_index, double value)
{
    if (monitor_index >= monitors_.size()) return false;

    GammaRamp ramp = buildRamp(value);
    return applyRamp(monitors_[monitor_index], ramp);
}

bool Gamma::restore(size_t monitor_index)
{
    if (monitor_index >= monitors_.size()) return false;

    const auto& monitor = monitors_[monitor_index];
    if (!monitor.has_original) return false;

    return applyRamp(monitor, monitor.original_ramp);
}

const MonitorInfo* Gamma::getMonitor(size_t index) const
{
    if (index >= monitors_.size()) return nullptr;
    return &monitors_[index];
}

size_t Gamma::getPrimaryIndex() const
{
    for (size_t i = 0; i < monitors_.size(); ++i) {
        if (monitors_[i].is_primary) return i;
    }
    return 0; // Fallback to first monitor
}

// Legacy single-monitor interface
bool Gamma::captureOriginal()
{
    return initialize();
}

bool Gamma::restoreOriginal()
{
    return restoreAll();
}

bool Gamma::apply(double value)
{
    return applyAll(value);
}

double Gamma::read() const
{
    if (monitors_.empty()) return 1.0;

    size_t primary = getPrimaryIndex();
    const auto& monitor = monitors_[primary];

    HDC hdc = CreateDCW(L"DISPLAY", monitor.device_name.c_str(), nullptr, nullptr);
    if (!hdc) return 1.0;

    GammaRamp ramp{};
    BOOL result = GetDeviceGammaRamp(hdc, &ramp);
    DeleteDC(hdc);

    if (!result) return 1.0;

    // Sample middle value to estimate gamma
    double normalized_in = 128.0 / 255.0;
    double normalized_out = ramp.red[128] / 65535.0;

    if (normalized_out <= 0.0 || normalized_out >= 1.0) {
        return 1.0;
    }

    double gamma = std::log(normalized_in) / std::log(normalized_out);

    if (gamma < 0.1) gamma = 0.1;
    if (gamma > 9.0) gamma = 9.0;

    return gamma;
}

bool Gamma::hasOriginal() const
{
    for (const auto& monitor : monitors_) {
        if (monitor.has_original) return true;
    }
    return false;
}

} // namespace lumos::platform
