/**
 * @file CImageViewer.h
 * @brief DICOM image viewer widget class declaration
 * @author DICOM Viewer Project
 * @date 2026
 *
 * Defines the CImageViewer widget which displays DICOM images and
 * provides mouse-based window/level adjustment for contrast control.
 */

#pragma once

#include "core/CDicomImage.h"
#include "utils/CColorPalette.h"
#include "utils/CImageConverter.h"

#include <QImage>
#include <QOpenGLFunctions>
#include <QOpenGLWidget>
#include <QPoint>
#include <QPointF>
#include <QPointer>
#include <QStringList>
#include <QVector>
#include <memory>

class QFrame;
class QEvent;
class QPropertyAnimation;
class QToolButton;
class QWidget;
class QDragEnterEvent;
class QDragMoveEvent;
class QDragLeaveEvent;
class QDropEvent;
class QSlider;
class QLabel;
class QOpenGLShaderProgram;
class QOpenGLBuffer;
class QOpenGLVertexArrayObject;
class QOpenGLTexture;

/**
 * @class CImageViewer
 * @brief Widget for displaying DICOM images with window/level control
 *
 * Displays DICOM images scaled to fit the widget while maintaining
 * aspect ratio. Provides interactive window/level adjustment via
 * mouse drag (horizontal = width, vertical = center).
 */
class CImageViewer : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

  public:
    struct SViewState
    {
        double zoom = 1.0;
        QPointF pan;
        int rotation = 0;
    };

    /**
     * @brief Constructor
     * @param parent Parent widget
     */
    explicit CImageViewer(QWidget *parent = nullptr);

    /**
     * @brief Destructor
     */
    ~CImageViewer() override;

    /** @name Image Management */
    ///@{
    /**
     * @brief Sets the DICOM image to display
     * @param image Shared pointer to DICOM image
     */
    void setDicomImage(std::shared_ptr<CDicomImage> image);

    /**
     * @brief Clears the displayed image
     */
    void clearImage();
    ///@}

    /** @name Window/Level Control */
    ///@{
    /**
     * @brief Sets window/level values
     * @param wl Window/level settings
     */
    void setWindowLevel(const DicomViewer::SWindowLevel &wl);

    /**
     * @brief Retrieves current window/level values
     * @return Current window/level settings
     */
    DicomViewer::SWindowLevel windowLevel() const;

    /**
     * @brief Resets window/level to default values
     */
    void resetWindowLevel();

    /**
     * @brief Enables or disables mouse window/level adjustment
     * @param enabled True to enable adjustment
     */
    void setWindowLevelAdjustmentEnabled(bool enabled);

    /**
     * @brief Checks if mouse adjustment is enabled
     * @return True if enabled
     */
    bool isWindowLevelAdjustmentEnabled() const;
    ///@}

    /** @name Color Palette Control */
    ///@{
    /**
     * @brief Sets the color palette for display
     * @param type Palette type to apply
     */
    void setColorPalette(DicomViewer::EPaletteType type);

    /**
     * @brief Gets the current palette type
     * @return Current palette type
     */
    DicomViewer::EPaletteType paletteType() const;
    ///@}

    /**
     * @brief Gets the current view state (zoom/pan/rotation)
     * @return View state
     */
    SViewState viewState() const;

    /**
     * @brief Applies a view state (zoom/pan/rotation)
     * @param state View state to apply
     */
    void setViewState(const SViewState &state);

    /**
     * @brief Renders a thumbnail using the current render pipeline
     * @param size Target thumbnail size
     * @return QImage of the thumbnail (empty if not available)
     */
    QImage renderThumbnail(const QSize &size);

    /** @name View Controls */
    ///@{
    void zoomIn();
    void zoomOut();
    void zoomToFit();
    void zoomActualSize();
    void rotateLeft();
    void rotateRight();
    void resetView();
    ///@}

  signals:
    /**
     * @brief Emitted when palette changes
     * @param type New palette type
     */
    void paletteChanged(DicomViewer::EPaletteType type);
    /**
     * @brief Emitted when window/level values change
     * @param center New window center value
     * @param width New window width value
     */
    void windowLevelChanged(double center, double width);

    /**
     * @brief Emitted when image is cleared
     */
    void imageCleared();
    /**
     * @brief Emitted when files are dropped onto the viewer
     * @param paths Dropped file paths
     */
    void filesDropped(const QStringList &paths);
    void viewStateChanged(double zoom, double panX, double panY, int rotation);

  protected:
    /** @name Event Handlers */
    ///@{
    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int width, int height) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;
    ///@}

  private:
    /** @name Setup Methods */
    ///@{
    void setupWidgetProperties();
    void setupWindowLevelPanel();
    void setupHud();
    void setupPaletteSelector();
    void setupHudConnections();
    ///@}

    /** @name Internal Methods */
    ///@{
    /**
     * @brief Updates the display image from DICOM data
     */
    void updateDisplayImage();

    double fitScale() const;
    QSize rotatedImageSize() const;
    void positionHud();
    void positionWLControls();
    void positionPaletteOptions();
    void setPaletteOpen(bool open);
    int paletteIndex(DicomViewer::EPaletteType type) const;
    void configureWindowLevelControls();
    void syncWindowLevelControls();
    bool hasImage() const;
    QSize imagePixelSize() const;
    void ensureGlResources();
    void uploadTexture();
    void uploadPalette();
    void updateGeometry();
    void notifyViewStateChanged();
    ///@}

    std::shared_ptr<CDicomImage> m_dicomImage; /**< Source DICOM image */
    CImageConverter m_converter;               /**< Image converter instance */

    bool m_isAdjustingWindowLevel = false;      /**< Mouse drag state */
    bool m_isPanning = false;                   /**< Pan drag state */
    QPoint m_lastMousePos;                      /**< Last mouse position */
    QPointF m_panOffset;                        /**< Pan offset for dragging */
    bool m_windowLevelAdjustmentEnabled = true; /**< Adjustment enabled flag */

    QWidget *m_hud = nullptr;
    QWidget *m_wlSlidersPanel = nullptr;
    QSlider *m_centerSlider = nullptr;
    QSlider *m_widthSlider = nullptr;
    QLabel *m_centerValueLabel = nullptr;
    QLabel *m_widthValueLabel = nullptr;
    QPointer<QPropertyAnimation> m_wlExpandAnim;
    bool m_wlSlidersOpen = false;
    QToolButton *m_paletteButton = nullptr;
    QWidget *m_paletteOptions = nullptr;
    QPointer<QPropertyAnimation> m_paletteExpandAnim;
    QVector<QToolButton *> m_paletteOptionButtons;
    QVector<DicomViewer::EPaletteType> m_paletteOptionTypes;
    bool m_paletteOpen = false;
    double m_zoom = 1.0;
    int m_rotationDegrees = 0;
    bool m_isDragActive = false;
    int m_windowCenterMin = 0;
    int m_windowCenterMax = 0;
    int m_windowWidthMin = 1;
    int m_windowWidthMax = 1;

    QOpenGLShaderProgram *m_shaderProgram = nullptr;
    QOpenGLVertexArrayObject *m_vao = nullptr;
    QOpenGLBuffer *m_vbo = nullptr;
    QOpenGLTexture *m_texture = nullptr;
    QOpenGLTexture *m_paletteTexture = nullptr;
    bool m_textureDirty = false;
    bool m_paletteDirty = false;
    bool m_verticesDirty = false;
    bool m_textureIsRgb = false;
    int m_textureValueMin = 0;
    int m_textureValueMax = 255;
    QImage m_displayImage;
    bool m_useCpuFallback = false;
    bool m_loggedGlInfo = false;
    bool m_loggedDrawError = false;
};
