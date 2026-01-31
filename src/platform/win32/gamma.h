// Lumos - Gamma control module
// Copyright (C) 2026 Ian Dirk Armstrong
// License: GPL v2

#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <array>

namespace lumos::platform {

struct GammaRamp {
    std::array<WORD, 256> red;
    std::array<WORD, 256> green;
    std::array<WORD, 256> blue;
};

class Gamma {
public:
    Gamma() = default;
    ~Gamma();

    // Capture current gamma ramp (call on startup)
    bool captureOriginal();

    // Restore saved gamma ramp
    bool restoreOriginal();

    // Apply gamma value (0.1 - 9.0, 1.0 = normal)
    bool apply(double value);

    // Read current gamma from primary display
    double read() const;

    // Check if we have a captured original
    bool hasOriginal() const { return has_original_; }

private:
    GammaRamp original_ramp_{};
    bool has_original_ = false;
};

} // namespace lumos::platform
