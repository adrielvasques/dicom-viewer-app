/**
 * @file CMainWindow.h
 * @brief Main application window class declaration
 * @author DICOM Viewer Project
 * @date 2026
 *
 * Defines the CMainWindow class which serves as the main application
 * window, coordinating the image viewer, metadata panel, and file loading.
 */

#pragma once

#include "CImageViewer.h"
#include "CMetadataPanel.h"
#include "CThumbnailWidget.h"
#include "core/CDicomImage.h"
#include "presentation/viewmodels/MainViewModel.h"

#include <QMainWindow>
#include <QString>
#include <QStringList>
#include <memory>

class QLabel;
class QDockWidget;
class QActionGroup;
class QStackedWidget;

/**
 * @class CMainWindow
 * @brief Main application window for the DICOM viewer
 *
 * Provides the main user interface including:
 * - Menu bar with File and Help menus
 * - Toolbar with common actions
 * - Central image viewer widget
 * - Dockable metadata panel
 * - Status bar with window/level display
 */
class CMainWindow : public QMainWindow
{
    Q_OBJECT

  public:
    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit CMainWindow(std::shared_ptr<MainViewModel> viewModel,
                         QWidget *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~CMainWindow() override = default;

    void setUiChromeVisible(bool visible);

  signals:
    /**
     * @brief Emitted when an image is successfully loaded
     * @param filePath Path to the loaded file
     */
    void imageLoaded(const QString &filePath);

    /**
     * @brief Emitted when an error occurs
     * @param message Error message
     */
    void errorOccurred(const QString &message);

  private slots:
    /** @name Action Handlers */
    ///@{
    /**
     * @brief Handles Open File action
     */
    void onOpenFileClicked();

    /**
     * @brief Handles Reset Window/Level action
     */
    void onResetWindowLevelClicked();

    /**
     * @brief Handles window/level changes from image viewer
     * @param center New window center
     * @param width New window width
     */
    void onWindowLevelChanged(double center, double width);

    /**
     * @brief Handles About action
     */
    void onAboutClicked();

    /**
     * @brief Handles palette selection from menu
     */
    void onPaletteSelected();

    /**
     * @brief Handles palette change notification
     * @param type New palette type
     */
    void onPaletteChanged(DicomViewer::EPaletteType type);
    void onViewStateChanged(double zoom, double panX, double panY, int rotation);
    /**
     * @brief Handles files dropped on the viewer
     * @param filePaths Dropped file paths
     */
    void onFilesDropped(const QStringList &filePaths);
    /**
     * @brief Handles thumbnail selection
     * @param index Selected thumbnail index
     */
    void onThumbnailSelected(int index);
    /**
     * @brief Handles thumbnail delete requests
     * @param index Thumbnail index to remove
     */
    void onThumbnailDeleteRequested(int index);

    /** @name Export Handlers */
    ///@{
    void onExportPng();
    void onExportJpeg();
    void onExportPdf();
    void onGenerateReport();
    ///@}

    /** @name View Action Handlers */
    ///@{
    void onZoomIn();
    void onZoomOut();
    void onFitToWindow();
    void onActualSize();
    void onRotateLeft();
    void onRotateRight();
    void onResetView();
    ///@}

  private:
    /** @name UI Setup Methods */
    ///@{
    void setupUi();
    void setupMenuBar();
    void setupStatusBar();
    void setupDockWidgets();
    void setupConnections();
    void setupPaletteMenu();
    ///@}

    /** @name Helper Methods */
    ///@{
    /**
     * @brief Displays an error message to the user
     * @param message Error message to display
     */
    void showError(const QString &message);

    /**
     * @brief Updates the window/level display in status bar
     * @param center Window center value
     * @param width Window width value
     */
    void updateWindowLevelDisplay(double center, double width);

    /**
     * @brief Updates the image type display in status bar
     * @param image Pointer to the current image
     */
    void updateImageTypeDisplay(const CDicomImage *image);
    void applyPaletteState(DicomViewer::EPaletteType type);

    /**
     * @brief Prompts user for optional report comments
     * @param comment Output comment text (empty if not provided)
     * @return True if the dialog was accepted
     */
    bool requestReportComment(QString &comment);
    ///@}

    void applyCurrentImage();
    void onImageAdded(int index);
    void onImageRemoved(int index);

    CImageViewer *m_imageViewer = nullptr;
    CMetadataPanel *m_metadataPanel = nullptr;
    CThumbnailWidget *m_thumbnailWidget = nullptr;
    QDockWidget *m_sidePanelDock = nullptr;
    QStackedWidget *m_sidePanelStack = nullptr;
    QLabel *m_windowLevelLabel = nullptr;
    QLabel *m_imageTypeLabel = nullptr;
    QLabel *m_imageSizeLabel = nullptr;
    QLabel *m_paletteLabel = nullptr;
    QMenu *m_paletteMenu = nullptr;
    QActionGroup *m_paletteActionGroup = nullptr;

    std::shared_ptr<MainViewModel> m_viewModel;

    QString m_lastOpenDirectory; /**< Last used directory for file dialog */
};
