/**
 * @file CColorPalette.h
 * @brief Color palette system for pseudo-coloring grayscale images
 * @author DICOM Viewer Project
 * @date 2026
 *
 * Defines color palettes (LUTs) that can be applied to grayscale
 * DICOM images for enhanced visualization.
 */

#pragma once

#include "DicomViewer/Types.h"

#include <QColor>
#include <QString>
#include <array>
#include <vector>

/**
 * @class CColorPalette
 * @brief Manages color palettes for image pseudo-coloring
 *
 * Provides 256-entry color lookup tables (LUTs) that map grayscale
 * values to RGB colors for enhanced visualization of medical images.
 */
class CColorPalette
{
  public:
    /**
     * @brief Constructor - creates default grayscale palette
     */
    CColorPalette();

    /**
     * @brief Constructor with specific palette type
     * @param type Palette type to create
     */
    explicit CColorPalette(DicomViewer::EPaletteType type);

    /** @name Palette Access */
    ///@{
    /**
     * @brief Gets the current palette type
     * @return Current palette type
     */
    DicomViewer::EPaletteType type() const;

    /**
     * @brief Sets the palette type
     * @param type New palette type
     */
    void setType(DicomViewer::EPaletteType type);

    /**
     * @brief Maps a grayscale value to RGB color
     * @param grayValue Input grayscale value (0-255)
     * @return RGB color from the palette
     */
    QColor mapColor(uint8_t grayValue) const;

    /**
     * @brief Gets RGB components for a grayscale value
     * @param grayValue Input grayscale value (0-255)
     * @return Array with R, G, B values
     */
    std::array<uint8_t, 3> mapRgb(uint8_t grayValue) const;

    /**
     * @brief Gets the palette name for display
     * @return Palette name
     */
    QString name() const;
    ///@}

    /** @name Static Helpers */
    ///@{
    /**
     * @brief Gets all available palette types
     * @return Vector of all palette types
     */
    static std::vector<DicomViewer::EPaletteType> availablePalettes();

    /**
     * @brief Gets the name for a palette type
     * @param type Palette type
     * @return String Type
     */
    static QString paletteName(DicomViewer::EPaletteType type);
    ///@}

  private:
    /**
     * @brief Generates the lookup table for current palette type
     */
    void generateLut();

    /**
     * @brief Generates grayscale LUT
     */
    void generateGrayscale();

    /**
     * @brief Generates inverted grayscale LUT
     */
    void generateInverted();

    /**
     * @brief Generates hot/thermal LUT
     */
    void generateHot();

    /**
     * @brief Generates cool LUT
     */
    void generateCool();

    /**
     * @brief Generates rainbow LUT
     */
    void generateRainbow();

    /**
     * @brief Generates bone LUT
     */
    void generateBone();

    /**
     * @brief Generates copper LUT
     */
    void generateCopper();

    /**
     * @brief Generates ocean LUT
     */
    void generateOcean();

    DicomViewer::EPaletteType m_type = DicomViewer::EPaletteType::Grayscale; /**< Current palette type */
    std::array<std::array<uint8_t, 3>, 256> m_lut; /**< RGB lookup table */
};
