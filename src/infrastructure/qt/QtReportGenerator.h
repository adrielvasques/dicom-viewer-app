/**
 * @file QtReportGenerator.h
 * @brief Qt-based PDF report generator
 * @date 2026
 */

#pragma once

#include "application/ports/IReportGenerator.h"

class QtReportGenerator final : public IReportGenerator
{
  public:
    bool generate(const SReportData &data, std::string &errorMessage) const override;
};
