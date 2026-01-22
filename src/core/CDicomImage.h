/**
 * @file CDicomImage.h
 * @brief DICOM image data container class declaration
 * @author DICOM Viewer Project
 * @date 2026
 *
 * Defines the CDicomImage class which encapsulates DICOM image data
 * including pixel data, dimensions, photometric interpretation,
 * window/level parameters, and associated metadata.
 */

#pragma once

#include "CDicomMetadata.h"
#include "DicomViewer/Types.h"

#include <cstdint>
#include <memory>
#include <vector>

class CDicomLoader;

/**
 * @class CDicomImage
 * @brief Container for DICOM image data
 *
 * Encapsulates all data associated with a DICOM image including:
 * - Raw pixel data
 * - Image dimensions and bit depth
 * - Photometric interpretation (grayscale/RGB)
 * - Window/level parameters for display
 * - Rescale slope/intercept for Hounsfield units
 * - Associated metadata
 *
 * Pixel data setters are private and accessible only via CDicomLoader.
 */
class CDicomImage
{
    friend class CDicomLoader;

  public:
    /**
     * @brief Default constructor
     */
    CDicomImage() = default;

    /** @name Pixel Data Access */
    ///@{
    /**
     * @brief Retrieves the pixel data
     * @return Const reference to pixel data vector
     */
    const std::vector<uint8_t> &pixelData() const;

    /**
     * @brief Checks if pixel data is present
     * @return True if pixel data exists
     */
    bool hasPixelData() const;
    ///@}

    /** @name Image Properties */
    ///@{
    /**
     * @brief Retrieves image dimensions
     * @return Struct containing width, height, bit depth info
     */
    DicomViewer::SImageDimensions dimensions() const;

    /**
     * @brief Retrieves photometric interpretation
     * @return Enum indicating grayscale or RGB format
     */
    DicomViewer::EPhotometricInterpretation photometricInterpretation() const;
    ///@}

    /** @name Window/Level Parameters */
    ///@{
    /**
     * @brief Sets current window/level values
     * @param wl Window/level struct with center and width
     */
    void setWindowLevel(const DicomViewer::SWindowLevel &wl);

    /**
     * @brief Retrieves current window/level values
     * @return Current window/level settings
     */
    DicomViewer::SWindowLevel windowLevel() const;

    /**
     * @brief Retrieves default window/level from DICOM header
     * @return Default window/level settings
     */
    DicomViewer::SWindowLevel defaultWindowLevel() const;

    /**
     * @brief Resets window/level to default values
     */
    void resetWindowLevel();
    ///@}

    /** @name Rescale Parameters */
    ///@{
    /**
     * @brief Retrieves rescale slope for Hounsfield units
     * @return Rescale slope value
     */
    double rescaleSlope() const;

    /**
     * @brief Retrieves rescale intercept for Hounsfield units
     * @return Rescale intercept value
     */
    double rescaleIntercept() const;
    ///@}

    /** @name Pixel Format */
    ///@{
    /**
     * @brief Retrieves bits per sample (8 or 16)
     * @return Bits per sample value
     */
    uint8_t bitsPerSample() const;

    /**
     * @brief Checks if pixel data is signed
     * @return True if signed, false if unsigned
     */
    bool isPixelSigned() const;
    ///@}

    /** @name Metadata Access */
    ///@{
    /**
     * @brief Retrieves associated metadata
     * @return Pointer to metadata, nullptr if none
     */
    const CDicomMetadata *metadata() const;
    ///@}

    /** @name Validity */
    ///@{
    /**
     * @brief Checks if image is valid and displayable
     * @return True if image has valid pixel data and dimensions
     */
    bool isValid() const;

    /**
     * @brief Clears all image data
     */
    void clear();
    ///@}

  private:
    /** @name Private Setters (accessed by CDicomLoader) */
    ///@{
    void setPixelData(std::vector<uint8_t> &&data);
    void setDimensions(const DicomViewer::SImageDimensions &dims);
    void setPhotometricInterpretation(DicomViewer::EPhotometricInterpretation pi);
    void setDefaultWindowLevel(const DicomViewer::SWindowLevel &wl);
    void setRescaleSlope(double slope);
    void setRescaleIntercept(double intercept);
    void setMetadata(std::unique_ptr<CDicomMetadata> metadata);
    void setBitsPerSample(uint8_t bits);
    void setPixelSigned(bool isSigned);
    ///@}

    std::vector<uint8_t> m_pixelData;           /**< Raw pixel data */
    DicomViewer::SImageDimensions m_dimensions; /**< Image dimensions */
    DicomViewer::EPhotometricInterpretation m_photometricInterpretation =
        DicomViewer::EPhotometricInterpretation::Unknown; /**< Color space */

    DicomViewer::SWindowLevel m_windowLevel;        /**< Current window/level */
    DicomViewer::SWindowLevel m_defaultWindowLevel; /**< Default window/level */

    double m_rescaleSlope = 1.0;     /**< Rescale slope */
    double m_rescaleIntercept = 0.0; /**< Rescale intercept */

    uint8_t m_bitsPerSample = 8; /**< Bits per sample (8 or 16) */
    bool m_pixelSigned = false;  /**< True if pixel data is signed */

    std::unique_ptr<CDicomMetadata> m_metadata; /**< Associated metadata */
};
