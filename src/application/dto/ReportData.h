/**
 * @file ReportData.h
 * @brief Data required to generate a DICOM report
 * @date 2026
 */

#pragma once

#include "application/dto/ImageBuffer.h"
#include "core/CDicomImage.h"
#include "DicomViewer/Types.h"

#include <memory>
#include <optional>
#include <string>

struct SReportData
{
    std::string filePath;
    SImageBuffer image;
    std::shared_ptr<CDicomImage> dicomImage;
    std::string comment;
    DicomViewer::EPaletteType palette = DicomViewer::EPaletteType::Grayscale;
    std::optional<DicomViewer::SWindowLevel> windowLevel;
};
