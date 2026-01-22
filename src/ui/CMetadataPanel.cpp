/**
 * @file CMetadataPanel.cpp
 * @brief Implementation of the CMetadataPanel class
 * @author DICOM Viewer Project
 * @date 2026
 *
 * Implements the DICOM metadata display panel using a QTableWidget
 * to show tag names and values in a two-column format.
 */

#include "CMetadataPanel.h"

#include <QHeaderView>
#include <QResizeEvent>
#include <QTableWidget>
#include <QVBoxLayout>
#include <algorithm>
#include <cmath>

/**
 * @brief Constructor
 * @param parent Parent widget
 */
CMetadataPanel::CMetadataPanel(QWidget *parent)
    : QWidget(parent)
{
    setupUi();
}

/**
 * @brief Sets metadata to display
 * @param metadata Pointer to metadata object (can be nullptr)
 */
void CMetadataPanel::setMetadata(const CDicomMetadata *metadata)
{
    populateTable(metadata);
}

/**
 * @brief Clears displayed metadata
 */
void CMetadataPanel::clearMetadata()
{
    m_tableWidget->setRowCount(0);
}

void CMetadataPanel::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    if (!m_tableWidget || m_updatingColumns)
    {
        return;
    }

    const int viewWidth = m_tableWidget->viewport()->width();
    if (viewWidth <= 0)
    {
        return;
    }

    const int minWidth = m_tableWidget->horizontalHeader()->minimumSectionSize();
    const double ratioMin = static_cast<double>(minWidth) / viewWidth;
    const double ratioMax = 1.0 - ratioMin;
    const double ratio = std::clamp(m_columnRatio, ratioMin, ratioMax);

    m_updatingColumns = true;
    const int targetWidth = static_cast<int>(std::round(viewWidth * ratio));
    m_tableWidget->setColumnWidth(0, targetWidth);
    m_updatingColumns = false;
}

void CMetadataPanel::updateColumnRatio(int logicalIndex, int oldSize, int newSize)
{
    Q_UNUSED(oldSize);
    if (m_updatingColumns || logicalIndex != 0 || !m_tableWidget)
    {
        return;
    }

    const int viewWidth = m_tableWidget->viewport()->width();
    if (viewWidth <= 0)
    {
        return;
    }

    const int minWidth = m_tableWidget->horizontalHeader()->minimumSectionSize();
    const double ratioMin = static_cast<double>(minWidth) / viewWidth;
    const double ratioMax = 1.0 - ratioMin;
    m_columnRatio = std::clamp(static_cast<double>(newSize) / viewWidth, ratioMin, ratioMax);
}

/**
 * @brief Sets up the UI components
 */
void CMetadataPanel::setupUi()
{
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);

    m_tableWidget = new QTableWidget(this);
    m_tableWidget->setObjectName("MetadataTable");
    m_tableWidget->setColumnCount(2);
    m_tableWidget->setHorizontalHeaderLabels({tr("Tag"), tr("Value")});
    m_tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableWidget->setAlternatingRowColors(true);
    m_tableWidget->verticalHeader()->setVisible(false);
    m_tableWidget->setShowGrid(true);

    // Allow user to resize Tag column only; Value stays inside the table
    auto *header = m_tableWidget->horizontalHeader();
    header->setSectionsMovable(false);
    header->setStretchLastSection(true);
    header->setSectionResizeMode(0, QHeaderView::Interactive);
    header->setSectionResizeMode(1, QHeaderView::Stretch);
    header->setMinimumSectionSize(80);
    m_tableWidget->setSizeAdjustPolicy(QAbstractScrollArea::AdjustIgnored);
    connect(header, &QHeaderView::sectionResized,
            this, &CMetadataPanel::updateColumnRatio);

    m_layout->addWidget(m_tableWidget);
}

/**
 * @brief Populates the table with metadata
 * @param metadata Metadata to display
 */
void CMetadataPanel::populateTable(const CDicomMetadata *metadata)
{
    m_tableWidget->setRowCount(0);

    if (metadata == nullptr || metadata->isEmpty())
    {
        return;
    }

    const auto tags = metadata->allTags();
    m_tableWidget->setRowCount(static_cast<int>(tags.size()));

    int row = 0;
    for (const auto &entry : tags)
    {
        auto *tagItem = new QTableWidgetItem(QString::fromStdString(entry.first));
        auto *valueItem = new QTableWidgetItem(QString::fromStdString(entry.second));

        m_tableWidget->setItem(row, 0, tagItem);
        m_tableWidget->setItem(row, 1, valueItem);
        row++;
    }

    m_tableWidget->resizeColumnsToContents();
}
