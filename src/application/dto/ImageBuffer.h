/**
 * @file ImageBuffer.h
 * @brief Raw image buffer DTO for rendering/export
 * @date 2026
 */

#pragma once

#include <cstdint>
#include <vector>

enum class EPixelFormat
{
    Grayscale8,
    RGB24,
    RGBA32
};

struct SImageBuffer
{
    int width = 0;
    int height = 0;
    EPixelFormat format = EPixelFormat::RGB24;
    int bytesPerLine = 0;
    std::vector<uint8_t> data;
};
