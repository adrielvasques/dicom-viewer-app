/**
 * @file IReportGenerator.h
 * @brief Interface for PDF report generation
 * @date 2026
 */

#pragma once

#include "application/dto/ReportData.h"

#include <string>

class IReportGenerator
{
  public:
    virtual ~IReportGenerator() = default;
    virtual bool generate(const SReportData &data, std::string &errorMessage) const = 0;
};
