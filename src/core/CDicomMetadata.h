/**
 * @file CDicomMetadata.h
 * @brief DICOM metadata container class declaration
 * @author DICOM Viewer Project
 * @date 2026
 *
 * Defines the CDicomMetadata class which stores and provides access to
 * DICOM header tags. Tags are organized into patient, study, series,
 * and image categories.
 */

#pragma once

#include <optional>
#include <string>
#include <unordered_map>

class CDicomLoader;

/**
 * @class CDicomMetadata
 * @brief Container for DICOM metadata tags
 *
 * Stores DICOM header information extracted from medical image files.
 * Provides typed accessors for common tags and generic access for any tag.
 * Setters are private and accessible only via friend class CDicomLoader.
 */
class CDicomMetadata
{
    friend class CDicomLoader;

  public:
    /**
     * @brief Default constructor
     */
    CDicomMetadata() = default;

    /** @name Patient Information Getters */
    ///@{
    std::string patientName() const;
    std::string patientId() const;
    std::string patientBirthDate() const;
    std::string patientSex() const;
    ///@}

    /** @name Study Information Getters */
    ///@{
    std::string studyDate() const;
    std::string studyTime() const;
    std::string studyDescription() const;
    std::string accessionNumber() const;
    ///@}

    /** @name Series Information Getters */
    ///@{
    std::string seriesDescription() const;
    std::string modality() const;
    std::string seriesNumber() const;
    ///@}

    /** @name Image Information Getters */
    ///@{
    std::string instanceNumber() const;
    std::string imagePositionPatient() const;
    std::string sliceThickness() const;
    std::string rows() const;
    std::string columns() const;
    std::string bitsAllocated() const;
    std::string windowCenter() const;
    std::string windowWidth() const;
    ///@}

    /** @name Generic Tag Access */
    ///@{
    /**
     * @brief Retrieves a tag by name
     * @param tagName Name of the tag to retrieve
     * @return Optional containing value if found
     */
    std::optional<std::string> tag(const std::string &tagName) const;

    /**
     * @brief Retrieves all stored tags
     * @return Map of tag names to values
     */
    std::unordered_map<std::string, std::string> allTags() const;

    /**
     * @brief Checks if metadata is empty
     * @return True if no tags stored
     */
    bool isEmpty() const;
    ///@}

  private:
    /** @name Patient Information Setters */
    ///@{
    void setPatientName(const std::string &name);
    void setPatientId(const std::string &id);
    void setPatientBirthDate(const std::string &date);
    void setPatientSex(const std::string &sex);
    ///@}

    /** @name Study Information Setters */
    ///@{
    void setStudyDate(const std::string &date);
    void setStudyTime(const std::string &time);
    void setStudyDescription(const std::string &description);
    void setAccessionNumber(const std::string &number);
    ///@}

    /** @name Series Information Setters */
    ///@{
    void setSeriesDescription(const std::string &description);
    void setModality(const std::string &modality);
    void setSeriesNumber(const std::string &number);
    ///@}

    /** @name Image Information Setters */
    ///@{
    void setInstanceNumber(const std::string &number);
    void setImagePositionPatient(const std::string &position);
    void setSliceThickness(const std::string &thickness);
    void setRows(const std::string &rows);
    void setColumns(const std::string &columns);
    void setBitsAllocated(const std::string &bits);
    void setWindowCenter(const std::string &center);
    void setWindowWidth(const std::string &width);
    ///@}

    /** @name Generic Tag Manipulation */
    ///@{
    void setTag(const std::string &tagName, const std::string &value);
    void clear();
    ///@}

    std::unordered_map<std::string, std::string> m_tags; /**< Storage for all metadata tags */
};
