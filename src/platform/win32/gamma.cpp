// Lumos - Gamma control module
// Copyright (C) 2026 Ian Dirk Armstrong
// License: GPL v2

#include "gamma.h"
#include <algorithm>
#include <cmath>

namespace lumos::platform {

// Tone curve shape functions
// NOTE: These are NOT calibrated transforms - they produce curve SHAPES
// that happen to resemble certain standards, but are applied as simple
// GPU output remaps with no device characterization or measurement.
namespace {

// Shadow-lifting curve (raises dark values, sRGB-like shape)
double ShadowLiftCurve(double linear) {
    if (linear <= 0.0031308)
        return 12.92 * linear;
    return 1.055 * std::pow(linear, 1.0 / 2.4) - 0.055;
}

// Soft contrast curve (gentle S-shape, Rec.709-like)
double SoftContrastCurve(double linear) {
    const double beta = 0.018;
    const double alpha = 1.099;
    const double gamma_exp = 0.45;

    if (linear < beta)
        return 4.5 * linear;
    return alpha * std::pow(linear, gamma_exp) - (alpha - 1.0);
}

// Simple power-law curve
double PowerCurve(double linear, double strength) {
    return std::pow(linear, 1.0 / strength);
}

} // anonymous namespace

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

bool Gamma::setRamp(const MonitorInfo& monitor, const GammaRamp& ramp)
{
    HDC hdc = CreateDCW(L"DISPLAY", monitor.device_name.c_str(), nullptr, nullptr);
    if (!hdc) return false;

    // Need non-const for SetDeviceGammaRamp
    GammaRamp ramp_copy = ramp;
    BOOL result = SetDeviceGammaRamp(hdc, &ramp_copy);
    DeleteDC(hdc);

    return result != FALSE;
}

bool Gamma::readRamp(const MonitorInfo& monitor, GammaRamp& out_ramp) const
{
    HDC hdc = CreateDCW(L"DISPLAY", monitor.device_name.c_str(), nullptr, nullptr);
    if (!hdc) return false;

    // GetDeviceGammaRamp expects WORD[3][256], which matches our GammaRamp layout
    BOOL result = GetDeviceGammaRamp(hdc, &out_ramp);
    DeleteDC(hdc);

    return result != FALSE;
}

bool Gamma::verifyRamp(const MonitorInfo& monitor, const GammaRamp& expected)
{
    GammaRamp actual{};
    if (!readRamp(monitor, actual)) {
        return false;
    }

    // Compare with tolerance. SetDeviceGammaRamp can round values slightly,
    // so we allow a small epsilon. If the ramp was silently rejected, the
    // actual values will be very different (often the previous ramp).
    constexpr int kMaxDiff = 512;  // ~0.8% of 65535, allows for rounding

    for (int i = 0; i < 256; ++i) {
        int diff_r = std::abs(static_cast<int>(expected.red[i]) - static_cast<int>(actual.red[i]));
        int diff_g = std::abs(static_cast<int>(expected.green[i]) - static_cast<int>(actual.green[i]));
        int diff_b = std::abs(static_cast<int>(expected.blue[i]) - static_cast<int>(actual.blue[i]));

        if (diff_r > kMaxDiff || diff_g > kMaxDiff || diff_b > kMaxDiff) {
            return false;
        }
    }

    return true;
}

GammaRamp Gamma::buildIdentityRamp()
{
    GammaRamp ramp{};
    for (int i = 0; i < 256; ++i) {
        WORD val = static_cast<WORD>(i * 257);  // Maps 0-255 to 0-65535
        ramp.red[i] = val;
        ramp.green[i] = val;
        ramp.blue[i] = val;
    }
    return ramp;
}

GammaRamp Gamma::blendRampTowardIdentity(const GammaRamp& ramp,
                                          const GammaRamp& identity,
                                          double scale)
{
    GammaRamp result{};
    for (int i = 0; i < 256; ++i) {
        // Blend: identity + scale * (ramp - identity)
        // When scale=1.0, result=ramp. When scale=0.0, result=identity.
        double r = identity.red[i] + scale * (static_cast<double>(ramp.red[i]) - identity.red[i]);
        double g = identity.green[i] + scale * (static_cast<double>(ramp.green[i]) - identity.green[i]);
        double b = identity.blue[i] + scale * (static_cast<double>(ramp.blue[i]) - identity.blue[i]);

        result.red[i] = static_cast<WORD>(std::clamp(r, 0.0, 65535.0));
        result.green[i] = static_cast<WORD>(std::clamp(g, 0.0, 65535.0));
        result.blue[i] = static_cast<WORD>(std::clamp(b, 0.0, 65535.0));
    }
    return result;
}

void Gamma::enforceMonotonicity(GammaRamp& ramp)
{
    // Ensure ramp values are strictly increasing (or at least non-decreasing).
    // Windows heuristics reject ramps with flat or decreasing segments.
    // We enforce a minimum step of 1 to keep it strictly increasing.
    constexpr WORD kMinStep = 1;

    auto enforce_channel = [](std::array<WORD, 256>& channel) {
        WORD prev = 0;
        for (int i = 0; i < 256; ++i) {
            if (channel[i] <= prev && i > 0) {
                channel[i] = (prev < 65535 - kMinStep) ? prev + kMinStep : 65535;
            }
            prev = channel[i];
        }
    };

    enforce_channel(ramp.red);
    enforce_channel(ramp.green);
    enforce_channel(ramp.blue);
}

bool Gamma::applyRampAdaptive(MonitorInfo& monitor, const GammaRamp& ideal)
{
    static const GammaRamp identity = buildIdentityRamp();

    // Start with the monitor's cached safe scale, slightly expanded to probe
    // whether we can use more range now
    double scale = (std::min)(1.0, monitor.safe_scale * 1.05);

    // Binary search bounds
    double low = 0.0;
    double high = scale;

    constexpr int kMaxAttempts = 6;
    GammaRamp last_good = identity;
    bool any_success = false;

    for (int attempt = 0; attempt < kMaxAttempts; ++attempt) {
        double try_scale = (attempt == 0) ? high : 0.5 * (low + high);

        GammaRamp blended = blendRampTowardIdentity(ideal, identity, try_scale);
        enforceMonotonicity(blended);

        if (!setRamp(monitor, blended)) {
            // Hard failure (API returned FALSE) - shrink range
            high = try_scale;
            continue;
        }

        // SetDeviceGammaRamp returned TRUE, but it might have silently rejected
        if (verifyRamp(monitor, blended)) {
            // Success! Remember this scale and try to expand
            low = try_scale;
            last_good = blended;
            any_success = true;
            monitor.safe_scale = try_scale;

            // If we're close enough to our target scale, stop
            if (high - low < 0.02) {
                break;
            }
        } else {
            // Silent rejection - shrink range
            high = try_scale;
        }
    }

    // Apply the best ramp we found
    if (any_success) {
        // If our last attempt wasn't the good one, re-apply
        if (high != low) {
            setRamp(monitor, last_good);
        }
        return true;
    }

    // Complete failure - fall back to identity
    setRamp(monitor, identity);
    monitor.safe_scale = 0.1;  // Very conservative for next attempt
    return false;
}

GammaRamp Gamma::buildRamp(ToneCurve curve, double strength,
                           const std::vector<CurvePoint>* custom_curve)
{
    if (strength < 0.1) strength = 0.1;
    if (strength > 9.0) strength = 9.0;

    GammaRamp ramp{};
    for (int i = 0; i < 256; ++i) {
        double linear = i / 255.0;
        double corrected;

        switch (curve) {
            case ToneCurve::Linear:
                corrected = linear;  // Identity - no change
                break;

            case ToneCurve::ShadowLift:
                corrected = ShadowLiftCurve(linear);
                break;

            case ToneCurve::SoftContrast:
                corrected = SoftContrastCurve(linear);
                break;

            case ToneCurve::Cinema:
                corrected = PowerCurve(linear, 2.6);
                break;

            case ToneCurve::Custom:
                if (custom_curve && custom_curve->size() >= 2) {
                    // Linear interpolation between control points
                    const auto& points = *custom_curve;

                    // Find surrounding points
                    if (linear <= points.front().x) {
                        // Before first point
                        corrected = points.front().y;
                    }
                    else if (linear >= points.back().x) {
                        // After last point
                        corrected = points.back().y;
                    }
                    else {
                        // Interpolate between two points
                        corrected = linear;  // Safe default
                        for (size_t j = 0; j < points.size() - 1; ++j) {
                            if (linear >= points[j].x && linear <= points[j + 1].x) {
                                double x1 = points[j].x;
                                double y1 = points[j].y;
                                double x2 = points[j + 1].x;
                                double y2 = points[j + 1].y;

                                // Linear interpolation
                                double t = (x2 > x1) ? (linear - x1) / (x2 - x1) : 0.0;
                                corrected = y1 + t * (y2 - y1);
                                break;
                            }
                        }
                    }
                }
                else {
                    // Fallback to linear (identity) if no valid curve
                    corrected = linear;
                }
                break;

            case ToneCurve::Power:
            default:
                corrected = PowerCurve(linear, strength);
                break;
        }

        // Clamp to valid range [0, 1]
        // Note: We use expanded bounds here. The adaptive application layer
        // (applyRampAdaptive) will scale back toward identity if Windows
        // rejects the ramp. This allows us to request aggressive curves and
        // get the maximum the driver will accept.
        if (corrected < 0.0) corrected = 0.0;
        if (corrected > 1.0) corrected = 1.0;

        // Optional soft bounds: keep values within a generous envelope around
        // identity to increase likelihood of acceptance.
        // Allow crushing blacks (min=0) and generous shadow lift (offset 0.2)
        double identity_val = linear;
        double min_allowed = 0.0;
        double max_allowed = (std::min)(1.0, identity_val * 3.0 + 0.2);
        if (corrected < min_allowed) corrected = min_allowed;
        if (corrected > max_allowed) corrected = max_allowed;

        WORD val = static_cast<WORD>(corrected * 65535.0);
        ramp.red[i] = val;
        ramp.green[i] = val;
        ramp.blue[i] = val;
    }

    // Enforce monotonicity to help pass Windows heuristics
    enforceMonotonicity(ramp);

    return ramp;
}

bool Gamma::restoreAll()
{
    bool success = true;
    for (const auto& monitor : monitors_) {
        if (monitor.has_original) {
            // Use direct setRamp for restore - original ramps should always be valid
            if (!setRamp(monitor, monitor.original_ramp)) {
                success = false;
            }
        }
    }
    return success;
}

bool Gamma::applyAll(double value)
{
    return applyAll(ToneCurve::Power, value, nullptr);
}

bool Gamma::applyAll(ToneCurve curve, double strength,
                     const std::vector<CurvePoint>* custom_curve)
{
    GammaRamp ramp = buildRamp(curve, strength, custom_curve);
    bool success = true;
    for (auto& monitor : monitors_) {
        if (!applyRampAdaptive(monitor, ramp)) {
            success = false;
        }
    }
    return success;
}

bool Gamma::apply(size_t monitor_index, double value)
{
    return apply(monitor_index, ToneCurve::Power, value, nullptr);
}

bool Gamma::apply(size_t monitor_index, ToneCurve curve, double strength,
                  const std::vector<CurvePoint>* custom_curve)
{
    if (monitor_index >= monitors_.size()) return false;

    GammaRamp ramp = buildRamp(curve, strength, custom_curve);
    return applyRampAdaptive(monitors_[monitor_index], ramp);
}

bool Gamma::restore(size_t monitor_index)
{
    if (monitor_index >= monitors_.size()) return false;

    const auto& monitor = monitors_[monitor_index];
    if (!monitor.has_original) return false;

    // Use direct setRamp for restore - original ramps should always be valid
    return setRamp(monitor, monitor.original_ramp);
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
