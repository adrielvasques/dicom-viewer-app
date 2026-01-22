/**
 * @file CMetadataPanel.h
 * @brief DICOM metadata display panel class declaration
 * @author DICOM Viewer Project
 * @date 2026
 *
 * Defines the CMetadataPanel widget which displays DICOM metadata
 * in a tabular format using QTableWidget.
 */

#pragma once

#include "core/CDicomMetadata.h"

#include <QWidget>

class QTableWidget;
class QVBoxLayout;
class QResizeEvent;

/**
 * @class CMetadataPanel
 * @brief Widget for displaying DICOM metadata
 *
 * Displays DICOM header tags in a two-column table format with
 * tag names and values. Provides patient, study, series, and
 * image information extracted from DICOM files.
 */
class CMetadataPanel : public QWidget
{
    Q_OBJECT

  public:
    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit CMetadataPanel(QWidget *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~CMetadataPanel() override = default;

    /** @name Public Methods */
    ///@{
    /**
     * @brief Sets metadata to display
     * @param metadata Pointer to metadata object (can be nullptr)
     */
    void setMetadata(const CDicomMetadata *metadata);

    /**
     * @brief Clears displayed metadata
     */
    void clearMetadata();
    ///@}

  protected:
    void resizeEvent(QResizeEvent *event) override;

  private:
    /** @name Internal Methods */
    ///@{
    /**
     * @brief Sets up the UI components
     */
    void setupUi();

    /**
     * @brief Populates the table with metadata
     * @param metadata Metadata to display
     */
    void populateTable(const CDicomMetadata *metadata);
    void updateColumnRatio(int logicalIndex, int oldSize, int newSize);
    ///@}

    QTableWidget *m_tableWidget = nullptr; /**< Table for displaying tags */
    QVBoxLayout *m_layout = nullptr;       /**< Main layout */
    double m_columnRatio = 0.5;
    bool m_updatingColumns = false;
};
