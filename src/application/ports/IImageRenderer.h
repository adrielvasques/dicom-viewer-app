/**
 * @file IImageRenderer.h
 * @brief Interface for rendering DICOM images into QImage
 * @date 2026
 */

#pragma once

#include "DicomViewer/Types.h"
#include "application/dto/ImageBuffer.h"
#include "core/CDicomImage.h"

#include <optional>

class IImageRenderer
{
  public:
    virtual ~IImageRenderer() = default;

    virtual SImageBuffer render(const CDicomImage &image,
                                DicomViewer::EPaletteType palette,
                                const std::optional<DicomViewer::SWindowLevel> &windowLevel) const = 0;
};
