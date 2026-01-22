#pragma once

#include <cstdint>

namespace DicomViewer
{

enum class EPhotometricInterpretation
{
    Monochrome1,
    Monochrome2,
    Rgb,
    PaletteColor,
    Unknown
};

enum class ELoadResult
{
    Success,
    FileNotFound,
    InvalidFormat,
    UnsupportedTransferSyntax,
    DecompressionFailed,
    Unknown
};

enum class EPaletteType
{
    Grayscale,
    Inverted,
    Hot,
    Cool,
    Rainbow,
    Bone,
    Copper,
    Ocean
};

struct SWindowLevel
{
    double center = 0.0;
    double width = 1.0;
};

struct SImageDimensions
{
    uint32_t width = 0;
    uint32_t height = 0;
    uint16_t bitsAllocated = 0;
    uint16_t bitsStored = 0;
    uint16_t highBit = 0;
    uint16_t samplesPerPixel = 1;
    bool isSigned = false;
};

constexpr int kMinWindowWidth = 1;

} // namespace DicomViewer
