/**
 * @file CColorPalette.cpp
 * @brief Implementation of the CColorPalette class
 * @author DICOM Viewer Project
 * @date 2026
 *
 * Implements color lookup tables (LUTs) for pseudo-coloring
 * grayscale medical images.
 */

#include "CColorPalette.h"
#include <QObject>
#include <algorithm>
#include <cmath>

/**
 * @brief Default constructor - creates grayscale palette
 */
CColorPalette::CColorPalette()
    : m_type(DicomViewer::EPaletteType::Grayscale)
{
    generateLut();
}

/**
 * @brief Constructor with specific palette type
 * @param type Palette type to create
 */
CColorPalette::CColorPalette(DicomViewer::EPaletteType type)
    : m_type(type)
{
    generateLut();
}

/**
 * @brief Gets the current palette type
 * @return Current palette type
 */
DicomViewer::EPaletteType CColorPalette::type() const
{
    return m_type;
}

/**
 * @brief Sets the palette type and regenerates LUT
 * @param type New palette type
 */
void CColorPalette::setType(DicomViewer::EPaletteType type)
{
    if (m_type != type)
    {
        m_type = type;
        generateLut();
    }
}

/**
 * @brief Maps a grayscale value to RGB color
 * @param grayValue Input grayscale value (0-255)
 * @return QColor from the palette
 */
QColor CColorPalette::mapColor(uint8_t grayValue) const
{
    const auto &rgb = m_lut[grayValue];
    return QColor(rgb[0], rgb[1], rgb[2]);
}

/**
 * @brief Gets RGB components for a grayscale value
 * @param grayValue Input grayscale value (0-255)
 * @return Array with R, G, B values
 */
std::array<uint8_t, 3> CColorPalette::mapRgb(uint8_t grayValue) const
{
    return m_lut[grayValue];
}

/**
 * @brief Gets the palette name for display
 * @return Human-readable palette name
 */
QString CColorPalette::name() const
{
    return paletteName(m_type);
}

/**
 * @brief Gets all available palette types
 * @return Vector of all palette types
 */
std::vector<DicomViewer::EPaletteType> CColorPalette::availablePalettes()
{
    return {
        DicomViewer::EPaletteType::Grayscale,
        DicomViewer::EPaletteType::Inverted,
        DicomViewer::EPaletteType::Hot,
        DicomViewer::EPaletteType::Cool,
        DicomViewer::EPaletteType::Rainbow,
        DicomViewer::EPaletteType::Bone,
        DicomViewer::EPaletteType::Copper,
        DicomViewer::EPaletteType::Ocean};
}

/**
 * @brief Gets the name for a palette type
 * @param type Palette type
 * @return Human-readable name
 */
QString CColorPalette::paletteName(DicomViewer::EPaletteType type)
{
    switch (type)
    {
    case DicomViewer::EPaletteType::Grayscale:
        return QObject::tr("Grayscale");
    case DicomViewer::EPaletteType::Inverted:
        return QObject::tr("Inverted");
    case DicomViewer::EPaletteType::Hot:
        return QObject::tr("Hot (Thermal)");
    case DicomViewer::EPaletteType::Cool:
        return QObject::tr("Cool");
    case DicomViewer::EPaletteType::Rainbow:
        return QObject::tr("Rainbow");
    case DicomViewer::EPaletteType::Bone:
        return QObject::tr("Bone");
    case DicomViewer::EPaletteType::Copper:
        return QObject::tr("Copper");
    case DicomViewer::EPaletteType::Ocean:
        return QObject::tr("Ocean");
    default:
        return QObject::tr("Unknown");
    }
}

/**
 * @brief Generates the lookup table for current palette type
 */
void CColorPalette::generateLut()
{
    switch (m_type)
    {
    case DicomViewer::EPaletteType::Grayscale:
        generateGrayscale();
        break;
    case DicomViewer::EPaletteType::Inverted:
        generateInverted();
        break;
    case DicomViewer::EPaletteType::Hot:
        generateHot();
        break;
    case DicomViewer::EPaletteType::Cool:
        generateCool();
        break;
    case DicomViewer::EPaletteType::Rainbow:
        generateRainbow();
        break;
    case DicomViewer::EPaletteType::Bone:
        generateBone();
        break;
    case DicomViewer::EPaletteType::Copper:
        generateCopper();
        break;
    case DicomViewer::EPaletteType::Ocean:
        generateOcean();
        break;
    default:
        generateGrayscale();
        break;
    }
}

/**
 * @brief Generates grayscale LUT (identity mapping)
 */
void CColorPalette::generateGrayscale()
{
    for (int i = 0; i < 256; ++i)
    {
        m_lut[i] = {static_cast<uint8_t>(i),
                    static_cast<uint8_t>(i),
                    static_cast<uint8_t>(i)};
    }
}

/**
 * @brief Generates inverted grayscale LUT
 */
void CColorPalette::generateInverted()
{
    for (int i = 0; i < 256; ++i)
    {
        uint8_t v = static_cast<uint8_t>(255 - i);
        m_lut[i] = {v, v, v};
    }
}

/**
 * @brief Generates hot/thermal LUT (black-red-yellow-white)
 */
void CColorPalette::generateHot()
{
    for (int i = 0; i < 256; ++i)
    {
        double t = i / 255.0;
        uint8_t r, g, b;

        // Red channel: ramps up in first third
        if (t < 0.375)
        {
            r = static_cast<uint8_t>(t / 0.375 * 255);
        }
        else
        {
            r = 255;
        }

        // Green channel: ramps up in second third
        if (t < 0.375)
        {
            g = 0;
        }
        else if (t < 0.75)
        {
            g = static_cast<uint8_t>((t - 0.375) / 0.375 * 255);
        }
        else
        {
            g = 255;
        }

        // Blue channel: ramps up in last quarter
        if (t < 0.75)
        {
            b = 0;
        }
        else
        {
            b = static_cast<uint8_t>((t - 0.75) / 0.25 * 255);
        }

        m_lut[i] = {r, g, b};
    }
}

/**
 * @brief Generates cool LUT (cyan to magenta through blue)
 */
void CColorPalette::generateCool()
{
    for (int i = 0; i < 256; ++i)
    {
        double t = i / 255.0;
        uint8_t r = static_cast<uint8_t>(t * 255);
        uint8_t g = static_cast<uint8_t>((1.0 - t) * 255);
        uint8_t b = 255;
        m_lut[i] = {r, g, b};
    }
}

/**
 * @brief Generates rainbow LUT (full spectrum)
 */
void CColorPalette::generateRainbow()
{
    for (int i = 0; i < 256; ++i)
    {
        double t = i / 255.0;
        double hue = t * 300.0; // 0 to 300 degrees (red to magenta)

        // HSV to RGB conversion (S=1, V=1)
        double c = 1.0;
        double x = c * (1.0 - std::abs(std::fmod(hue / 60.0, 2.0) - 1.0));
        double r1, g1, b1;

        if (hue < 60)
        {
            r1 = c;
            g1 = x;
            b1 = 0;
        }
        else if (hue < 120)
        {
            r1 = x;
            g1 = c;
            b1 = 0;
        }
        else if (hue < 180)
        {
            r1 = 0;
            g1 = c;
            b1 = x;
        }
        else if (hue < 240)
        {
            r1 = 0;
            g1 = x;
            b1 = c;
        }
        else
        {
            r1 = x;
            g1 = 0;
            b1 = c;
        }

        m_lut[i] = {static_cast<uint8_t>(r1 * 255),
                    static_cast<uint8_t>(g1 * 255),
                    static_cast<uint8_t>(b1 * 255)};
    }
}

/**
 * @brief Generates bone LUT (grayscale with blue tint)
 */
void CColorPalette::generateBone()
{
    for (int i = 0; i < 256; ++i)
    {
        double t = i / 255.0;
        uint8_t r, g, b;

        if (t < 0.375)
        {
            r = static_cast<uint8_t>(t / 0.375 * 0.746 * 255);
            g = static_cast<uint8_t>(t / 0.375 * 0.746 * 255);
            b = static_cast<uint8_t>((t / 0.375 * 0.746 + 0.254 * t / 0.375) * 255);
        }
        else if (t < 0.75)
        {
            double t2 = (t - 0.375) / 0.375;
            r = static_cast<uint8_t>((0.746 + t2 * 0.254) * 255);
            g = static_cast<uint8_t>((0.746 + t2 * 0.254) * 255);
            b = static_cast<uint8_t>((0.746 + 0.254 + t2 * 0.0) * 255);
        }
        else
        {
            double t3 = (t - 0.75) / 0.25;
            r = static_cast<uint8_t>((0.746 + 0.254 * t3 + 0.254) * 255);
            g = static_cast<uint8_t>(255);
            b = static_cast<uint8_t>(255);
        }

        // Simplified bone palette
        r = static_cast<uint8_t>(std::min(255.0, t * 255 * 0.9 + t * t * 25.5));
        g = static_cast<uint8_t>(std::min(255.0, t * 255 * 0.9 + t * t * 25.5));
        b = static_cast<uint8_t>(std::min(255.0, t * 255 * 1.0));

        m_lut[i] = {r, g, b};
    }
}

/**
 * @brief Generates copper LUT (dark to copper/orange tones)
 */
void CColorPalette::generateCopper()
{
    for (int i = 0; i < 256; ++i)
    {
        double t = i / 255.0;

        uint8_t r = static_cast<uint8_t>(std::min(255.0, t * 1.25 * 255));
        uint8_t g = static_cast<uint8_t>(t * 0.7812 * 255);
        uint8_t b = static_cast<uint8_t>(t * 0.4975 * 255);

        m_lut[i] = {r, g, b};
    }
}

/**
 * @brief Generates ocean LUT (green to blue tones)
 */
void CColorPalette::generateOcean()
{
    for (int i = 0; i < 256; ++i)
    {
        double t = i / 255.0;

        uint8_t r = static_cast<uint8_t>(t * t * 255);
        uint8_t g = static_cast<uint8_t>(t * 255);
        uint8_t b = static_cast<uint8_t>((0.4 + 0.6 * t) * 255);

        m_lut[i] = {r, g, b};
    }
}
