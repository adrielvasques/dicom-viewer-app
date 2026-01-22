/**
 * @file CDicomLoader.h
 * @brief DICOM file loader class declaration
 * @author DICOM Viewer Project
 * @date 2026
 *
 * Defines the CDicomLoader class which handles loading DICOM files
 * using the DCMTK library. Extracts image data, pixel data, and
 * metadata from DICOM files.
 */

#pragma once

#include "CDicomImage.h"
#include "DicomViewer/Types.h"

#include <memory>
#include <string>
#include <tuple>

/**
 * @class CDicomLoader
 * @brief Loads and parses DICOM files using DCMTK
 *
 * Provides functionality to load DICOM files from disk, validate
 * their format, extract image data (including pixel data), and
 * extract metadata. Uses DCMTK's DicomImage class for proper
 * rendering pipeline support.
 *
 * Usage:
 * @code
 * CDicomLoader loader;
 * auto [image, result] = loader.loadFile("/path/to/file.dcm");
 * if (result == DicomViewer::ELoadResult::Success) {
 *     // Use image...
 * }
 * @endcode
 */
class CDicomLoader
{
  public:
    /**
     * @brief Default constructor
     */
    CDicomLoader() = default;

    /**
     * @brief Destructor
     */
    ~CDicomLoader() = default;

    /** @name Non-copyable, Movable */
    ///@{
    CDicomLoader(const CDicomLoader &) = delete;
    CDicomLoader &operator=(const CDicomLoader &) = delete;
    CDicomLoader(CDicomLoader &&) = default;
    CDicomLoader &operator=(CDicomLoader &&) = default;
    ///@}

    /** @name Main Interface */
    ///@{
    /**
     * @brief Loads a DICOM file from disk
     * @param filePath Path to the DICOM file
     * @return Tuple containing the loaded image (or nullptr) and result code
     */
    std::tuple<std::unique_ptr<CDicomImage>, DicomViewer::ELoadResult>
    loadFile(const std::string &filePath);

    /**
     * @brief Validates if a file is a valid DICOM file
     * @param filePath Path to the file to validate
     * @return True if the file is a valid DICOM file
     */
    static bool isValidDicomFile(const std::string &filePath);

    /**
     * @brief Converts a load result to a human-readable error message
     * @param result The load result code
     * @return Localized error message string
     */
    static std::string errorMessage(DicomViewer::ELoadResult result);
    ///@}

  private:
    /** @name Internal Helper Methods */
    ///@{
    /**
     * @brief Extracts image properties from DICOM dataset
     * @param dcmDataset Pointer to DcmDataset
     * @param image Target image to populate
     * @return True if extraction succeeded
     */
    bool extractImageData(void *dcmDataset, CDicomImage &image);

    /**
     * @brief Extracts metadata from DICOM dataset
     * @param dcmDataset Pointer to DcmDataset
     * @param metadata Target metadata to populate
     * @return True if extraction succeeded
     */
    bool extractMetadata(void *dcmDataset, CDicomMetadata &metadata);

    /**
     * @brief Extracts pixel data using DicomImage rendering pipeline
     * @param dcmFileFormat Pointer to DcmFileFormat
     * @param image Target image to populate
     * @return True if extraction succeeded
     */
    bool extractPixelData(void *dcmFileFormat, CDicomImage &image);

    /**
     * @brief Parses photometric interpretation string
     * @param piString Photometric interpretation from DICOM
     * @return Corresponding enum value
     */
    DicomViewer::EPhotometricInterpretation
    parsePhotometricInterpretation(const char *piString);
    ///@}
};
