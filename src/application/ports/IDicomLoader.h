/**
 * @file IDicomLoader.h
 * @brief Interface for loading DICOM files (application port)
 * @date 2026
 */

#pragma once

#include "DicomViewer/Types.h"
#include "core/CDicomImage.h"

#include <memory>
#include <string>

struct SDicomLoadResult
{
    std::shared_ptr<CDicomImage> image;
    DicomViewer::ELoadResult result = DicomViewer::ELoadResult::Unknown;
    std::string errorMessage;
};

class IDicomLoader
{
  public:
    virtual ~IDicomLoader() = default;
    virtual SDicomLoadResult load(const std::string &filePath) = 0;
};
