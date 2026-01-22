/**
 * @file CImageConverter.cpp
 * @brief Implementation of the CImageConverter class
 * @author DICOM Viewer Project
 * @date 2026
 *
 * Implements DICOM to QImage conversion with support for grayscale
 * and RGB images. Applies window/level transformation and color
 * palettes for contrast adjustment on grayscale images.
 */

#include "CImageConverter.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <vector>

/**
 * @brief Sets the color palette for grayscale images
 * @param type Palette type to use
 */
void CImageConverter::setPalette(DicomViewer::EPaletteType type)
{
    m_palette.setType(type);
}

/**
 * @brief Gets the current palette type
 * @return Current palette type
 */
DicomViewer::EPaletteType CImageConverter::paletteType() const
{
    return m_palette.type();
}

/**
 * @brief Gets the current palette
 * @return Reference to the current palette
 */
const CColorPalette &CImageConverter::palette() const
{
    return m_palette;
}

/**
 * @brief Converts DICOM image to QImage using current window/level
 * @param dicomImage Source DICOM image
 * @return QImage ready for display, or null QImage on failure
 */
QImage CImageConverter::toQImage(const CDicomImage &dicomImage) const
{
    return toQImage(dicomImage, dicomImage.windowLevel());
}

/**
 * @brief Converts DICOM image to QImage with specific window/level
 * @param dicomImage Source DICOM image
 * @param windowLevel Custom window/level settings
 * @return QImage ready for display, or null QImage on failure
 */
QImage CImageConverter::toQImage(const CDicomImage &dicomImage,
                                 const DicomViewer::SWindowLevel &windowLevel) const
{
    if (!dicomImage.isValid())
    {
        return QImage();
    }

    auto pi = dicomImage.photometricInterpretation();

    switch (pi)
    {
    case DicomViewer::EPhotometricInterpretation::Monochrome1:
    case DicomViewer::EPhotometricInterpretation::Monochrome2:
        return convertMonochrome(dicomImage, windowLevel);

    case DicomViewer::EPhotometricInterpretation::Rgb:
        return convertRgb(dicomImage);

    default:
        return QImage();
    }
}

/**
 * @brief Converts monochrome DICOM image to QImage with palette
 *
 * Supports both 8-bit and 16-bit pixel data. Applies window/level
 * transformation and optional color palette.
 *
 * @param image Source DICOM image
 * @param wl Window/level settings for contrast adjustment
 * @return QImage with applied palette
 */
QImage CImageConverter::convertMonochrome(const CDicomImage &image,
                                          const DicomViewer::SWindowLevel &wl) const
{
    const auto dims = image.dimensions();
    const auto &pixelData = image.pixelData();

    if (pixelData.empty())
    {
        return QImage();
    }

    // Verify data size matches expected dimensions
    const size_t pixelCount = static_cast<size_t>(dims.width) * dims.height;
    const bool is16bit = image.bitsPerSample() == 16;
    const size_t bytesPerPixel = is16bit ? 2 : 1;
    const size_t expectedSize = pixelCount * bytesPerPixel;
    if (pixelData.size() < expectedSize)
    {
        return QImage();
    }

    const bool useColorPalette = (m_palette.type() != DicomViewer::EPaletteType::Grayscale);
    const bool isMonochrome1 = (image.photometricInterpretation() ==
                                DicomViewer::EPhotometricInterpretation::Monochrome1);
    const bool isSigned = image.isPixelSigned();

    int minValue = 0;
    int maxValue = 255;
    if (is16bit)
    {
        if (isSigned)
        {
            minValue = -32768;
            maxValue = 32767;
        }
        else
        {
            minValue = 0;
            maxValue = 65535;
        }
    }

    const auto lut = buildWindowLevelLut(wl, minValue, maxValue, isMonochrome1);
    const int lutOffset = -minValue;

    if (!useColorPalette)
    {
        QImage result(static_cast<int>(dims.width),
                      static_cast<int>(dims.height),
                      QImage::Format_Grayscale8);

        if (is16bit)
        {
            const auto *src = reinterpret_cast<const uint16_t *>(pixelData.data());
            for (uint32_t y = 0; y < dims.height; ++y)
            {
                uint8_t *dstRow = result.scanLine(static_cast<int>(y));
                const size_t rowOffset = static_cast<size_t>(y) * dims.width;
                for (uint32_t x = 0; x < dims.width; ++x)
                {
                    const size_t index = rowOffset + x;
                    int pixelValue = isSigned
                                         ? static_cast<int>(reinterpret_cast<const int16_t *>(src)[index])
                                         : static_cast<int>(src[index]);
                    dstRow[x] = lut[static_cast<size_t>(pixelValue + lutOffset)];
                }
            }
        }
        else
        {
            const uint8_t *src = pixelData.data();
            for (uint32_t y = 0; y < dims.height; ++y)
            {
                uint8_t *dstRow = result.scanLine(static_cast<int>(y));
                const size_t rowOffset = static_cast<size_t>(y) * dims.width;
                for (uint32_t x = 0; x < dims.width; ++x)
                {
                    dstRow[x] = lut[src[rowOffset + x]];
                }
            }
        }

        return result;
    }

    std::array<std::array<uint8_t, 3>, 256> rgbLut{};
    for (int i = 0; i < 256; ++i)
    {
        auto rgb = m_palette.mapRgb(static_cast<uint8_t>(i));
        rgbLut[i] = {rgb[0], rgb[1], rgb[2]};
    }

    // Apply color palette - output as RGB
    QImage result(static_cast<int>(dims.width),
                  static_cast<int>(dims.height),
                  QImage::Format_RGB888);

    if (is16bit)
    {
        const auto *src = reinterpret_cast<const uint16_t *>(pixelData.data());
        for (uint32_t y = 0; y < dims.height; ++y)
        {
            uint8_t *dstRow = result.scanLine(static_cast<int>(y));
            const size_t rowOffset = static_cast<size_t>(y) * dims.width;
            for (uint32_t x = 0; x < dims.width; ++x)
            {
                const size_t index = rowOffset + x;
                int pixelValue = isSigned
                                     ? static_cast<int>(reinterpret_cast<const int16_t *>(src)[index])
                                     : static_cast<int>(src[index]);
                const uint8_t value = lut[static_cast<size_t>(pixelValue + lutOffset)];
                const auto &rgb = rgbLut[value];
                dstRow[x * 3 + 0] = rgb[0];
                dstRow[x * 3 + 1] = rgb[1];
                dstRow[x * 3 + 2] = rgb[2];
            }
        }
    }
    else
    {
        const uint8_t *src = pixelData.data();
        for (uint32_t y = 0; y < dims.height; ++y)
        {
            uint8_t *dstRow = result.scanLine(static_cast<int>(y));
            const size_t rowOffset = static_cast<size_t>(y) * dims.width;
            for (uint32_t x = 0; x < dims.width; ++x)
            {
                const uint8_t value = lut[src[rowOffset + x]];
                const auto &rgb = rgbLut[value];
                dstRow[x * 3 + 0] = rgb[0];
                dstRow[x * 3 + 1] = rgb[1];
                dstRow[x * 3 + 2] = rgb[2];
            }
        }
    }

    return result;
}

/**
 * @brief Converts RGB DICOM image to QImage
 * @param image Source DICOM image
 * @return RGB QImage
 */
QImage CImageConverter::convertRgb(const CDicomImage &image) const
{
    const auto dims = image.dimensions();
    const auto &pixelData = image.pixelData();

    if (pixelData.empty())
    {
        return QImage();
    }

    // Create RGB image
    QImage result(static_cast<int>(dims.width),
                  static_cast<int>(dims.height),
                  QImage::Format_RGB888);

    // Verify data size matches expected dimensions (3 bytes per pixel)
    const size_t expectedSize = static_cast<size_t>(dims.width) * dims.height * 3;
    if (pixelData.size() < expectedSize)
    {
        return QImage();
    }

    // Copy pixel data row by row
    const size_t rowBytes = static_cast<size_t>(dims.width) * 3;
    for (uint32_t y = 0; y < dims.height; ++y)
    {
        const uint8_t *srcRow = pixelData.data() + (y * rowBytes);
        uint8_t *dstRow = result.scanLine(static_cast<int>(y));
        std::memcpy(dstRow, srcRow, rowBytes);
    }

    return result;
}

std::vector<uint8_t> CImageConverter::buildWindowLevelLut(
    const DicomViewer::SWindowLevel &wl,
    int minValue,
    int maxValue,
    bool invertForMonochrome1) const
{
    if (maxValue < minValue)
    {
        std::swap(minValue, maxValue);
    }

    const int range = maxValue - minValue + 1;
    std::vector<uint8_t> lut(static_cast<size_t>(range));

    if (wl.width <= 0.0)
    {
        std::fill(lut.begin(), lut.end(), static_cast<uint8_t>(0));
        return lut;
    }

    const double lowerBound = wl.center - (wl.width / 2.0);
    const double upperBound = wl.center + (wl.width / 2.0);
    const double scale = 255.0 / wl.width;

    for (int i = 0; i < range; ++i)
    {
        const double rawValue = static_cast<double>(minValue + i);
        uint8_t value;

        if (rawValue <= lowerBound)
        {
            value = 0;
        }
        else if (rawValue >= upperBound)
        {
            value = 255;
        }
        else
        {
            value = static_cast<uint8_t>((rawValue - lowerBound) * scale);
        }

        if (invertForMonochrome1)
        {
            value = 255 - value;
        }

        lut[static_cast<size_t>(i)] = value;
    }

    return lut;
}
