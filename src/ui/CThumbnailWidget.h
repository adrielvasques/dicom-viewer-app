/**
 * @file CThumbnailWidget.h
 * @brief Thumbnail list widget for loaded DICOM images
 * @author DICOM Viewer Project
 * @date 2026
 */

#pragma once

#include <QImage>
#include <QSize>
#include <QString>
#include <QWidget>
#include <memory>
#include <vector>

#include "utils/CColorPalette.h"

class CDicomImage;
class QListWidget;
class QLabel;
class QListWidgetItem;
class QResizeEvent;

/**
 * @class CThumbnailWidget
 * @brief Displays thumbnails for all loaded DICOM images
 *
 * Shows a scrollable list of scaled thumbnails so the user can
 * browse and select a specific image.
 */
class CThumbnailWidget : public QWidget
{
    Q_OBJECT

  public:
    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit CThumbnailWidget(QWidget *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~CThumbnailWidget() override = default;

    /**
     * @brief Adds a thumbnail entry for a loaded DICOM image
     * @param label Display label (typically file name)
     * @param tooltip Tooltip text (typically full path)
     * @param image Shared pointer to DICOM image
     */
    void addImage(const QString &label,
                  const QString &tooltip,
                  std::shared_ptr<CDicomImage> image);

    /**
     * @brief Clears all thumbnails
     */
    void clearImages();

    /**
     * @brief Selects a thumbnail by index
     * @param index Index to select
     */
    void setCurrentIndex(int index);

    /**
     * @brief Gets the currently selected index
     * @return Index or -1 if none selected
     */
    int currentIndex() const;

    /**
     * @brief Removes a thumbnail by index
     * @param index Index to remove
     */
    void removeImage(int index);

    /**
     * @brief Updates a thumbnail image using current window/level + palette
     * @param index Thumbnail index
     * @param palette Palette type to apply
     */
    void updateThumbnail(int index, DicomViewer::EPaletteType palette);

    /**
     * @brief Updates all thumbnails with the given palette
     * @param palette Palette type to apply
     */
    void updateAllThumbnails(DicomViewer::EPaletteType palette);

    /**
     * @brief Sets a thumbnail image explicitly
     * @param index Thumbnail index
     * @param thumbnail Image to display
     */
    void setThumbnailImage(int index, const QImage &thumbnail);

    /**
     * @brief Gets the configured thumbnail size
     * @return Thumbnail size
     */
    QSize thumbnailSize() const;

  signals:
    /**
     * @brief Emitted when a thumbnail is selected
     * @param index Selected index
     */
    void imageSelected(int index);
    /**
     * @brief Emitted when delete is requested for a thumbnail
     * @param index Index to delete
     */
    void imageDeleteRequested(int index);

  protected:
    void resizeEvent(QResizeEvent *event) override;

  private:
    QImage buildThumbnail(const CDicomImage &image, DicomViewer::EPaletteType palette) const;
    QWidget *buildItemWidget(const QString &label,
                             const QString &details,
                             const QString &tooltip,
                             const QImage &thumbnail,
                             QListWidgetItem *item);
    QSize cardSize() const;
    void updateLayoutSizes();
    void updateSelectionState();
    void updateEmptyState();

    QListWidget *m_listWidget = nullptr;
    QLabel *m_emptyLabel = nullptr;
    QSize m_thumbnailSize{150, 150};
    std::vector<std::shared_ptr<CDicomImage>> m_images;
};
