/**
 * @file CMainWindow.cpp
 * @brief Implementation of the CMainWindow class
 * @author DICOM Viewer Project
 * @date 2026
 *
 * Implements the main application window providing the user interface
 * for loading and viewing DICOM images.
 */

#include "CMainWindow.h"
#include "utils/CColorPalette.h"

#include <QAction>
#include <QActionGroup>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QDockWidget>
#include <QFileDialog>
#include <QFileInfo>
#include <QIcon>
#include <QLabel>
#include <QMenuBar>
#include <QMessageBox>
#include <QPainter>
#include <QSignalBlocker>
#include <QSize>
#include <QStackedWidget>
#include <QStatusBar>
#include <QStyle>
#include <QSvgRenderer>
#include <QTextEdit>
#include <QToolBar>
#include <QVBoxLayout>
#include <algorithm>

/**
 * @brief Constructor
 * @param parent Parent widget
 */
CMainWindow::CMainWindow(std::shared_ptr<MainViewModel> viewModel,
                         QWidget *parent)
    : QMainWindow(parent),
      m_viewModel(std::move(viewModel))
{
    setObjectName("MainWindow");
    setWindowIcon(QIcon(":/images/app-icon.png"));

    setupUi();
    setupMenuBar();
    setupPaletteMenu();
    setupStatusBar();
    setupDockWidgets();
    setupConnections();
}

/**
 * @brief Sets up the main UI components
 */
void CMainWindow::setupUi()
{
    // Create central image viewer
    m_imageViewer = new CImageViewer(this);
    m_imageViewer->setObjectName("ImageViewer");
    setCentralWidget(m_imageViewer);

    // Create metadata panel
    m_metadataPanel = new CMetadataPanel(this);
    m_metadataPanel->setObjectName("MetadataPanel");
}

/**
 * @brief Sets up the menu bar
 */
void CMainWindow::setupMenuBar()
{
    menuBar()->setObjectName("MainMenuBar");

    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));

    QAction *openAction = fileMenu->addAction(tr("&Open..."));
    openAction->setShortcut(QKeySequence::Open);
    openAction->setStatusTip(tr("Open a DICOM file"));
    connect(openAction, &QAction::triggered, this, &CMainWindow::onOpenFileClicked);

    fileMenu->addSeparator();

    // Export Image submenu
    QMenu *exportMenu = fileMenu->addMenu(tr("&Export Image"));

    QAction *exportPngAction = exportMenu->addAction(tr("As &PNG..."));
    exportPngAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_P));
    exportPngAction->setStatusTip(tr("Export image as PNG"));
    connect(exportPngAction, &QAction::triggered, this, &CMainWindow::onExportPng);

    QAction *exportJpegAction = exportMenu->addAction(tr("As &JPEG..."));
    exportJpegAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_J));
    exportJpegAction->setStatusTip(tr("Export image as JPEG"));
    connect(exportJpegAction, &QAction::triggered, this, &CMainWindow::onExportJpeg);

    QAction *exportPdfAction = exportMenu->addAction(tr("As P&DF..."));
    exportPdfAction->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_D));
    exportPdfAction->setStatusTip(tr("Export image as PDF"));
    connect(exportPdfAction, &QAction::triggered, this, &CMainWindow::onExportPdf);

    QAction *reportAction = fileMenu->addAction(tr("&Generate Report..."));
    reportAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R));
    reportAction->setStatusTip(tr("Generate a PDF report with image and metadata"));
    connect(reportAction, &QAction::triggered, this, &CMainWindow::onGenerateReport);

    fileMenu->addSeparator();

    QAction *exitAction = fileMenu->addAction(tr("E&xit"));
    exitAction->setShortcut(QKeySequence::Quit);
    exitAction->setStatusTip(tr("Exit the application"));
    connect(exitAction, &QAction::triggered, this, &QMainWindow::close);

    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));

    // Zoom actions
    QAction *zoomInAction = viewMenu->addAction(tr("Zoom &In"));
    zoomInAction->setShortcut(QKeySequence(Qt::Key_Plus));
    zoomInAction->setStatusTip(tr("Zoom in on the image"));
    connect(zoomInAction, &QAction::triggered, this, &CMainWindow::onZoomIn);

    QAction *zoomOutAction = viewMenu->addAction(tr("Zoom &Out"));
    zoomOutAction->setShortcut(QKeySequence(Qt::Key_Minus));
    zoomOutAction->setStatusTip(tr("Zoom out on the image"));
    connect(zoomOutAction, &QAction::triggered, this, &CMainWindow::onZoomOut);

    QAction *fitAction = viewMenu->addAction(tr("&Fit to Window"));
    fitAction->setShortcut(QKeySequence(Qt::Key_F));
    fitAction->setStatusTip(tr("Fit image to window size"));
    connect(fitAction, &QAction::triggered, this, &CMainWindow::onFitToWindow);

    QAction *actualSizeAction = viewMenu->addAction(tr("&Actual Size"));
    actualSizeAction->setShortcut(QKeySequence(Qt::Key_1));
    actualSizeAction->setStatusTip(tr("Show image at actual size (100%)"));
    connect(actualSizeAction, &QAction::triggered, this, &CMainWindow::onActualSize);

    viewMenu->addSeparator();

    // Rotation actions
    QAction *rotateLeftAction = viewMenu->addAction(tr("Rotate &Left"));
    rotateLeftAction->setShortcut(QKeySequence(Qt::Key_BracketLeft));
    rotateLeftAction->setStatusTip(tr("Rotate image 90° counter-clockwise"));
    connect(rotateLeftAction, &QAction::triggered, this, &CMainWindow::onRotateLeft);

    QAction *rotateRightAction = viewMenu->addAction(tr("Rotate &Right"));
    rotateRightAction->setShortcut(QKeySequence(Qt::Key_BracketRight));
    rotateRightAction->setStatusTip(tr("Rotate image 90° clockwise"));
    connect(rotateRightAction, &QAction::triggered, this, &CMainWindow::onRotateRight);

    viewMenu->addSeparator();

    // Reset actions
    QAction *resetViewAction = viewMenu->addAction(tr("Reset &View"));
    resetViewAction->setShortcut(QKeySequence(Qt::Key_0));
    resetViewAction->setStatusTip(tr("Reset zoom, pan and rotation"));
    connect(resetViewAction, &QAction::triggered, this, &CMainWindow::onResetView);

    QAction *resetWLAction = viewMenu->addAction(tr("Reset &Window/Level"));
    resetWLAction->setShortcut(QKeySequence(Qt::SHIFT | Qt::Key_R));
    resetWLAction->setStatusTip(tr("Reset window/level to default values"));
    connect(resetWLAction, &QAction::triggered, this, &CMainWindow::onResetWindowLevelClicked);

    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));

    QAction *aboutAction = helpMenu->addAction(tr("&About"));
    aboutAction->setStatusTip(tr("About this application"));
    connect(aboutAction, &QAction::triggered, this, &CMainWindow::onAboutClicked);
}

/**
 * @brief Sets up the palette submenu in View menu
 */
void CMainWindow::setupPaletteMenu()
{
    // Find the View menu
    QMenu *viewMenu = nullptr;
    for (QAction *action : menuBar()->actions())
    {
        if (action->menu() && action->text().contains("View", Qt::CaseInsensitive))
        {
            viewMenu = action->menu();
            break;
        }
    }

    if (!viewMenu)
    {
        return;
    }

    viewMenu->addSeparator();

    // Create palette submenu
    m_paletteMenu = viewMenu->addMenu(tr("Color &Palette"));
    m_paletteActionGroup = new QActionGroup(this);
    m_paletteActionGroup->setExclusive(true);

    // Add palette options
    for (DicomViewer::EPaletteType type : CColorPalette::availablePalettes())
    {
        QAction *action = m_paletteMenu->addAction(CColorPalette::paletteName(type));
        action->setCheckable(true);
        action->setData(static_cast<int>(type));
        m_paletteActionGroup->addAction(action);

        // Set default selection
        if (type == DicomViewer::EPaletteType::Grayscale)
        {
            action->setChecked(true);
        }

        connect(action, &QAction::triggered, this, &CMainWindow::onPaletteSelected);
    }
}

/**
 * @brief Sets up the status bar
 */
void CMainWindow::setupStatusBar()
{
    statusBar()->setObjectName("MainStatusBar");

    // Image type indicator (Grayscale/RGB)
    m_imageTypeLabel = new QLabel(this);
    m_imageTypeLabel->setMinimumWidth(120);
    m_imageTypeLabel->setAlignment(Qt::AlignCenter);
    m_imageTypeLabel->setStyleSheet("QLabel { padding: 2px 8px; }");
    statusBar()->addPermanentWidget(m_imageTypeLabel);

    // Image size indicator
    m_imageSizeLabel = new QLabel(this);
    m_imageSizeLabel->setMinimumWidth(100);
    statusBar()->addPermanentWidget(m_imageSizeLabel);

    // Palette indicator
    m_paletteLabel = new QLabel(this);
    m_paletteLabel->setMinimumWidth(100);
    statusBar()->addPermanentWidget(m_paletteLabel);
    m_paletteLabel->setText(tr("Palette: Grayscale"));

    // Window/Level indicator
    m_windowLevelLabel = new QLabel(this);
    m_windowLevelLabel->setMinimumWidth(150);
    statusBar()->addPermanentWidget(m_windowLevelLabel);

    updateWindowLevelDisplay(0, 0);
    updateImageTypeDisplay(nullptr);
}

/**
 * @brief Sets up the dock widgets
 */
void CMainWindow::setupDockWidgets()
{
    // Helper to load SVG icon with custom color
    auto loadSvgIcon = [](const QString &path, const QColor &color, int size = 22)
    {
        QPixmap pix(size, size);
        pix.fill(Qt::transparent);
        QPainter painter(&pix);
        painter.setRenderHint(QPainter::Antialiasing, true);

        QSvgRenderer svg(path);
        if (svg.isValid())
        {
            QPixmap mask(size, size);
            mask.fill(Qt::transparent);
            QPainter maskPainter(&mask);
            svg.render(&maskPainter);
            maskPainter.end();

            painter.fillRect(pix.rect(), color);
            painter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
            painter.drawPixmap(0, 0, mask);
        }
        return QIcon(pix);
    };

    const QColor iconColor("#475569"); // Slate gray

    m_thumbnailWidget = new CThumbnailWidget(this);
    m_sidePanelStack = new QStackedWidget(this);
    m_sidePanelStack->setObjectName("SidePanelStack");
    m_sidePanelStack->addWidget(m_thumbnailWidget);
    m_sidePanelStack->addWidget(m_metadataPanel);

    m_sidePanelDock = new QDockWidget(tr("Thumbnails"), this);
    m_sidePanelDock->setObjectName("SidePanelDock");
    m_sidePanelDock->setWidget(m_sidePanelStack);
    m_sidePanelDock->setAllowedAreas(Qt::LeftDockWidgetArea);
    m_sidePanelDock->setFeatures(QDockWidget::DockWidgetMovable);
    addDockWidget(Qt::LeftDockWidgetArea, m_sidePanelDock);

    auto *sideBar = new QToolBar(tr("Navigation"), this);
    sideBar->setObjectName("SideBar");
    sideBar->setMovable(false);
    sideBar->setFloatable(false);
    sideBar->setOrientation(Qt::Vertical);
    sideBar->setToolButtonStyle(Qt::ToolButtonIconOnly);
    sideBar->setIconSize(QSize(20, 20));

    auto *sideGroup = new QActionGroup(this);
    sideGroup->setExclusive(true);

    QAction *showThumbnails = new QAction(this);
    showThumbnails->setCheckable(true);
    showThumbnails->setChecked(true);
    showThumbnails->setText(tr("Thumbnails"));
    showThumbnails->setIcon(loadSvgIcon(":/icons/image.svg", iconColor));
    showThumbnails->setToolTip(tr("Show thumbnails"));
    sideGroup->addAction(showThumbnails);
    sideBar->addAction(showThumbnails);
    connect(showThumbnails, &QAction::triggered, this, [this]()
            {
        if (m_sidePanelStack) {
            m_sidePanelStack->setCurrentIndex(0);
        }
        if (m_sidePanelDock) {
            m_sidePanelDock->setWindowTitle(tr("Thumbnails"));
            m_sidePanelDock->show();
            m_sidePanelDock->raise();
        } });

    QAction *showMetadata = new QAction(this);
    showMetadata->setCheckable(true);
    showMetadata->setText(tr("Metadata"));
    showMetadata->setIcon(loadSvgIcon(":/icons/info-circle.svg", iconColor));
    showMetadata->setToolTip(tr("Show DICOM metadata"));
    sideGroup->addAction(showMetadata);
    sideBar->addAction(showMetadata);
    connect(showMetadata, &QAction::triggered, this, [this]()
            {
        if (m_sidePanelStack) {
            m_sidePanelStack->setCurrentIndex(1);
        }
        if (m_sidePanelDock) {
            m_sidePanelDock->setWindowTitle(tr("DICOM Metadata"));
            m_sidePanelDock->show();
            m_sidePanelDock->raise();
        } });

    sideBar->addSeparator();

    // Open file action
    QAction *openAction = new QAction(this);
    openAction->setText(tr("Open"));
    openAction->setIcon(loadSvgIcon(":/icons/folder-open.svg", iconColor));
    openAction->setToolTip(tr("Open DICOM file"));
    connect(openAction, &QAction::triggered, this, &CMainWindow::onOpenFileClicked);
    sideBar->addAction(openAction);

    addToolBar(Qt::LeftToolBarArea, sideBar);
}

/**
 * @brief Sets up signal/slot connections
 */
void CMainWindow::setupConnections()
{
    connect(m_imageViewer, &CImageViewer::windowLevelChanged,
            this, &CMainWindow::onWindowLevelChanged);
    connect(m_imageViewer, &CImageViewer::paletteChanged,
            this, &CMainWindow::onPaletteChanged);
    connect(m_imageViewer, &CImageViewer::viewStateChanged,
            this, &CMainWindow::onViewStateChanged);
    connect(m_imageViewer, &CImageViewer::filesDropped,
            this, &CMainWindow::onFilesDropped);
    if (m_thumbnailWidget)
    {
        connect(m_thumbnailWidget, &CThumbnailWidget::imageSelected,
                this, &CMainWindow::onThumbnailSelected);
        connect(m_thumbnailWidget, &CThumbnailWidget::imageDeleteRequested,
                this, &CMainWindow::onThumbnailDeleteRequested);
    }

    if (m_viewModel)
    {
        connect(m_viewModel.get(), &MainViewModel::errorOccurred,
                this, &CMainWindow::showError);
        connect(m_viewModel.get(), &MainViewModel::statusMessage,
                this, [this](const QString &message, int timeout)
                {
                    if (statusBar())
                    {
                        statusBar()->showMessage(message, timeout);
                    } });
        connect(m_viewModel.get(), &MainViewModel::imageAdded,
                this, &CMainWindow::onImageAdded);
        connect(m_viewModel.get(), &MainViewModel::imageRemoved,
                this, &CMainWindow::onImageRemoved);
        connect(m_viewModel.get(), &MainViewModel::currentImageChanged,
                this, &CMainWindow::applyCurrentImage);
        connect(m_viewModel.get(), &MainViewModel::paletteUpdated,
                this, &CMainWindow::applyPaletteState);
    }
}

void CMainWindow::setUiChromeVisible(bool visible)
{
    if (menuBar())
    {
        menuBar()->setVisible(visible);
    }
    for (QToolBar *toolBar : findChildren<QToolBar *>())
    {
        toolBar->setVisible(visible);
    }
    if (statusBar())
    {
        statusBar()->setVisible(visible);
    }
    for (QDockWidget *dock : findChildren<QDockWidget *>())
    {
        dock->setVisible(visible);
    }
}

/**
 * @brief Handles Open File action
 */
void CMainWindow::onOpenFileClicked()
{
    QString startDir = m_lastOpenDirectory.isEmpty()
                           ? QDir::homePath()
                           : m_lastOpenDirectory;

    QStringList filePaths = QFileDialog::getOpenFileNames(
        this,
        tr("Open DICOM File"),
        startDir,
        tr("DICOM Files (*.dcm *.DCM);;All Files (*)"));

    if (filePaths.isEmpty())
    {
        return;
    }

    // Remember the directory for next time
    m_lastOpenDirectory = QFileInfo(filePaths.first()).absolutePath();

    if (m_viewModel)
    {
        const auto state = m_imageViewer->viewState();
        MainViewModel::SViewState viewState{state.zoom, state.pan, state.rotation};
        m_viewModel->loadFiles(filePaths, viewState, m_imageViewer->windowLevel());
    }
}

void CMainWindow::onFilesDropped(const QStringList &filePaths)
{
    if (filePaths.isEmpty())
    {
        return;
    }

    m_lastOpenDirectory = QFileInfo(filePaths.first()).absolutePath();

    if (m_viewModel)
    {
        const auto state = m_imageViewer->viewState();
        MainViewModel::SViewState viewState{state.zoom, state.pan, state.rotation};
        m_viewModel->loadFiles(filePaths, viewState, m_imageViewer->windowLevel());
    }
}

/**
 * @brief Handles Reset Window/Level action
 */
void CMainWindow::onResetWindowLevelClicked()
{
    m_imageViewer->resetWindowLevel();
}

/**
 * @brief Handles window/level changes from image viewer
 * @param center New window center value
 * @param width New window width value
 */
void CMainWindow::onWindowLevelChanged(double center, double width)
{
    updateWindowLevelDisplay(center, width);

    if (m_thumbnailWidget && m_viewModel && m_viewModel->currentIndex() >= 0)
    {
        const QImage thumb =
            m_imageViewer->renderThumbnail(m_thumbnailWidget->thumbnailSize());
        if (!thumb.isNull())
        {
            m_thumbnailWidget->setThumbnailImage(m_viewModel->currentIndex(), thumb);
        }
        else
        {
            m_thumbnailWidget->updateThumbnail(m_viewModel->currentIndex(), m_imageViewer->paletteType());
        }
    }

    if (m_viewModel)
    {
        m_viewModel->updateCurrentWindowLevel(center, width);
    }
}

/**
 * @brief Handles About action
 */
void CMainWindow::onAboutClicked()
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("About DICOM Viewer"));
    dialog.setFixedSize(480, 480);

    auto *layout = new QVBoxLayout(&dialog);
    layout->setSpacing(12);
    layout->setContentsMargins(24, 24, 24, 24);

    // Logo
    auto *logoLabel = new QLabel(&dialog);
    QPixmap logo(":/images/splash-screen.png");
    logoLabel->setPixmap(logo.scaledToWidth(180, Qt::SmoothTransformation));
    logoLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(logoLabel);

    // Title and version
    auto *titleLabel = new QLabel(tr("<h2 style='margin:0;'>DICOM Viewer</h2>"
                                     "<p style='color:#666; margin:4px 0 0 0;'>Version 1.0</p>"),
                                  &dialog);
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);

    // Description
    auto *descLabel = new QLabel(tr("Medical imaging viewer for DICOM files.\n"
                                    "Built with C++17, Qt 6, and DCMTK."),
                                 &dialog);
    descLabel->setAlignment(Qt::AlignCenter);
    descLabel->setStyleSheet("color: #555;");
    layout->addWidget(descLabel);

    // Features
    auto *featuresLabel = new QLabel(tr(
                                         "<p style='margin:8px 0 4px 0; font-weight:bold; color:#444;'>Features:</p>"
                                         "<ul style='margin:0; padding-left:20px; color:#555;'>"
                                         "<li>Load and display DICOM medical images</li>"
                                         "<li>Window/Level adjustment with mouse drag</li>"
                                         "<li>Multiple color palettes (Hot, Cool, Rainbow, etc.)</li>"
                                         "<li>Zoom, pan, and rotation controls</li>"
                                         "<li>DICOM metadata display panel</li>"
                                         "<li>Thumbnail view for multiple images</li>"
                                         "<li>Export to PNG, JPEG, and PDF</li>"
                                         "<li>Generate PDF diagnostic reports</li>"
                                         "</ul>"),
                                     &dialog);
    featuresLabel->setWordWrap(true);
    layout->addWidget(featuresLabel);

    layout->addStretch();

    // Copyright
    auto *copyrightLabel = new QLabel(tr("© 2026 DICOM Viewer Project"), &dialog);
    copyrightLabel->setAlignment(Qt::AlignCenter);
    copyrightLabel->setStyleSheet("color: #888; font-size: 11px;");
    layout->addWidget(copyrightLabel);

    dialog.exec();
}

/**
 * @brief Displays an error message to the user
 * @param message Error message to display
 */
void CMainWindow::showError(const QString &message)
{
    QMessageBox::critical(this, tr("Error"), message);
}

/**
 * @brief Updates the window/level display in status bar
 * @param center Window center value
 * @param width Window width value
 */
void CMainWindow::updateWindowLevelDisplay(double center, double width)
{
    m_windowLevelLabel->setText(
        tr("W/L: %1 / %2")
            .arg(static_cast<int>(width))
            .arg(static_cast<int>(center)));
}

/**
 * @brief Updates the image type display in status bar
 * @param image Pointer to the current image
 */
void CMainWindow::updateImageTypeDisplay(const CDicomImage *image)
{
    if (image == nullptr || !image->isValid())
    {
        m_imageTypeLabel->setText(tr("No image"));
        m_imageTypeLabel->setStyleSheet("QLabel { padding: 2px 8px; color: gray; }");
        m_imageSizeLabel->setText("");
        return;
    }

    // Determine image type and set color indicator
    auto pi = image->photometricInterpretation();
    QString typeText;
    QString styleSheet;

    switch (pi)
    {
    case DicomViewer::EPhotometricInterpretation::Monochrome1:
        typeText = tr("Grayscale (M1)");
        styleSheet = "QLabel { padding: 2px 8px; background-color: #555; color: white; border-radius: 3px; }";
        break;
    case DicomViewer::EPhotometricInterpretation::Monochrome2:
        typeText = tr("Grayscale (M2)");
        styleSheet = "QLabel { padding: 2px 8px; background-color: #555; color: white; border-radius: 3px; }";
        break;
    case DicomViewer::EPhotometricInterpretation::Rgb:
        typeText = tr("RGB Color");
        styleSheet = "QLabel { padding: 2px 8px; background-color: #28a745; color: white; border-radius: 3px; }";
        break;
    case DicomViewer::EPhotometricInterpretation::PaletteColor:
        typeText = tr("Palette Color");
        styleSheet = "QLabel { padding: 2px 8px; background-color: #17a2b8; color: white; border-radius: 3px; }";
        break;
    default:
        typeText = tr("Unknown");
        styleSheet = "QLabel { padding: 2px 8px; background-color: #dc3545; color: white; border-radius: 3px; }";
        break;
    }

    m_imageTypeLabel->setText(typeText);
    m_imageTypeLabel->setStyleSheet(styleSheet);

    // Update image size
    auto dims = image->dimensions();
    m_imageSizeLabel->setText(tr("%1 x %2").arg(dims.width).arg(dims.height));
}

void CMainWindow::applyPaletteState(DicomViewer::EPaletteType type)
{
    if (m_paletteLabel)
    {
        m_paletteLabel->setText(tr("Palette: %1").arg(CColorPalette::paletteName(type)));
    }

    if (m_paletteActionGroup)
    {
        for (QAction *action : m_paletteActionGroup->actions())
        {
            if (action->data().toInt() == static_cast<int>(type))
            {
                action->setChecked(true);
                break;
            }
        }
    }

    if (!m_thumbnailWidget || !m_viewModel)
    {
        return;
    }

    const int currentIndex = m_viewModel->currentIndex();
    if (currentIndex < 0)
    {
        return;
    }

    const QImage thumb = m_imageViewer->renderThumbnail(m_thumbnailWidget->thumbnailSize());
    if (!thumb.isNull())
    {
        m_thumbnailWidget->setThumbnailImage(currentIndex, thumb);
    }
    else
    {
        m_thumbnailWidget->updateThumbnail(currentIndex, type);
    }
}

/**
 * @brief Handles palette selection from menu
 */
void CMainWindow::onPaletteSelected()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (action)
    {
        DicomViewer::EPaletteType type = static_cast<DicomViewer::EPaletteType>(action->data().toInt());
        m_imageViewer->setColorPalette(type);
    }
}

/**
 * @brief Handles palette change notification
 * @param type New palette type
 */
void CMainWindow::onPaletteChanged(DicomViewer::EPaletteType type)
{
    if (m_viewModel)
    {
        m_viewModel->updateCurrentPalette(type);
        return;
    }

    applyPaletteState(type);
}

void CMainWindow::onViewStateChanged(double zoom, double panX, double panY, int rotation)
{
    if (!m_viewModel || m_viewModel->currentIndex() < 0)
    {
        return;
    }

    const auto *entry = m_viewModel->currentEntry();
    const bool rotationChanged = (entry && entry->rotation != rotation);
    m_viewModel->updateCurrentViewState(zoom, panX, panY, rotation);

    if (rotationChanged && m_thumbnailWidget)
    {
        const QImage thumb =
            m_imageViewer->renderThumbnail(m_thumbnailWidget->thumbnailSize());
        if (!thumb.isNull())
        {
            m_thumbnailWidget->setThumbnailImage(m_viewModel->currentIndex(), thumb);
        }
        else
        {
            m_thumbnailWidget->updateThumbnail(m_viewModel->currentIndex(), entry ? entry->palette : m_imageViewer->paletteType());
        }
    }
}

void CMainWindow::onThumbnailSelected(int index)
{
    if (!m_viewModel)
    {
        return;
    }

    if (index == m_viewModel->currentIndex())
    {
        return;
    }

    const auto state = m_imageViewer->viewState();
    MainViewModel::SViewState viewState{state.zoom, state.pan, state.rotation};
    const auto windowLevel = m_imageViewer->windowLevel();
    m_viewModel->selectImage(index, viewState, windowLevel);
}

void CMainWindow::onThumbnailDeleteRequested(int index)
{
    if (!m_viewModel)
    {
        return;
    }

    const auto state = m_imageViewer->viewState();
    MainViewModel::SViewState viewState{state.zoom, state.pan, state.rotation};
    const auto windowLevel = m_imageViewer->windowLevel();
    m_viewModel->removeImage(index, viewState, windowLevel);
}

void CMainWindow::onImageAdded(int index)
{
    if (!m_viewModel)
    {
        return;
    }

    const auto *entry = m_viewModel->entryAt(index);
    if (!entry || !entry->image)
    {
        return;
    }

    if (m_thumbnailWidget)
    {
        const QString label = QFileInfo(entry->filePath).fileName();
        m_thumbnailWidget->addImage(label, entry->filePath, entry->image);
    }

    emit imageLoaded(entry->filePath);
    if (statusBar())
    {
        statusBar()->showMessage(tr("Loaded: %1").arg(entry->filePath), 3000);
    }
}

void CMainWindow::onImageRemoved(int index)
{
    if (m_thumbnailWidget)
    {
        m_thumbnailWidget->removeImage(index);
    }
}

void CMainWindow::applyCurrentImage()
{
    if (!m_viewModel)
    {
        return;
    }

    const auto *entry = m_viewModel->currentEntry();
    if (!entry || !entry->image)
    {
        m_imageViewer->clearImage();
        m_metadataPanel->clearMetadata();
        updateWindowLevelDisplay(0, 0);
        updateImageTypeDisplay(nullptr);
        setWindowTitle(tr("DICOM Viewer"));
        if (m_thumbnailWidget)
        {
            QSignalBlocker blocker(m_thumbnailWidget);
            m_thumbnailWidget->setCurrentIndex(-1);
        }
        return;
    }

    m_imageViewer->setDicomImage(entry->image);
    m_imageViewer->setColorPalette(entry->palette);
    m_imageViewer->setViewState({entry->zoom, entry->pan, entry->rotation});
    applyPaletteState(entry->palette);
    if (entry->windowLevel.width > 0.0)
    {
        m_imageViewer->setWindowLevel(entry->windowLevel);
    }
    else
    {
        m_imageViewer->resetWindowLevel();
    }

    m_metadataPanel->setMetadata(entry->image->metadata());

    const auto wl = entry->image->windowLevel();
    updateWindowLevelDisplay(wl.center, wl.width);
    updateImageTypeDisplay(entry->image.get());

    if (m_thumbnailWidget)
    {
        QSignalBlocker blocker(m_thumbnailWidget);
        m_thumbnailWidget->setCurrentIndex(m_viewModel->currentIndex());
    }

    setWindowTitle(tr("DICOM Viewer - %1").arg(QFileInfo(entry->filePath).fileName()));
    if (statusBar())
    {
        statusBar()->showMessage(tr("Selected: %1").arg(entry->filePath), 3000);
    }

    if (m_thumbnailWidget)
    {
        const QImage thumb =
            m_imageViewer->renderThumbnail(m_thumbnailWidget->thumbnailSize());
        if (!thumb.isNull())
        {
            m_thumbnailWidget->setThumbnailImage(m_viewModel->currentIndex(), thumb);
        }
        else
        {
            m_thumbnailWidget->updateThumbnail(m_viewModel->currentIndex(), entry->palette);
        }
    }
}

void CMainWindow::onZoomIn()
{
    if (m_imageViewer)
    {
        m_imageViewer->zoomIn();
    }
}

void CMainWindow::onZoomOut()
{
    if (m_imageViewer)
    {
        m_imageViewer->zoomOut();
    }
}

void CMainWindow::onFitToWindow()
{
    if (m_imageViewer)
    {
        m_imageViewer->zoomToFit();
    }
}

void CMainWindow::onActualSize()
{
    if (m_imageViewer)
    {
        m_imageViewer->zoomActualSize();
    }
}

void CMainWindow::onRotateLeft()
{
    if (m_imageViewer)
    {
        m_imageViewer->rotateLeft();
    }
}

void CMainWindow::onRotateRight()
{
    if (m_imageViewer)
    {
        m_imageViewer->rotateRight();
    }
}

void CMainWindow::onResetView()
{
    if (m_imageViewer)
    {
        m_imageViewer->resetView();
    }
}

void CMainWindow::onExportPng()
{
    if (!m_viewModel || !m_viewModel->currentEntry() ||
        !m_viewModel->currentEntry()->image ||
        !m_viewModel->currentEntry()->image->isValid())
    {
        showError(tr("No image loaded to export."));
        return;
    }

    QString defaultName = "dicom_export.png";
    const auto *entry = m_viewModel->currentEntry();
    if (entry)
    {
        defaultName = QFileInfo(entry->filePath).baseName() + ".png";
    }

    QString filePath = QFileDialog::getSaveFileName(
        this,
        tr("Export as PNG"),
        m_lastOpenDirectory.isEmpty() ? QDir::homePath() + "/" + defaultName
                                      : m_lastOpenDirectory + "/" + defaultName,
        tr("PNG Image (*.png)"));

    if (filePath.isEmpty())
    {
        return;
    }

    m_viewModel->exportCurrentImage(filePath, "PNG");
}

void CMainWindow::onExportJpeg()
{
    if (!m_viewModel || !m_viewModel->currentEntry() ||
        !m_viewModel->currentEntry()->image ||
        !m_viewModel->currentEntry()->image->isValid())
    {
        showError(tr("No image loaded to export."));
        return;
    }

    QString defaultName = "dicom_export.jpg";
    const auto *entry = m_viewModel->currentEntry();
    if (entry)
    {
        defaultName = QFileInfo(entry->filePath).baseName() + ".jpg";
    }

    QString filePath = QFileDialog::getSaveFileName(
        this,
        tr("Export as JPEG"),
        m_lastOpenDirectory.isEmpty() ? QDir::homePath() + "/" + defaultName
                                      : m_lastOpenDirectory + "/" + defaultName,
        tr("JPEG Image (*.jpg *.jpeg)"));

    if (filePath.isEmpty())
    {
        return;
    }

    m_viewModel->exportCurrentImage(filePath, "JPEG");
}

void CMainWindow::onExportPdf()
{
    if (!m_viewModel || !m_viewModel->currentEntry() ||
        !m_viewModel->currentEntry()->image ||
        !m_viewModel->currentEntry()->image->isValid())
    {
        showError(tr("No image loaded to export."));
        return;
    }

    QString defaultName = "dicom_export.pdf";
    const auto *entry = m_viewModel->currentEntry();
    if (entry)
    {
        defaultName = QFileInfo(entry->filePath).baseName() + ".pdf";
    }

    QString filePath = QFileDialog::getSaveFileName(
        this,
        tr("Export as PDF"),
        m_lastOpenDirectory.isEmpty() ? QDir::homePath() + "/" + defaultName
                                      : m_lastOpenDirectory + "/" + defaultName,
        tr("PDF Document (*.pdf)"));

    if (filePath.isEmpty())
    {
        return;
    }

    m_viewModel->exportCurrentImagePdf(filePath);
}

bool CMainWindow::requestReportComment(QString &comment)
{
    QDialog dialog(this);
    dialog.setWindowTitle(tr("Report Comments"));
    dialog.setModal(true);
    dialog.setFixedSize(520, 320);

    auto *layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(20, 20, 20, 20);
    layout->setSpacing(10);

    auto *titleLabel = new QLabel(tr("Add comments to the report?"), &dialog);
    titleLabel->setStyleSheet("font-weight: 600; color: #1a365d;");
    layout->addWidget(titleLabel);

    auto *hintLabel = new QLabel(tr("These comments will appear in a dedicated section of the PDF report."), &dialog);
    hintLabel->setWordWrap(true);
    hintLabel->setStyleSheet("color: #555;");
    layout->addWidget(hintLabel);

    auto *textEdit = new QTextEdit(&dialog);
    textEdit->setAcceptRichText(false);
    textEdit->setPlaceholderText(tr("Type your comments here (optional)..."));
    textEdit->setMinimumHeight(150);
    layout->addWidget(textEdit);

    auto *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);

    if (dialog.exec() != QDialog::Accepted)
    {
        return false;
    }

    comment = textEdit->toPlainText().trimmed();
    return true;
}

void CMainWindow::onGenerateReport()
{
    if (!m_viewModel || !m_viewModel->currentEntry() ||
        !m_viewModel->currentEntry()->image ||
        !m_viewModel->currentEntry()->image->isValid())
    {
        showError(tr("No image loaded to generate report."));
        return;
    }

    QString defaultName = "dicom_report.pdf";
    const auto *entry = m_viewModel->currentEntry();
    if (entry)
    {
        defaultName = QFileInfo(entry->filePath).baseName() + "_report.pdf";
    }

    QString filePath = QFileDialog::getSaveFileName(
        this,
        tr("Generate Report"),
        m_lastOpenDirectory.isEmpty() ? QDir::homePath() + "/" + defaultName
                                      : m_lastOpenDirectory + "/" + defaultName,
        tr("PDF Document (*.pdf)"));

    if (filePath.isEmpty())
    {
        return;
    }

    QString reportComment;
    if (!requestReportComment(reportComment))
    {
        return;
    }

    if (m_viewModel)
    {
        m_viewModel->generateReport(filePath, reportComment);
    }
}
