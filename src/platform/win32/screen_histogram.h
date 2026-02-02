// Lumos - Screen histogram capture
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
#include <atomic>
#include <thread>
#include <mutex>

namespace lumos::platform {

// Screen histogram data (256 bins for luminance values 0-255)
struct ScreenHistogram {
    std::array<float, 256> luminance{};  // Normalized histogram (0.0 - 1.0)
    float max_value = 0.0f;               // Maximum bin value before normalization
    bool valid = false;                   // Whether data is valid
};

// Captures screen content and computes luminance histogram
class ScreenHistogramCapture {
public:
    ScreenHistogramCapture() = default;
    ~ScreenHistogramCapture();

    // Start/stop background capture thread
    void start();
    void stop();

    // Get current histogram (thread-safe)
    ScreenHistogram getHistogram() const;

    // Set capture interval in milliseconds
    void setCaptureInterval(int ms) { capture_interval_ms_ = ms; }

    // Enable/disable capture
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }

private:
    void captureThread();
    void captureScreen();

    std::thread capture_thread_;
    mutable std::mutex histogram_mutex_;
    ScreenHistogram histogram_;

    std::atomic<bool> running_{false};
    std::atomic<bool> enabled_{true};
    int capture_interval_ms_ = 500;  // Update every 500ms by default
};

} // namespace lumos::platform
