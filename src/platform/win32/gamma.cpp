// Lumos - Gamma control module
// Copyright (C) 2026 Ian Dirk Armstrong
// License: GPL v2

#include "gamma.h"
#include <cmath>

namespace lumos::platform {

Gamma::~Gamma()
{
    // Best-effort restore on destruction
    if (has_original_) {
        restoreOriginal();
    }
}

bool Gamma::captureOriginal()
{
    HDC hdc = GetDC(nullptr);
    if (!hdc) return false;

    BOOL result = GetDeviceGammaRamp(hdc, &original_ramp_);
    ReleaseDC(nullptr, hdc);

    if (result) {
        has_original_ = true;
    }
    return result != FALSE;
}

bool Gamma::restoreOriginal()
{
    if (!has_original_) return false;

    HDC hdc = GetDC(nullptr);
    if (!hdc) return false;

    BOOL result = SetDeviceGammaRamp(hdc, &original_ramp_);
    ReleaseDC(nullptr, hdc);

    return result != FALSE;
}

bool Gamma::apply(double value)
{
    // Clamp to valid range
    if (value < 0.1) value = 0.1;
    if (value > 9.0) value = 9.0;

    // Build gamma ramp
    GammaRamp ramp{};
    for (int i = 0; i < 256; ++i) {
        double normalized = i / 255.0;
        double corrected = std::pow(normalized, 1.0 / value);
        WORD val = static_cast<WORD>(corrected * 65535.0);

        ramp.red[i] = val;
        ramp.green[i] = val;
        ramp.blue[i] = val;
    }

    HDC hdc = GetDC(nullptr);
    if (!hdc) return false;

    BOOL result = SetDeviceGammaRamp(hdc, &ramp);
    ReleaseDC(nullptr, hdc);

    return result != FALSE;
}

double Gamma::read() const
{
    HDC hdc = GetDC(nullptr);
    if (!hdc) return 1.0;

    GammaRamp ramp{};
    BOOL result = GetDeviceGammaRamp(hdc, &ramp);
    ReleaseDC(nullptr, hdc);

    if (!result) return 1.0;

    // Sample middle value to estimate gamma
    // Using index 128 (middle of ramp)
    double normalized_in = 128.0 / 255.0;  // 0.502
    double normalized_out = ramp.red[128] / 65535.0;

    // Avoid log(0) or invalid values
    if (normalized_out <= 0.0 || normalized_out >= 1.0) {
        return 1.0;
    }

    // gamma = 1 / (log(out) / log(in))
    // out = in^(1/gamma)
    // log(out) = (1/gamma) * log(in)
    // gamma = log(in) / log(out)
    double gamma = std::log(normalized_in) / std::log(normalized_out);

    // Clamp to reasonable range
    if (gamma < 0.1) gamma = 0.1;
    if (gamma > 9.0) gamma = 9.0;

    return gamma;
}

} // namespace lumos::platform
