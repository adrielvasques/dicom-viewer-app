/**
 * @file CDicomMetadata.cpp
 * @brief Implementation of the CDicomMetadata class
 * @author DICOM Viewer Project
 * @date 2026
 *
 * Provides storage and access for DICOM metadata tags extracted from
 * medical image files. Organizes tags into patient, study, series,
 * and image information categories.
 */

#include "CDicomMetadata.h"

#include <utility>

namespace
{
    std::string getTagValue(const std::unordered_map<std::string, std::string> &tags,
                            const std::string &key)
    {
        auto it = tags.find(key);
        return it != tags.end() ? it->second : std::string();
    }
}

std::string CDicomMetadata::patientName() const
{
    return getTagValue(m_tags, "Patient Name");
}

std::string CDicomMetadata::patientId() const
{
    return getTagValue(m_tags, "Patient ID");
}

std::string CDicomMetadata::patientBirthDate() const
{
    return getTagValue(m_tags, "Patient Birth Date");
}

std::string CDicomMetadata::patientSex() const
{
    return getTagValue(m_tags, "Patient Sex");
}

std::string CDicomMetadata::studyDate() const
{
    return getTagValue(m_tags, "Study Date");
}

std::string CDicomMetadata::studyTime() const
{
    return getTagValue(m_tags, "Study Time");
}

std::string CDicomMetadata::studyDescription() const
{
    return getTagValue(m_tags, "Study Description");
}

std::string CDicomMetadata::accessionNumber() const
{
    return getTagValue(m_tags, "Accession Number");
}

std::string CDicomMetadata::seriesDescription() const
{
    return getTagValue(m_tags, "Series Description");
}

std::string CDicomMetadata::modality() const
{
    return getTagValue(m_tags, "Modality");
}

std::string CDicomMetadata::seriesNumber() const
{
    return getTagValue(m_tags, "Series Number");
}

std::string CDicomMetadata::instanceNumber() const
{
    return getTagValue(m_tags, "Instance Number");
}

std::string CDicomMetadata::imagePositionPatient() const
{
    return getTagValue(m_tags, "Image Position Patient");
}

std::string CDicomMetadata::sliceThickness() const
{
    return getTagValue(m_tags, "Slice Thickness");
}

std::string CDicomMetadata::rows() const
{
    return getTagValue(m_tags, "Rows");
}

std::string CDicomMetadata::columns() const
{
    return getTagValue(m_tags, "Columns");
}

std::string CDicomMetadata::bitsAllocated() const
{
    return getTagValue(m_tags, "Bits Allocated");
}

std::string CDicomMetadata::windowCenter() const
{
    return getTagValue(m_tags, "Window Center");
}

std::string CDicomMetadata::windowWidth() const
{
    return getTagValue(m_tags, "Window Width");
}

std::optional<std::string> CDicomMetadata::tag(const std::string &tagName) const
{
    auto it = m_tags.find(tagName);
    if (it == m_tags.end())
    {
        return std::nullopt;
    }
    return it->second;
}

std::unordered_map<std::string, std::string> CDicomMetadata::allTags() const
{
    return m_tags;
}

bool CDicomMetadata::isEmpty() const
{
    return m_tags.empty();
}

void CDicomMetadata::setPatientName(const std::string &name)
{
    setTag("Patient Name", name);
}

void CDicomMetadata::setPatientId(const std::string &id)
{
    setTag("Patient ID", id);
}

void CDicomMetadata::setPatientBirthDate(const std::string &date)
{
    setTag("Patient Birth Date", date);
}

void CDicomMetadata::setPatientSex(const std::string &sex)
{
    setTag("Patient Sex", sex);
}

void CDicomMetadata::setStudyDate(const std::string &date)
{
    setTag("Study Date", date);
}

void CDicomMetadata::setStudyTime(const std::string &time)
{
    setTag("Study Time", time);
}

void CDicomMetadata::setStudyDescription(const std::string &description)
{
    setTag("Study Description", description);
}

void CDicomMetadata::setAccessionNumber(const std::string &number)
{
    setTag("Accession Number", number);
}

void CDicomMetadata::setSeriesDescription(const std::string &description)
{
    setTag("Series Description", description);
}

void CDicomMetadata::setModality(const std::string &modality)
{
    setTag("Modality", modality);
}

void CDicomMetadata::setSeriesNumber(const std::string &number)
{
    setTag("Series Number", number);
}

void CDicomMetadata::setInstanceNumber(const std::string &number)
{
    setTag("Instance Number", number);
}

void CDicomMetadata::setImagePositionPatient(const std::string &position)
{
    setTag("Image Position Patient", position);
}

void CDicomMetadata::setSliceThickness(const std::string &thickness)
{
    setTag("Slice Thickness", thickness);
}

void CDicomMetadata::setRows(const std::string &rows)
{
    setTag("Rows", rows);
}

void CDicomMetadata::setColumns(const std::string &columns)
{
    setTag("Columns", columns);
}

void CDicomMetadata::setBitsAllocated(const std::string &bits)
{
    setTag("Bits Allocated", bits);
}

void CDicomMetadata::setWindowCenter(const std::string &center)
{
    setTag("Window Center", center);
}

void CDicomMetadata::setWindowWidth(const std::string &width)
{
    setTag("Window Width", width);
}

void CDicomMetadata::setTag(const std::string &tagName, const std::string &value)
{
    if (!value.empty())
    {
        m_tags[tagName] = value;
    }
}
