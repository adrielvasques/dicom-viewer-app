/**
 * @file CDicomImage.cpp
 * @brief Implementation of the CDicomImage class
 * @author DICOM Viewer Project
 * @date 2026
 *
 * Implements the DICOM image container providing storage and access
 * to pixel data, image properties, and window/level parameters.
 */

#include "CDicomImage.h"

/**
 * @brief Retrieves the pixel data
 * @return Const reference to pixel data vector
 */
const std::vector<uint8_t> &CDicomImage::pixelData() const
{
    return m_pixelData;
}

/**
 * @brief Checks if pixel data is present
 * @return True if pixel data exists and is not empty
 */
bool CDicomImage::hasPixelData() const
{
    return !m_pixelData.empty();
}

/**
 * @brief Retrieves image dimensions
 * @return Struct containing width, height, and bit depth information
 */
DicomViewer::SImageDimensions CDicomImage::dimensions() const
{
    return m_dimensions;
}

/**
 * @brief Retrieves photometric interpretation
 * @return Enum indicating the color space (grayscale or RGB)
 */
DicomViewer::EPhotometricInterpretation CDicomImage::photometricInterpretation() const
{
    return m_photometricInterpretation;
}

/**
 * @brief Sets current window/level values for display
 * @param wl Window/level struct with center and width values
 */
void CDicomImage::setWindowLevel(const DicomViewer::SWindowLevel &wl)
{
    m_windowLevel = wl;
}

/**
 * @brief Retrieves current window/level values
 * @return Current window center and width settings
 */
DicomViewer::SWindowLevel CDicomImage::windowLevel() const
{
    return m_windowLevel;
}

/**
 * @brief Retrieves default window/level from DICOM header
 * @return Default window center and width from file
 */
DicomViewer::SWindowLevel CDicomImage::defaultWindowLevel() const
{
    return m_defaultWindowLevel;
}

/**
 * @brief Resets window/level to default values from DICOM header
 */
void CDicomImage::resetWindowLevel()
{
    m_windowLevel = m_defaultWindowLevel;
}

/**
 * @brief Retrieves rescale slope for Hounsfield unit conversion
 * @return Rescale slope value (default 1.0)
 */
double CDicomImage::rescaleSlope() const
{
    return m_rescaleSlope;
}

/**
 * @brief Retrieves rescale intercept for Hounsfield unit conversion
 * @return Rescale intercept value (default 0.0)
 */
double CDicomImage::rescaleIntercept() const
{
    return m_rescaleIntercept;
}

/**
 * @brief Retrieves associated metadata
 * @return Pointer to metadata object, nullptr if none set
 */
const CDicomMetadata *CDicomImage::metadata() const
{
    return m_metadata.get();
}

/**
 * @brief Checks if image is valid and displayable
 * @return True if image has valid pixel data and non-zero dimensions
 */
bool CDicomImage::isValid() const
{
    return hasPixelData() &&
           m_dimensions.width > 0 &&
           m_dimensions.height > 0 &&
           m_photometricInterpretation != DicomViewer::EPhotometricInterpretation::Unknown;
}

/**
 * @brief Clears all image data and resets to initial state
 */
void CDicomImage::clear()
{
    m_pixelData.clear();
    m_dimensions = DicomViewer::SImageDimensions{};
    m_photometricInterpretation = DicomViewer::EPhotometricInterpretation::Unknown;
    m_windowLevel = DicomViewer::SWindowLevel{};
    m_defaultWindowLevel = DicomViewer::SWindowLevel{};
    m_rescaleSlope = 1.0;
    m_rescaleIntercept = 0.0;
    m_bitsPerSample = 8;
    m_pixelSigned = false;
    m_metadata.reset();
}

/**
 * @brief Sets the pixel data using move semantics
 * @param data Vector of pixel data to move into the image
 */
void CDicomImage::setPixelData(std::vector<uint8_t> &&data)
{
    m_pixelData = std::move(data);
}

/**
 * @brief Sets image dimensions
 * @param dims Struct containing width, height, and bit depth info
 */
void CDicomImage::setDimensions(const DicomViewer::SImageDimensions &dims)
{
    m_dimensions = dims;
}

/**
 * @brief Sets photometric interpretation
 * @param pi Enum indicating grayscale or RGB format
 */
void CDicomImage::setPhotometricInterpretation(DicomViewer::EPhotometricInterpretation pi)
{
    m_photometricInterpretation = pi;
}

/**
 * @brief Sets default window/level values from DICOM header
 * @param wl Window/level struct with center and width
 */
void CDicomImage::setDefaultWindowLevel(const DicomViewer::SWindowLevel &wl)
{
    m_defaultWindowLevel = wl;
    m_windowLevel = wl;
}

/**
 * @brief Sets rescale slope for Hounsfield unit conversion
 * @param slope Rescale slope value
 */
void CDicomImage::setRescaleSlope(double slope)
{
    m_rescaleSlope = slope;
}

/**
 * @brief Sets rescale intercept for Hounsfield unit conversion
 * @param intercept Rescale intercept value
 */
void CDicomImage::setRescaleIntercept(double intercept)
{
    m_rescaleIntercept = intercept;
}

/**
 * @brief Sets associated metadata using move semantics
 * @param metadata Unique pointer to metadata object
 */
void CDicomImage::setMetadata(std::unique_ptr<CDicomMetadata> metadata)
{
    m_metadata = std::move(metadata);
}

/**
 * @brief Retrieves bits per sample
 * @return 8 or 16 depending on pixel data format
 */
uint8_t CDicomImage::bitsPerSample() const
{
    return m_bitsPerSample;
}

/**
 * @brief Checks if pixel data is signed
 * @return True if signed (e.g., CT images), false otherwise
 */
bool CDicomImage::isPixelSigned() const
{
    return m_pixelSigned;
}

/**
 * @brief Sets bits per sample
 * @param bits 8 or 16
 */
void CDicomImage::setBitsPerSample(uint8_t bits)
{
    m_bitsPerSample = bits;
}

/**
 * @brief Sets whether pixel data is signed
 * @param isSigned True for signed pixel data
 */
void CDicomImage::setPixelSigned(bool isSigned)
{
    m_pixelSigned = isSigned;
}
