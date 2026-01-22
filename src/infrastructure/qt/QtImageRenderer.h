/**
 * @file QtImageRenderer.h
 * @brief Qt-based DICOM image renderer
 * @date 2026
 */

#pragma once

#include "application/ports/IImageRenderer.h"
#include "utils/CImageConverter.h"

class QtImageRenderer final : public IImageRenderer
{
  public:
    SImageBuffer render(const CDicomImage &image,
                        DicomViewer::EPaletteType palette,
                        const std::optional<DicomViewer::SWindowLevel> &windowLevel) const override;

  private:
    mutable CImageConverter m_converter;
};
