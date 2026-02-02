// Lumos - Screen histogram capture
// Copyright (C) 2026 Ian Dirk Armstrong
// License: GPL v2

#include "screen_histogram.h"
#include <algorithm>
#include <array>

namespace lumos::platform {

ScreenHistogramCapture::~ScreenHistogramCapture()
{
    stop();
}

void ScreenHistogramCapture::start()
{
    if (running_) return;

    running_ = true;
    capture_thread_ = std::thread(&ScreenHistogramCapture::captureThread, this);
}

void ScreenHistogramCapture::stop()
{
    running_ = false;
    if (capture_thread_.joinable()) {
        capture_thread_.join();
    }
}

ScreenHistogram ScreenHistogramCapture::getHistogram() const
{
    std::lock_guard<std::mutex> lock(histogram_mutex_);
    return histogram_;
}

void ScreenHistogramCapture::captureThread()
{
    while (running_) {
        if (enabled_) {
            captureScreen();
        }
        Sleep(capture_interval_ms_);
    }
}

void ScreenHistogramCapture::captureScreen()
{
    // Get primary monitor dimensions
    int screen_width = GetSystemMetrics(SM_CXSCREEN);
    int screen_height = GetSystemMetrics(SM_CYSCREEN);

    // Sample at lower resolution for performance (every 4th pixel)
    const int sample_step = 4;

    // Create compatible DC and bitmap
    HDC hScreenDC = GetDC(nullptr);
    HDC hMemDC = CreateCompatibleDC(hScreenDC);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = screen_width / sample_step;
    bmi.bmiHeader.biHeight = -screen_height / sample_step;  // Negative for top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pBits = nullptr;
    HBITMAP hBitmap = CreateDIBSection(hMemDC, &bmi, DIB_RGB_COLORS, &pBits, nullptr, 0);

    if (!hBitmap || !pBits) {
        DeleteDC(hMemDC);
        ReleaseDC(nullptr, hScreenDC);
        return;
    }

    HGDIOBJ hOldBitmap = SelectObject(hMemDC, hBitmap);

    // Capture screen with stretch (downsampling)
    SetStretchBltMode(hMemDC, HALFTONE);
    StretchBlt(
        hMemDC, 0, 0, screen_width / sample_step, screen_height / sample_step,
        hScreenDC, 0, 0, screen_width, screen_height,
        SRCCOPY);

    // Calculate histogram
    std::array<int, 256> raw_histogram{};
    int total_pixels = 0;

    int bitmap_width = screen_width / sample_step;
    int bitmap_height = screen_height / sample_step;
    int row_stride = ((bitmap_width * 3 + 3) / 4) * 4;  // 24-bit rows are DWORD-aligned

    BYTE* pixels = static_cast<BYTE*>(pBits);

    for (int y = 0; y < bitmap_height; ++y) {
        BYTE* row = pixels + y * row_stride;
        for (int x = 0; x < bitmap_width; ++x) {
            // BGR order in 24-bit DIB
            BYTE b = row[x * 3 + 0];
            BYTE g = row[x * 3 + 1];
            BYTE r = row[x * 3 + 2];

            // Calculate relative luminance (Rec. 709 coefficients)
            // Y = 0.2126 R + 0.7152 G + 0.0722 B
            int luminance = static_cast<int>(0.2126f * r + 0.7152f * g + 0.0722f * b);
            luminance = std::clamp(luminance, 0, 255);

            raw_histogram[luminance]++;
            total_pixels++;
        }
    }

    // Cleanup GDI
    SelectObject(hMemDC, hOldBitmap);
    DeleteObject(hBitmap);
    DeleteDC(hMemDC);
    ReleaseDC(nullptr, hScreenDC);

    // Normalize histogram
    ScreenHistogram result;
    result.max_value = 0.0f;

    if (total_pixels > 0) {
        for (int i = 0; i < 256; ++i) {
            result.luminance[i] = static_cast<float>(raw_histogram[i]) / total_pixels;
            if (result.luminance[i] > result.max_value) {
                result.max_value = result.luminance[i];
            }
        }
    }

    // Normalize to 0-1 range based on max value (for visualization)
    if (result.max_value > 0.0f) {
        for (int i = 0; i < 256; ++i) {
            result.luminance[i] /= result.max_value;
        }
        result.valid = true;
    }

    // Thread-safe update
    {
        std::lock_guard<std::mutex> lock(histogram_mutex_);
        histogram_ = result;
    }
}

} // namespace lumos::platform
