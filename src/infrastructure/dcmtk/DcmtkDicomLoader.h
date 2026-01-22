/**
 * @file DcmtkDicomLoader.h
 * @brief DCMTK-backed DICOM loader (infrastructure adapter)
 * @date 2026
 */

#pragma once

#include "application/ports/IDicomLoader.h"
#include "core/CDicomLoader.h"

class DcmtkDicomLoader final : public IDicomLoader
{
  public:
    SDicomLoadResult load(const std::string &filePath) override;

  private:
    CDicomLoader m_loader;
};
