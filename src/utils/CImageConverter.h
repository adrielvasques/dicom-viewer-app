/**
 * @file CImageConverter.h
 * @brief DICOM to QImage converter class declaration
 * @author DICOM Viewer Project
 * @date 2026
 *
 * Defines the CImageConverter class which converts DICOM pixel data
 * to Qt QImage objects for display, handling both grayscale and RGB
 * images with window/level transformation and color palettes.
 */

#pragma once

#include "CColorPalette.h"
#include "core/CDicomImage.h"

#include <QImage>
#include <vector>

/**
 * @class CImageConverter
 * @brief Converts DICOM image data to QImage for display
 *
 * Handles the conversion of DICOM pixel data to Qt's QImage format.
 * Supports both grayscale (MONOCHROME1, MONOCHROME2) and RGB images.
 * For grayscale images, applies window/level transformation and
 * optional color palettes for enhanced visualization.
 */
class CImageConverter
{
  public:
    /**
     * @brief Default constructor
     */
    CImageConverter() = default;

    /** @name Palette Management */
    ///@{
    /**
     * @brief Sets the color palette for grayscale images
     * @param type Palette type to use
     */
    void setPalette(DicomViewer::EPaletteType type);

    /**
     * @brief Gets the current palette type
     * @return Current palette type
     */
    DicomViewer::EPaletteType paletteType() const;

    /**
     * @brief Gets the current palette
     * @return Reference to the current palette
     */
    const CColorPalette &palette() const;
    ///@}

    /** @name Conversion Methods */
    ///@{
    /**
     * @brief Converts DICOM image to QImage using current window/level
     * @param dicomImage Source DICOM image
     * @return QImage ready for display, or null QImage on failure
     */
    QImage toQImage(const CDicomImage &dicomImage) const;

    /**
     * @brief Converts DICOM image to QImage with specific window/level
     * @param dicomImage Source DICOM image
     * @param windowLevel Custom window/level settings
     * @return QImage ready for display, or null QImage on failure
     */
    QImage toQImage(const CDicomImage &dicomImage,
                    const DicomViewer::SWindowLevel &windowLevel) const;
    ///@}

  private:
    /** @name Internal Conversion Methods */
    ///@{
    /**
     * @brief Converts monochrome DICOM image to QImage with palette
     * @param image Source DICOM image
     * @param wl Window/level settings for contrast adjustment
     * @return QImage with applied palette
     */
    QImage convertMonochrome(const CDicomImage &image,
                             const DicomViewer::SWindowLevel &wl) const;

    /**
     * @brief Converts RGB DICOM image to QImage
     * @param image Source DICOM image
     * @return RGB QImage
     */
    QImage convertRgb(const CDicomImage &image) const;

    /**
     * @brief Builds lookup table for window/level transformation
     * @param wl Window/level settings
     * @param minValue Minimum input value
     * @param maxValue Maximum input value
     * @param invertForMonochrome1 True to invert output values
     * @return Lookup table mapping input values to 8-bit output
     */
    std::vector<uint8_t> buildWindowLevelLut(
        const DicomViewer::SWindowLevel &wl,
        int minValue,
        int maxValue,
        bool invertForMonochrome1) const;
    ///@}

    CColorPalette m_palette; /**< Color palette for grayscale images */
};
