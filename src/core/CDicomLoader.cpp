/**
 * @file CDicomLoader.cpp
 * @brief Implementation of the CDicomLoader class
 * @author DICOM Viewer Project
 * @date 2026
 *
 * Implements DICOM file loading using DCMTK library. Handles file
 * parsing, metadata extraction, and pixel data retrieval through
 * DCMTK's rendering pipeline.
 */

#include "CDicomLoader.h"

#include <dcmtk/config/osconfig.h>
#include <dcmtk/dcmdata/dctk.h>
#include <dcmtk/dcmdata/dcxfer.h>
#include <dcmtk/dcmimage/diregist.h>
#include <dcmtk/dcmimgle/dcmimage.h>
#include <dcmtk/dcmimgle/dipixel.h>
#include <dcmtk/dcmjpeg/djdecode.h>
#include <dcmtk/dcmjpeg/djencode.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <filesystem>

namespace
{
/**
 * @brief Registra os codecs JPEG do DCMTK (executado uma vez)
 */
struct SCodecRegistration
{
    SCodecRegistration()
    {
        DJDecoderRegistration::registerCodecs();
    }
    ~SCodecRegistration()
    {
        DJDecoderRegistration::cleanup();
    }
};
static SCodecRegistration s_codecRegistration;
} // namespace

/**
 * @brief Loads a DICOM file from disk
 * @param filePath Path to the DICOM file
 * @return Tuple containing the loaded image (or nullptr on failure) and result code
 */
std::tuple<std::unique_ptr<CDicomImage>, DicomViewer::ELoadResult>
CDicomLoader::loadFile(const std::string &filePath)
{
    auto image = std::make_unique<CDicomImage>();

    // Check if file exists
    if (!std::filesystem::exists(filePath))
    {
        return {nullptr, DicomViewer::ELoadResult::FileNotFound};
    }

    // Load DICOM file using DCMTK
    DcmFileFormat fileFormat;
    OFCondition status = fileFormat.loadFile(filePath.c_str());

    if (status.bad())
    {
        return {nullptr, DicomViewer::ELoadResult::InvalidFormat};
    }

    DcmDataset *dataset = fileFormat.getDataset();
    if (dataset == nullptr)
    {
        return {nullptr, DicomViewer::ELoadResult::InvalidFormat};
    }

    // Extract metadata
    auto metadata = std::make_unique<CDicomMetadata>();
    extractMetadata(dataset, *metadata);
    image->setMetadata(std::move(metadata));

    // Extract image properties
    if (!extractImageData(dataset, *image))
    {
        return {nullptr, DicomViewer::ELoadResult::InvalidFormat};
    }

    // Extract pixel data using DicomImage for proper rendering
    if (!extractPixelData(&fileFormat, *image))
    {
        return {nullptr, DicomViewer::ELoadResult::DecompressionFailed};
    }

    return {std::move(image), DicomViewer::ELoadResult::Success};
}

/**
 * @brief Validates if a file is a valid DICOM file
 * @param filePath Path to the file to validate
 * @return True if the file appears to be a valid DICOM file
 */
bool CDicomLoader::isValidDicomFile(const std::string &filePath)
{
    DcmFileFormat fileFormat;
    OFCondition status = fileFormat.loadFile(filePath.c_str());
    return status.good();
}

/**
 * @brief Converts a load result to a human-readable error message
 * @param result The load result code
 * @return Localized error message string
 */
std::string CDicomLoader::errorMessage(DicomViewer::ELoadResult result)
{
    switch (result)
    {
    case DicomViewer::ELoadResult::Success:
        return std::string();
    case DicomViewer::ELoadResult::FileNotFound:
        return "File not found. Please check the file path.";
    case DicomViewer::ELoadResult::InvalidFormat:
        return "Invalid DICOM file format. The file may be corrupted "
               "or is not a valid DICOM file.";
    case DicomViewer::ELoadResult::UnsupportedTransferSyntax:
        return "Unsupported transfer syntax. This DICOM compression "
               "format is not supported.";
    case DicomViewer::ELoadResult::DecompressionFailed:
        return "Failed to decompress or render image data.";
    default:
        return "An unknown error occurred while loading the file.";
    }
}

/**
 * @brief Extracts image properties from DICOM dataset
 * @param dcmDataset Pointer to DcmDataset (void* to avoid header exposure)
 * @param image Target image to populate with properties
 * @return True if extraction succeeded
 */
bool CDicomLoader::extractImageData(void *dcmDataset, CDicomImage &image)
{
    DcmDataset *dataset = static_cast<DcmDataset *>(dcmDataset);

    DicomViewer::SImageDimensions dims;

    // Extract dimensions
    Uint16 rows = 0, columns = 0;
    if (dataset->findAndGetUint16(DCM_Rows, rows).good() &&
        dataset->findAndGetUint16(DCM_Columns, columns).good())
    {
        dims.height = rows;
        dims.width = columns;
    }
    else
    {
        return false;
    }

    // Extract bit depth information
    Uint16 bitsAllocated = 0, bitsStored = 0, highBit = 0;
    dataset->findAndGetUint16(DCM_BitsAllocated, bitsAllocated);
    dataset->findAndGetUint16(DCM_BitsStored, bitsStored);
    dataset->findAndGetUint16(DCM_HighBit, highBit);
    dims.bitsAllocated = bitsAllocated;
    dims.bitsStored = bitsStored;
    dims.highBit = highBit;

    // Extract samples per pixel
    Uint16 samplesPerPixel = 1;
    dataset->findAndGetUint16(DCM_SamplesPerPixel, samplesPerPixel);
    dims.samplesPerPixel = samplesPerPixel;

    // Check pixel representation (signed/unsigned)
    Uint16 pixelRepresentation = 0;
    dataset->findAndGetUint16(DCM_PixelRepresentation, pixelRepresentation);
    dims.isSigned = (pixelRepresentation == 1);

    image.setDimensions(dims);

    // Extract photometric interpretation
    OFString piString;
    if (dataset->findAndGetOFString(DCM_PhotometricInterpretation, piString).good())
    {
        image.setPhotometricInterpretation(
            parsePhotometricInterpretation(piString.c_str()));
    }

    // Extract rescale parameters
    Float64 rescaleSlope = 1.0, rescaleIntercept = 0.0;
    if (dataset->findAndGetFloat64(DCM_RescaleSlope, rescaleSlope).good())
    {
        image.setRescaleSlope(rescaleSlope);
    }
    if (dataset->findAndGetFloat64(DCM_RescaleIntercept, rescaleIntercept).good())
    {
        image.setRescaleIntercept(rescaleIntercept);
    }

    return true;
}

/**
 * @brief Extracts metadata from DICOM dataset
 * @param dcmDataset Pointer to DcmDataset
 * @param metadata Target metadata to populate
 * @return True if extraction succeeded
 */
bool CDicomLoader::extractMetadata(void *dcmDataset, CDicomMetadata &metadata)
{
    DcmDataset *dataset = static_cast<DcmDataset *>(dcmDataset);
    OFString value;

    // Patient information
    if (dataset->findAndGetOFStringArray(DCM_PatientName, value).good())
    {
        metadata.setPatientName(value.c_str());
    }
    if (dataset->findAndGetOFString(DCM_PatientID, value).good())
    {
        metadata.setPatientId(value.c_str());
    }
    if (dataset->findAndGetOFString(DCM_PatientBirthDate, value).good())
    {
        metadata.setPatientBirthDate(value.c_str());
    }
    if (dataset->findAndGetOFString(DCM_PatientSex, value).good())
    {
        metadata.setPatientSex(value.c_str());
    }

    // Study information
    if (dataset->findAndGetOFString(DCM_StudyDate, value).good())
    {
        metadata.setStudyDate(value.c_str());
    }
    if (dataset->findAndGetOFString(DCM_StudyTime, value).good())
    {
        metadata.setStudyTime(value.c_str());
    }
    if (dataset->findAndGetOFString(DCM_StudyDescription, value).good())
    {
        metadata.setStudyDescription(value.c_str());
    }
    if (dataset->findAndGetOFString(DCM_AccessionNumber, value).good())
    {
        metadata.setAccessionNumber(value.c_str());
    }

    // Series information
    if (dataset->findAndGetOFString(DCM_SeriesDescription, value).good())
    {
        metadata.setSeriesDescription(value.c_str());
    }
    if (dataset->findAndGetOFString(DCM_Modality, value).good())
    {
        metadata.setModality(value.c_str());
    }
    if (dataset->findAndGetOFString(DCM_SeriesNumber, value).good())
    {
        metadata.setSeriesNumber(value.c_str());
    }

    // Image information
    if (dataset->findAndGetOFString(DCM_InstanceNumber, value).good())
    {
        metadata.setInstanceNumber(value.c_str());
    }
    if (dataset->findAndGetOFStringArray(DCM_ImagePositionPatient, value).good())
    {
        metadata.setImagePositionPatient(value.c_str());
    }
    if (dataset->findAndGetOFString(DCM_SliceThickness, value).good())
    {
        metadata.setSliceThickness(value.c_str());
    }
    if (dataset->findAndGetOFString(DCM_PhotometricInterpretation, value).good())
    {
        metadata.setTag("Photometric Interpretation", value.c_str());
    }
    if (dataset->findAndGetOFString(DCM_TransferSyntaxUID, value).good())
    {
        metadata.setTag("Transfer Syntax", value.c_str());
    }

    // Numeric values as strings for display
    Uint16 numValue = 0;
    if (dataset->findAndGetUint16(DCM_Rows, numValue).good())
    {
        metadata.setRows(std::to_string(numValue));
    }
    if (dataset->findAndGetUint16(DCM_Columns, numValue).good())
    {
        metadata.setColumns(std::to_string(numValue));
    }
    if (dataset->findAndGetUint16(DCM_BitsAllocated, numValue).good())
    {
        metadata.setBitsAllocated(std::to_string(numValue));
    }

    // Window/Level values
    Float64 floatValue = 0.0;
    if (dataset->findAndGetFloat64(DCM_WindowCenter, floatValue).good())
    {
        metadata.setWindowCenter(std::to_string(floatValue));
    }
    if (dataset->findAndGetFloat64(DCM_WindowWidth, floatValue).good())
    {
        metadata.setWindowWidth(std::to_string(floatValue));
    }

    return true;
}

/**
 * @brief Extracts pixel data using DicomImage rendering pipeline
 * @param dcmFileFormat Pointer to DcmFileFormat
 * @param image Target image to populate with pixel data
 * @return True if extraction succeeded
 */
bool CDicomLoader::extractPixelData(void *dcmFileFormat, CDicomImage &image)
{
    DcmFileFormat *fileFormat = static_cast<DcmFileFormat *>(dcmFileFormat);

    // Use DicomImage for proper rendering pipeline (handles Modality LUT)
    DicomImage dcmImage(fileFormat, EXS_Unknown);

    if (dcmImage.getStatus() != EIS_Normal)
    {
        return false;
    }

    // Set window/level from DICOM or calculate from min/max
    double windowCenter = 0.0, windowWidth = 0.0;
    if (dcmImage.getWindow(windowCenter, windowWidth))
    {
        image.setDefaultWindowLevel({windowCenter, windowWidth});
    }
    else
    {
        // Calculate window from image min/max
        dcmImage.setMinMaxWindow();
        dcmImage.getWindow(windowCenter, windowWidth);
        image.setDefaultWindowLevel({windowCenter, windowWidth});
    }

    // Get intermediate data to preserve original bit depth
    const DiPixel *interData = dcmImage.getInterData();
    if (interData == nullptr)
    {
        return false;
    }

    EP_Representation rep = interData->getRepresentation();
    const void *pixelData = interData->getData();
    const unsigned long pixelCount = interData->getCount();

    if (pixelData == nullptr || pixelCount == 0)
    {
        return false;
    }

    // Copy pixel data based on representation
    if (rep == EPR_Uint8)
    {
        // 8-bit unsigned
        std::vector<uint8_t> data(pixelCount);
        std::memcpy(data.data(), pixelData, pixelCount);
        image.setPixelData(std::move(data));
        image.setBitsPerSample(8);
        image.setPixelSigned(false);
    }
    else if (rep == EPR_Uint16)
    {
        // 16-bit unsigned
        std::vector<uint8_t> data(pixelCount * 2);
        std::memcpy(data.data(), pixelData, pixelCount * 2);
        image.setPixelData(std::move(data));
        image.setBitsPerSample(16);
        image.setPixelSigned(false);
    }
    else if (rep == EPR_Sint16)
    {
        // 16-bit signed
        std::vector<uint8_t> data(pixelCount * 2);
        std::memcpy(data.data(), pixelData, pixelCount * 2);
        image.setPixelData(std::move(data));
        image.setBitsPerSample(16);
        image.setPixelSigned(true);
    }
    else
    {
        // Fallback to 8-bit output for unsupported formats
        constexpr int kOutputBitDepth = 8;
        const void *outputData = dcmImage.getOutputData(kOutputBitDepth);
        if (outputData == nullptr)
        {
            return false;
        }
        size_t dataSize = dcmImage.getOutputDataSize(kOutputBitDepth);
        if (dataSize == 0)
        {
            return false;
        }
        std::vector<uint8_t> data(dataSize);
        std::memcpy(data.data(), outputData, dataSize);
        image.setPixelData(std::move(data));
        image.setBitsPerSample(8);
        image.setPixelSigned(false);
        // For fallback, use 8-bit W/L range
        image.setDefaultWindowLevel({128.0, 256.0});
    }

    // Update dimensions from DicomImage (may differ from dataset if interpolated)
    auto dims = image.dimensions();
    dims.width = dcmImage.getWidth();
    dims.height = dcmImage.getHeight();
    dims.samplesPerPixel = dcmImage.isMonochrome() ? 1 : 3;
    image.setDimensions(dims);

    return true;
}

/**
 * @brief Parses photometric interpretation string to enum
 * @param piString Photometric interpretation string from DICOM
 * @return Corresponding enum value
 */
DicomViewer::EPhotometricInterpretation
CDicomLoader::parsePhotometricInterpretation(const char *piString)
{
    if (piString == nullptr)
    {
        return DicomViewer::EPhotometricInterpretation::Unknown;
    }

    std::string pi(piString);
    const auto start = pi.find_first_not_of(" \t\r\n");
    if (start == std::string::npos)
    {
        return DicomViewer::EPhotometricInterpretation::Unknown;
    }
    const auto end = pi.find_last_not_of(" \t\r\n");
    pi = pi.substr(start, end - start + 1);
    std::transform(pi.begin(), pi.end(), pi.begin(),
                   [](unsigned char c)
                   { return static_cast<char>(std::toupper(c)); });

    if (pi == "MONOCHROME1")
    {
        return DicomViewer::EPhotometricInterpretation::Monochrome1;
    }
    else if (pi == "MONOCHROME2")
    {
        return DicomViewer::EPhotometricInterpretation::Monochrome2;
    }
    else if (pi == "RGB")
    {
        return DicomViewer::EPhotometricInterpretation::Rgb;
    }
    else if (pi == "PALETTE COLOR")
    {
        return DicomViewer::EPhotometricInterpretation::PaletteColor;
    }

    return DicomViewer::EPhotometricInterpretation::Unknown;
}
