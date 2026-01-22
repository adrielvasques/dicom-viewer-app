/**
 * @file CImageViewer.cpp
 * @brief Implementation of the CImageViewer class
 * @author DICOM Viewer Project
 * @date 2026
 *
 * Implements the DICOM image display widget with mouse-based
 * window/level adjustment. Renders images scaled to fit while
 * maintaining aspect ratio.
 */

#include "CImageViewer.h"

#include "utils/CColorPalette.h"

#include <QApplication>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QEvent>
#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QMatrix4x4>
#include <QMimeData>
#include <QMouseEvent>
#include <QOpenGLBuffer>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QOpenGLPixelTransferOptions>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QPainter>
#include <QPainterPath>
#include <QPropertyAnimation>
#include <QResizeEvent>
#include <QSignalBlocker>
#include <QSlider>
#include <QStyle>
#include <QSvgRenderer>
#include <QTimer>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>
#include <QWheelEvent>
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

#include <DicomViewer/Debug.h>

namespace
{
constexpr double kSensitivityWidth = 1.0;  /**< Window width sensitivity */
constexpr double kSensitivityCenter = 1.0; /**< Window center sensitivity */
constexpr double kZoomStep = 1.2;
constexpr double kMinZoom = 0.1;
constexpr double kMaxZoom = 8.0;

struct SQuadVertex
{
    float x;
    float y;
    float u;
    float v;
};

const char *kVertexShaderSource = R"(
        #version 330 core
        layout(location = 0) in vec2 a_pos;
        layout(location = 1) in vec2 a_uv;
        uniform mat4 u_mvp;
        out vec2 v_uv;
        void main() {
            gl_Position = u_mvp * vec4(a_pos, 0.0, 1.0);
            v_uv = a_uv;
        }
    )";

const char *kFragmentShaderSource = R"(
        #version 330 core
        uniform sampler2D u_tex;
        uniform sampler2D u_lut;
        uniform int u_isColor;
        uniform int u_usePalette;
        uniform int u_invert;
        uniform float u_wc;
        uniform float u_ww;
        uniform float u_valueMin;
        uniform float u_valueMax;
        in vec2 v_uv;
        out vec4 fragColor;
        void main() {
            if (u_isColor == 1) {
                vec3 c = texture(u_tex, v_uv).rgb;
                fragColor = vec4(c, 1.0);
                return;
            }
            float t = texture(u_tex, v_uv).r;
            float raw = mix(u_valueMin, u_valueMax, t);
            float lower = u_wc - (u_ww * 0.5);
            float upper = u_wc + (u_ww * 0.5);
            float outv;
            if (u_ww <= 0.0) {
                outv = 0.0;
            } else if (raw <= lower) {
                outv = 0.0;
            } else if (raw >= upper) {
                outv = 1.0;
            } else {
                outv = (raw - lower) / u_ww;
            }
            if (u_invert == 1) {
                outv = 1.0 - outv;
            }
            if (u_usePalette == 1) {
                vec3 c = texture(u_lut, vec2(outv, 0.5)).rgb;
                fragColor = vec4(c, 1.0);
            } else {
                fragColor = vec4(outv, outv, outv, 1.0);
            }
        }
    )";

QIcon makePaletteCircleIcon(DicomViewer::EPaletteType type, const QSize &size)
{
    CColorPalette palette(type);
    QImage img(size, QImage::Format_ARGB32);
    img.fill(Qt::transparent);
    const QPointF center(size.width() / 2.0, size.height() / 2.0);
    const double radius = std::min(size.width(), size.height()) / 2.0 - 1.0;
    constexpr double kPi = 3.14159265358979323846;

    for (int y = 0; y < img.height(); ++y)
    {
        for (int x = 0; x < img.width(); ++x)
        {
            const double dx = x + 0.5 - center.x();
            const double dy = y + 0.5 - center.y();
            const double dist = std::sqrt(dx * dx + dy * dy);
            if (dist > radius)
            {
                continue;
            }
            const double angle = std::atan2(dy, dx);
            const double t = (angle + kPi) / (2.0 * kPi);
            const uint8_t value = static_cast<uint8_t>(std::round(t * 255.0));
            img.setPixelColor(x, y, palette.mapColor(value));
        }
    }

    QPainter p(&img);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(QPen(QColor(170, 180, 190), 1));
    p.drawEllipse(QRectF(1.0, 1.0, img.width() - 2.0, img.height() - 2.0));
    return QIcon(QPixmap::fromImage(img));
}

} // namespace

/**
 * @brief Constructor
 * @param parent Parent widget
 */
CImageViewer::CImageViewer(QWidget *parent)
    : QOpenGLWidget(parent)
{
    setupWidgetProperties();
    setupWindowLevelPanel();
    setupHud();
    setupPaletteSelector();
    setupHudConnections();

    emit paletteChanged(DicomViewer::EPaletteType::Grayscale);
    positionHud();
    m_hud->setVisible(false);

    qApp->installEventFilter(this);
}

void CImageViewer::setupWidgetProperties()
{
    setUpdateBehavior(QOpenGLWidget::NoPartialUpdate);
    setMinimumSize(200, 200);
    setMouseTracking(false);
    setFocusPolicy(Qt::StrongFocus);
    setAcceptDrops(true);
    setWindowLevelAdjustmentEnabled(false);

    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::black);
    setPalette(pal);
}

void CImageViewer::setupWindowLevelPanel()
{
    m_wlSlidersPanel = new QWidget(this);
    m_wlSlidersPanel->setObjectName("WLSlidersPanel");
    m_wlSlidersPanel->setAttribute(Qt::WA_TranslucentBackground);

    auto *wlLayout = new QVBoxLayout(m_wlSlidersPanel);
    wlLayout->setContentsMargins(10, 8, 10, 8);
    wlLayout->setSpacing(10);

    auto makeSliderIcon = [](const QString &path)
    {
        QPixmap pix(20, 20);
        pix.fill(Qt::transparent);
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing, true);
        QSvgRenderer svg(path);
        if (svg.isValid())
        {
            QPixmap mask(20, 20);
            mask.fill(Qt::transparent);
            QPainter maskPainter(&mask);
            svg.render(&maskPainter);
            maskPainter.end();
            p.fillRect(pix.rect(), Qt::white);
            p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
            p.drawPixmap(0, 0, mask);
        }
        return pix;
    };

    auto makeWlSlider = [&](const QString &iconPath, QSlider *&slider, QLabel *&valueLabel)
    {
        auto *group = new QWidget(m_wlSlidersPanel);
        auto *layout = new QHBoxLayout(group);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(8);

        auto *iconLabel = new QLabel(group);
        iconLabel->setObjectName("WLSliderIcon");
        iconLabel->setAlignment(Qt::AlignHCenter);
        iconLabel->setPixmap(makeSliderIcon(iconPath));

        valueLabel = new QLabel("--", group);
        valueLabel->setObjectName("WLSliderValue");
        valueLabel->setAlignment(Qt::AlignHCenter);

        slider = new QSlider(Qt::Horizontal, group);
        slider->setObjectName("WLSliderHorizontal");
        slider->setFixedWidth(170);
        slider->setFixedHeight(20);

        layout->addWidget(iconLabel);
        layout->addWidget(slider, 0);
        layout->addWidget(valueLabel);
        return group;
    };

    wlLayout->addWidget(makeWlSlider(":/icons/sun.svg", m_centerSlider, m_centerValueLabel));
    wlLayout->addWidget(makeWlSlider(":/icons/contrast.svg", m_widthSlider, m_widthValueLabel));
    m_wlSlidersPanel->setVisible(false);

    connect(m_centerSlider, &QSlider::valueChanged, this, [this](int value)
            {
        if (!m_dicomImage) return;
        auto wl = m_dicomImage->windowLevel();
        wl.center = value;
        setWindowLevel(wl); });

    connect(m_widthSlider, &QSlider::valueChanged, this, [this](int value)
            {
        if (!m_dicomImage) return;
        auto wl = m_dicomImage->windowLevel();
        wl.width = std::max(static_cast<double>(DicomViewer::kMinWindowWidth),
                            static_cast<double>(value));
        setWindowLevel(wl); });
}

void CImageViewer::setupHud()
{
    const QColor iconColor(28, 44, 56);
    const QSize iconSize(22, 22);
    constexpr int kButtonSize = 41;

    m_hud = new QWidget(this);
    m_hud->setObjectName("ImageHud");
    m_hud->setAttribute(Qt::WA_TranslucentBackground);

    auto *hudLayout = new QVBoxLayout(m_hud);
    hudLayout->setContentsMargins(14, 14, 14, 14);
    hudLayout->setSpacing(10);
    hudLayout->setAlignment(Qt::AlignRight);

    auto makeSvgIcon = [&](const QString &path)
    {
        QPixmap pix(iconSize);
        pix.fill(Qt::transparent);
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing, true);
        QSvgRenderer svg(path);
        if (svg.isValid())
        {
            QPixmap mask(iconSize);
            mask.fill(Qt::transparent);
            QPainter maskPainter(&mask);
            svg.render(&maskPainter);
            maskPainter.end();
            p.fillRect(pix.rect(), iconColor);
            p.setCompositionMode(QPainter::CompositionMode_DestinationIn);
            p.drawPixmap(0, 0, mask);
        }
        return QIcon(pix);
    };

    auto makeMagnifierIcon = [&](bool plus)
    {
        QPixmap pix(iconSize);
        pix.fill(Qt::transparent);
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing, true);
        QPen pen(iconColor, 1.8);
        pen.setCapStyle(Qt::RoundCap);
        p.setPen(pen);
        p.scale(iconSize.width() / 24.0, iconSize.height() / 24.0);
        p.drawEllipse(QPointF(10.0, 10.0), 6.5, 6.5);
        p.drawLine(QPointF(15.0, 15.0), QPointF(20.0, 20.0));
        p.drawLine(QPointF(7.0, 10.0), QPointF(13.0, 10.0));
        if (plus)
            p.drawLine(QPointF(10.0, 7.0), QPointF(10.0, 13.0));
        return QIcon(pix);
    };

    auto makeFitIcon = [&]()
    {
        QPixmap pix(iconSize);
        pix.fill(Qt::transparent);
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing, true);
        QPen pen(iconColor, 1.8);
        pen.setCapStyle(Qt::RoundCap);
        p.setPen(pen);
        p.scale(iconSize.width() / 24.0, iconSize.height() / 24.0);
        p.drawLine(QPointF(4.0, 9.0), QPointF(4.0, 4.0));
        p.drawLine(QPointF(4.0, 4.0), QPointF(9.0, 4.0));
        p.drawLine(QPointF(15.0, 4.0), QPointF(20.0, 4.0));
        p.drawLine(QPointF(20.0, 4.0), QPointF(20.0, 9.0));
        p.drawLine(QPointF(4.0, 15.0), QPointF(4.0, 20.0));
        p.drawLine(QPointF(4.0, 20.0), QPointF(9.0, 20.0));
        p.drawLine(QPointF(15.0, 20.0), QPointF(20.0, 20.0));
        p.drawLine(QPointF(20.0, 20.0), QPointF(20.0, 15.0));
        return QIcon(pix);
    };

    auto makeActualIcon = [&]()
    {
        QPixmap pix(iconSize);
        pix.fill(Qt::transparent);
        QPainter p(&pix);
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setRenderHint(QPainter::TextAntialiasing, true);
        p.scale(iconSize.width() / 24.0, iconSize.height() / 24.0);
        QFont font = p.font();
        font.setPixelSize(11);
        font.setBold(true);
        p.setFont(font);
        p.setPen(iconColor);
        p.drawText(QRectF(0, 0, 24, 24), Qt::AlignCenter, "1:1");
        return QIcon(pix);
    };

    auto makeButton = [&](const QIcon &icon, const QString &tooltip)
    {
        auto *button = new QToolButton(m_hud);
        button->setIcon(icon);
        button->setIconSize(iconSize);
        button->setToolTip(tooltip);
        button->setAutoRaise(true);
        button->setToolButtonStyle(Qt::ToolButtonIconOnly);
        button->setFixedSize(kButtonSize, kButtonSize);
        return button;
    };

    auto *zoomInButton = makeButton(makeMagnifierIcon(true), tr("Zoom in"));
    auto *zoomOutButton = makeButton(makeMagnifierIcon(false), tr("Zoom out"));
    auto *fitButton = makeButton(makeFitIcon(), tr("Fit to window"));
    auto *actualButton = makeButton(makeActualIcon(), tr("Actual size"));
    auto *rotateLeftButton = makeButton(makeSvgIcon(":/icons/rotate-left.svg"), tr("Rotate left"));
    auto *rotateRightButton = makeButton(makeSvgIcon(":/icons/rotate-right.svg"), tr("Rotate right"));
    auto *resetButton = makeButton(makeSvgIcon(":/icons/reset.svg"), tr("Reset view"));

    m_paletteButton = new QToolButton(m_hud);
    m_paletteButton->setObjectName("PaletteButton");
    m_paletteButton->setIcon(makePaletteCircleIcon(DicomViewer::EPaletteType::Grayscale, iconSize));
    m_paletteButton->setIconSize(iconSize);
    m_paletteButton->setAutoRaise(true);
    m_paletteButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_paletteButton->setFixedSize(34, 34);
    m_paletteButton->setToolTip(tr("Palette"));

    hudLayout->addWidget(zoomInButton);
    hudLayout->addWidget(zoomOutButton);
    hudLayout->addWidget(fitButton);
    hudLayout->addWidget(actualButton);
    hudLayout->addWidget(rotateLeftButton);
    hudLayout->addWidget(rotateRightButton);
    hudLayout->addWidget(m_paletteButton);
    hudLayout->addWidget(resetButton);

    // Store button references for connections
    zoomInButton->setObjectName("ZoomInButton");
    zoomOutButton->setObjectName("ZoomOutButton");
    fitButton->setObjectName("FitButton");
    actualButton->setObjectName("ActualButton");
    rotateLeftButton->setObjectName("RotateLeftButton");
    rotateRightButton->setObjectName("RotateRightButton");
    resetButton->setObjectName("ResetButton");
}

void CImageViewer::setupPaletteSelector()
{
    const QSize optionIconSize(16, 16);

    m_paletteOptions = new QFrame(this);
    m_paletteOptions->setObjectName("PaletteOptions");

    auto *optionsLayout = new QHBoxLayout(m_paletteOptions);
    optionsLayout->setContentsMargins(4, 4, 4, 4);
    optionsLayout->setSpacing(4);
    optionsLayout->setAlignment(Qt::AlignVCenter);

    for (DicomViewer::EPaletteType type : CColorPalette::availablePalettes())
    {
        auto *option = new QToolButton(m_paletteOptions);
        option->setIcon(makePaletteCircleIcon(type, optionIconSize));
        option->setIconSize(optionIconSize);
        option->setAutoRaise(true);
        option->setToolButtonStyle(Qt::ToolButtonIconOnly);
        option->setFixedSize(28, 28);
        option->setCheckable(true);
        option->setToolTip(CColorPalette::paletteName(type));
        optionsLayout->addWidget(option);
        m_paletteOptionButtons.append(option);
        m_paletteOptionTypes.append(type);

        connect(option, &QToolButton::clicked, this, [this, type]()
                {
            if (type != paletteType()) setColorPalette(type);
            setPaletteOpen(false); });
    }

    m_paletteOptions->setMaximumWidth(0);
    m_paletteOptions->setVisible(false);
}

void CImageViewer::setupHudConnections()
{
    const QSize iconSize(22, 22);

    auto *zoomInButton = m_hud->findChild<QToolButton *>("ZoomInButton");
    auto *zoomOutButton = m_hud->findChild<QToolButton *>("ZoomOutButton");
    auto *fitButton = m_hud->findChild<QToolButton *>("FitButton");
    auto *actualButton = m_hud->findChild<QToolButton *>("ActualButton");
    auto *rotateLeftButton = m_hud->findChild<QToolButton *>("RotateLeftButton");
    auto *rotateRightButton = m_hud->findChild<QToolButton *>("RotateRightButton");
    auto *resetButton = m_hud->findChild<QToolButton *>("ResetButton");

    connect(zoomInButton, &QToolButton::clicked, this, &CImageViewer::zoomIn);
    connect(zoomOutButton, &QToolButton::clicked, this, &CImageViewer::zoomOut);
    connect(fitButton, &QToolButton::clicked, this, &CImageViewer::zoomToFit);
    connect(actualButton, &QToolButton::clicked, this, &CImageViewer::zoomActualSize);
    connect(rotateLeftButton, &QToolButton::clicked, this, &CImageViewer::rotateLeft);
    connect(rotateRightButton, &QToolButton::clicked, this, &CImageViewer::rotateRight);

    connect(resetButton, &QToolButton::clicked, this, [this]()
            {
        resetView();
        resetWindowLevel();
        setColorPalette(DicomViewer::EPaletteType::Grayscale);
        setPaletteOpen(false); });

    connect(m_paletteButton, &QToolButton::clicked, this, [this]()
            { setPaletteOpen(!m_paletteOpen); });

    connect(this, &CImageViewer::paletteChanged, this, [this, iconSize](DicomViewer::EPaletteType type)
            {
        if (!m_paletteButton) return;
        m_paletteButton->setIcon(makePaletteCircleIcon(type, iconSize));
        const int index = paletteIndex(type);
        if (index >= 0 && index < m_paletteOptionButtons.size()) {
            for (int i = 0; i < m_paletteOptionButtons.size(); ++i) {
                m_paletteOptionButtons.at(i)->setChecked(i == index);
            }
        } });
}

CImageViewer::~CImageViewer()
{
    if (context())
    {
        makeCurrent();
        delete m_texture;
        m_texture = nullptr;
        delete m_paletteTexture;
        m_paletteTexture = nullptr;
        delete m_vbo;
        m_vbo = nullptr;
        delete m_vao;
        m_vao = nullptr;
        delete m_shaderProgram;
        m_shaderProgram = nullptr;
        doneCurrent();
    }
    else
    {
        delete m_texture;
        m_texture = nullptr;
        delete m_paletteTexture;
        m_paletteTexture = nullptr;
        delete m_vbo;
        m_vbo = nullptr;
        delete m_vao;
        m_vao = nullptr;
        delete m_shaderProgram;
        m_shaderProgram = nullptr;
    }
    if (qApp)
    {
        qApp->removeEventFilter(this);
    }
    if (m_paletteExpandAnim)
    {
        m_paletteExpandAnim->stop();
    }
    if (m_wlExpandAnim)
    {
        m_wlExpandAnim->stop();
    }
}

/**
 * @brief Sets the DICOM image to display
 * @param image Shared pointer to DICOM image
 */
void CImageViewer::setDicomImage(std::shared_ptr<CDicomImage> image)
{
    m_dicomImage = image;
    updateDisplayImage();
    m_paletteDirty = true;
    configureWindowLevelControls();

    // Show HUD when image is loaded
    if (m_hud)
    {
        m_hud->setVisible(image && image->isValid());
    }

    update();
}

/**
 * @brief Clears the displayed image
 */
void CImageViewer::clearImage()
{
    m_dicomImage.reset();
    updateDisplayImage();
    configureWindowLevelControls();

    // Hide HUD when image is cleared
    if (m_hud)
    {
        m_hud->setVisible(false);
    }

    update();
    emit imageCleared();
}

/**
 * @brief Sets window/level values
 * @param wl Window/level settings
 */
void CImageViewer::setWindowLevel(const DicomViewer::SWindowLevel &wl)
{
    if (m_dicomImage)
    {
        m_dicomImage->setWindowLevel(wl);
        syncWindowLevelControls();
        if (m_useCpuFallback)
        {
            updateDisplayImage();
        }
        update();
        emit windowLevelChanged(wl.center, wl.width);
    }
}

/**
 * @brief Retrieves current window/level values
 * @return Current window/level settings, or default if no image
 */
DicomViewer::SWindowLevel CImageViewer::windowLevel() const
{
    if (m_dicomImage)
    {
        return m_dicomImage->windowLevel();
    }
    return DicomViewer::SWindowLevel{};
}

/**
 * @brief Resets window/level to default values from DICOM header
 */
void CImageViewer::resetWindowLevel()
{
    if (m_dicomImage)
    {
        m_dicomImage->resetWindowLevel();
        syncWindowLevelControls();
        if (m_useCpuFallback)
        {
            updateDisplayImage();
        }
        update();
        auto wl = m_dicomImage->windowLevel();
        emit windowLevelChanged(wl.center, wl.width);
    }
}

/**
 * @brief Sets the color palette for display
 * @param type Palette type to apply
 */
void CImageViewer::setColorPalette(DicomViewer::EPaletteType type)
{
    m_converter.setPalette(type);
    m_paletteDirty = true;
    if (m_useCpuFallback)
    {
        updateDisplayImage();
    }
    update();
    emit paletteChanged(type);
}

/**
 * @brief Gets the current palette type
 * @return Current palette type
 */
DicomViewer::EPaletteType CImageViewer::paletteType() const
{
    return m_converter.paletteType();
}

CImageViewer::SViewState CImageViewer::viewState() const
{
    SViewState state;
    state.zoom = m_zoom;
    state.pan = m_panOffset;
    state.rotation = m_rotationDegrees;
    return state;
}

void CImageViewer::setViewState(const SViewState &state)
{
    m_zoom = state.zoom;
    m_panOffset = state.pan;
    m_rotationDegrees = state.rotation;
    update();
}

void CImageViewer::notifyViewStateChanged()
{
    emit viewStateChanged(m_zoom, m_panOffset.x(), m_panOffset.y(), m_rotationDegrees);
}

QImage CImageViewer::renderThumbnail(const QSize &size)
{
    if (!hasImage() || size.isEmpty())
    {
        return QImage();
    }

    if (m_useCpuFallback)
    {
        if (m_displayImage.isNull())
        {
            updateDisplayImage();
        }
        if (m_displayImage.isNull())
        {
            return QImage();
        }
        return m_displayImage.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    }

    if (!context())
    {
        return QImage();
    }

    makeCurrent();
    ensureGlResources();
    if (m_textureDirty)
    {
        uploadTexture();
    }
    if (m_paletteDirty)
    {
        uploadPalette();
    }
    if (m_verticesDirty)
    {
        updateGeometry();
    }

    if (!m_shaderProgram || !m_texture)
    {
        doneCurrent();
        return QImage();
    }

    QOpenGLFramebufferObjectFormat fboFormat;
    fboFormat.setAttachment(QOpenGLFramebufferObject::NoAttachment);
    fboFormat.setInternalTextureFormat(GL_RGBA8);
    QOpenGLFramebufferObject fbo(size, fboFormat);
    if (!fbo.isValid())
    {
        doneCurrent();
        return QImage();
    }

    fbo.bind();
    glViewport(0, 0, size.width(), size.height());
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    const QSize imageSize = imagePixelSize();
    const QSize rotatedSize = rotatedImageSize();
    if (imageSize.isEmpty() || rotatedSize.isEmpty())
    {
        fbo.release();
        doneCurrent();
        return QImage();
    }

    double scale = 1.0;
    if (rotatedSize.width() > 0 && rotatedSize.height() > 0)
    {
        const double scaleX = static_cast<double>(size.width()) / rotatedSize.width();
        const double scaleY = static_cast<double>(size.height()) / rotatedSize.height();
        scale = std::min(scaleX, scaleY);
    }

    QMatrix4x4 proj;
    proj.ortho(0.0f, static_cast<float>(size.width()),
               static_cast<float>(size.height()), 0.0f,
               -1.0f, 1.0f);

    QMatrix4x4 model;
    model.translate(size.width() / 2.0f, size.height() / 2.0f);
    model.rotate(static_cast<float>(m_rotationDegrees), 0.0f, 0.0f, 1.0f);
    model.scale(static_cast<float>(scale), static_cast<float>(scale));
    model.translate(-imageSize.width() / 2.0f, -imageSize.height() / 2.0f);

    const QMatrix4x4 mvp = proj * model;

    m_shaderProgram->bind();
    if (m_vao)
    {
        m_vao->bind();
    }

    const int texUnit = 0;
    m_texture->bind(texUnit);
    m_shaderProgram->setUniformValue("u_tex", texUnit);

    const bool usePalette = (m_converter.paletteType() != DicomViewer::EPaletteType::Grayscale);
    if (usePalette && m_paletteTexture)
    {
        const int lutUnit = 1;
        m_paletteTexture->bind(lutUnit);
        m_shaderProgram->setUniformValue("u_lut", lutUnit);
    }

    const auto wl = m_dicomImage->windowLevel();
    const bool invertForMonochrome1 =
        (m_dicomImage->photometricInterpretation() ==
         DicomViewer::EPhotometricInterpretation::Monochrome1);

    m_shaderProgram->setUniformValue("u_mvp", mvp);
    m_shaderProgram->setUniformValue("u_wc", static_cast<float>(wl.center));
    m_shaderProgram->setUniformValue("u_ww", static_cast<float>(wl.width));
    m_shaderProgram->setUniformValue("u_valueMin", static_cast<float>(m_textureValueMin));
    m_shaderProgram->setUniformValue("u_valueMax", static_cast<float>(m_textureValueMax));
    m_shaderProgram->setUniformValue("u_isColor", m_textureIsRgb ? 1 : 0);
    m_shaderProgram->setUniformValue("u_usePalette", usePalette ? 1 : 0);
    m_shaderProgram->setUniformValue("u_invert", invertForMonochrome1 ? 1 : 0);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    if (m_vao)
    {
        m_vao->release();
    }
    m_shaderProgram->release();
    m_texture->release();
    if (usePalette && m_paletteTexture)
    {
        m_paletteTexture->release();
    }

    QImage image = fbo.toImage(true);
    fbo.release();
    doneCurrent();
    return image;
}

void CImageViewer::zoomIn()
{
    if (!hasImage())
    {
        return;
    }
    m_zoom = std::min(kMaxZoom, m_zoom * kZoomStep);
    update();
    notifyViewStateChanged();
}

void CImageViewer::zoomOut()
{
    if (!hasImage())
    {
        return;
    }
    m_zoom = std::max(kMinZoom, m_zoom / kZoomStep);
    update();
    notifyViewStateChanged();
}

void CImageViewer::zoomToFit()
{
    if (!hasImage())
    {
        return;
    }
    m_zoom = 1.0;
    m_panOffset = QPointF(0, 0);
    update();
    notifyViewStateChanged();
}

void CImageViewer::zoomActualSize()
{
    if (!hasImage())
    {
        return;
    }
    const double base = fitScale();
    if (base <= 0.0)
    {
        return;
    }
    m_zoom = std::max(kMinZoom, std::min(kMaxZoom, 1.0 / base));
    update();
    notifyViewStateChanged();
}

void CImageViewer::rotateLeft()
{
    m_rotationDegrees = (m_rotationDegrees + 270) % 360;
    update();
    notifyViewStateChanged();
}

void CImageViewer::rotateRight()
{
    m_rotationDegrees = (m_rotationDegrees + 90) % 360;
    update();
    notifyViewStateChanged();
}

void CImageViewer::resetView()
{
    m_rotationDegrees = 0;
    m_zoom = 1.0;
    m_panOffset = QPointF(0, 0);
    update();
    notifyViewStateChanged();
}

/**
 * @brief Enables or disables mouse window/level adjustment
 * @param enabled True to enable adjustment
 */
void CImageViewer::setWindowLevelAdjustmentEnabled(bool enabled)
{
    m_windowLevelAdjustmentEnabled = enabled;
}

/**
 * @brief Checks if mouse adjustment is enabled
 * @return True if enabled
 */
bool CImageViewer::isWindowLevelAdjustmentEnabled() const
{
    return m_windowLevelAdjustmentEnabled;
}

void CImageViewer::initializeGL()
{
    initializeOpenGLFunctions();
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    ensureGlResources();
    if (!m_shaderProgram && !m_useCpuFallback)
    {
        m_useCpuFallback = true;
        DICOMVIEWER_WARN("Falling back to CPU rendering (shader unavailable)");
        updateDisplayImage();
    }

    if (!m_loggedGlInfo)
    {
        if (auto *ctx = context())
        {
            const auto fmt = ctx->format();
            DICOMVIEWER_LOG("GL context version:"
                            << fmt.majorVersion() << "." << fmt.minorVersion()
                            << "profile:" << fmt.profile()
                            << "renderer:" << reinterpret_cast<const char *>(glGetString(GL_RENDERER))
                            << "vendor:" << reinterpret_cast<const char *>(glGetString(GL_VENDOR))
                            << "glsl:" << reinterpret_cast<const char *>(glGetString(GL_SHADING_LANGUAGE_VERSION)));
        }
        else
        {
            DICOMVIEWER_WARN("No OpenGL context available in initializeGL");
        }
        m_loggedGlInfo = true;
    }
}

void CImageViewer::paintGL()
{
    if (m_useCpuFallback)
    {
        QPainter painter(this);
        painter.fillRect(rect(), Qt::black);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

        if (!hasImage())
        {
            QColor textColor = palette().color(QPalette::Disabled, QPalette::Text);
            if (!textColor.isValid())
            {
                textColor = palette().color(QPalette::Text);
            }
            textColor.setAlpha(170);

            const QSize dropSize(width() / 2, height() / 2);
            QRect dropRect(QPoint(0, 0), dropSize);
            dropRect.moveCenter(rect().center());
            QPen borderPen(m_isDragActive ? QColor("#38bdf8") : QColor(120, 130, 145));
            borderPen.setWidth(2);
            borderPen.setStyle(Qt::DashLine);
            painter.setPen(borderPen);
            painter.setBrush(Qt::NoBrush);
            painter.drawRoundedRect(dropRect, 14, 14);

            painter.setPen(textColor);
            painter.drawText(dropRect, Qt::AlignCenter,
                             tr("Drag and drop DICOM files here\nor use File > Open"));
            return;
        }

        if (m_displayImage.isNull())
        {
            updateDisplayImage();
        }
        if (m_displayImage.isNull())
        {
            return;
        }

        const QSize imageSize = imagePixelSize();
        if (imageSize.isEmpty())
        {
            return;
        }

        const double base = fitScale();
        const double scale = (base > 0.0) ? base * m_zoom : 1.0;

        QTransform transform;
        transform.translate(width() / 2.0 + m_panOffset.x(),
                            height() / 2.0 + m_panOffset.y());
        transform.rotate(static_cast<qreal>(m_rotationDegrees));
        transform.scale(scale, scale);
        transform.translate(-imageSize.width() / 2.0, -imageSize.height() / 2.0);
        painter.setTransform(transform);
        painter.drawImage(QPointF(0.0, 0.0), m_displayImage);
        return;
    }

    glViewport(0, 0, width(), height());
    glClear(GL_COLOR_BUFFER_BIT);

    if (!hasImage())
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setRenderHint(QPainter::TextAntialiasing, true);

        QColor textColor = palette().color(QPalette::Disabled, QPalette::Text);
        if (!textColor.isValid())
        {
            textColor = palette().color(QPalette::Text);
        }
        textColor.setAlpha(170);

        const QSize dropSize(width() / 2, height() / 2);
        QRect dropRect(QPoint(0, 0), dropSize);
        dropRect.moveCenter(rect().center());
        QPen borderPen(m_isDragActive ? QColor("#38bdf8") : QColor(120, 130, 145));
        borderPen.setWidth(2);
        borderPen.setStyle(Qt::DashLine);
        painter.setPen(borderPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(dropRect, 14, 14);

        painter.setPen(textColor);
        painter.drawText(dropRect, Qt::AlignCenter,
                         tr("Drag and drop DICOM files here\nor use File > Open"));
        return;
    }

    ensureGlResources();
    if (m_textureDirty)
    {
        uploadTexture();
    }
    if (m_paletteDirty)
    {
        uploadPalette();
    }
    if (m_verticesDirty)
    {
        updateGeometry();
    }

    if (!m_shaderProgram || !m_texture)
    {
        if (!m_loggedDrawError)
        {
            DICOMVIEWER_WARN("OpenGL draw skipped (shader/texture missing)"
                             << "shader:" << (m_shaderProgram != nullptr)
                             << "texture:" << (m_texture != nullptr));
            m_loggedDrawError = true;
        }
        return;
    }

    const QSize imageSize = imagePixelSize();
    if (imageSize.isEmpty())
    {
        return;
    }

    const double base = fitScale();
    const double scale = (base > 0.0) ? base * m_zoom : 1.0;

    QMatrix4x4 proj;
    proj.ortho(0.0f, static_cast<float>(width()),
               static_cast<float>(height()), 0.0f,
               -1.0f, 1.0f);

    QMatrix4x4 model;
    model.translate(width() / 2.0f + static_cast<float>(m_panOffset.x()),
                    height() / 2.0f + static_cast<float>(m_panOffset.y()));
    model.rotate(static_cast<float>(m_rotationDegrees), 0.0f, 0.0f, 1.0f);
    model.scale(static_cast<float>(scale), static_cast<float>(scale));
    model.translate(-imageSize.width() / 2.0f, -imageSize.height() / 2.0f);

    const QMatrix4x4 mvp = proj * model;

    if (!m_shaderProgram->bind())
    {
        if (!m_loggedDrawError)
        {
            DICOMVIEWER_ERROR("Failed to bind shader program");
            m_loggedDrawError = true;
        }
        return;
    }

    const bool vaoBound = (m_vao && m_vao->isCreated());
    if (m_vao)
    {
        m_vao->bind();
    }
    if (!vaoBound)
    {
        if (!m_loggedDrawError)
        {
            DICOMVIEWER_ERROR("Failed to bind VAO"
                              << "vao:" << (m_vao != nullptr)
                              << "created:" << (m_vao && m_vao->isCreated()));
            m_loggedDrawError = true;
        }
        m_shaderProgram->release();
        return;
    }

    const int texUnit = 0;
    m_texture->bind(texUnit);
    m_shaderProgram->setUniformValue("u_tex", texUnit);

    const bool usePalette = (m_converter.paletteType() != DicomViewer::EPaletteType::Grayscale);
    if (usePalette && m_paletteTexture)
    {
        const int lutUnit = 1;
        m_paletteTexture->bind(lutUnit);
        m_shaderProgram->setUniformValue("u_lut", lutUnit);
    }

    const auto wl = m_dicomImage->windowLevel();
    const bool invertForMonochrome1 =
        (m_dicomImage->photometricInterpretation() ==
         DicomViewer::EPhotometricInterpretation::Monochrome1);

    m_shaderProgram->setUniformValue("u_mvp", mvp);
    m_shaderProgram->setUniformValue("u_wc", static_cast<float>(wl.center));
    m_shaderProgram->setUniformValue("u_ww", static_cast<float>(wl.width));
    m_shaderProgram->setUniformValue("u_valueMin", static_cast<float>(m_textureValueMin));
    m_shaderProgram->setUniformValue("u_valueMax", static_cast<float>(m_textureValueMax));
    m_shaderProgram->setUniformValue("u_isColor", m_textureIsRgb ? 1 : 0);
    m_shaderProgram->setUniformValue("u_usePalette", usePalette ? 1 : 0);
    m_shaderProgram->setUniformValue("u_invert", invertForMonochrome1 ? 1 : 0);

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    const GLenum drawErr = glGetError();
    if (drawErr != GL_NO_ERROR && !m_loggedDrawError)
    {
        DICOMVIEWER_WARN("OpenGL draw error:" << drawErr
                                              << "vaoCreated:" << (m_vao && m_vao->isCreated())
                                              << "vboCreated:" << (m_vbo && m_vbo->isCreated())
                                              << "vboSize:" << (m_vbo ? m_vbo->size() : -1));
        m_loggedDrawError = true;
    }

    if (m_vao)
    {
        m_vao->release();
    }
    m_shaderProgram->release();
    m_texture->release();
    if (usePalette && m_paletteTexture)
    {
        m_paletteTexture->release();
    }
}

void CImageViewer::resizeGL(int width, int height)
{
    Q_UNUSED(width);
    Q_UNUSED(height);
}

/**
 * @brief Handles mouse press events
 * @param event Mouse event
 */
void CImageViewer::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_dicomImage)
    {
        m_lastMousePos = event->pos();
        if (m_zoom > 1.0)
        {
            // When zoomed in, left button pans
            m_isPanning = true;
            setCursor(Qt::ClosedHandCursor);
        }
        else if (m_windowLevelAdjustmentEnabled)
        {
            // When at normal zoom, left button adjusts window/level
            m_isAdjustingWindowLevel = true;
            setCursor(Qt::SizeAllCursor);
        }
    }

    QOpenGLWidget::mousePressEvent(event);
}

/**
 * @brief Handles mouse move events for window/level adjustment and panning
 *
 * Horizontal movement adjusts window width (contrast).
 * Vertical movement adjusts window center (brightness).
 * Middle mouse button drag pans the image.
 *
 * @param event Mouse event
 */
void CImageViewer::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isPanning && hasImage())
    {
        // Pan the image with bounds checking
        QPointF delta = event->pos() - m_lastMousePos;
        QPointF newOffset = m_panOffset + delta;

        // Calculate scaled image size
        const double base = fitScale();
        const double scale = (base > 0.0) ? base * m_zoom : 1.0;
        const QSize imageSize = rotatedImageSize();
        const double scaledWidth = imageSize.width() * scale;
        const double scaledHeight = imageSize.height() * scale;

        // Calculate maximum pan bounds (only allow pan if image is larger than view)
        const double maxPanX = std::max(0.0, (scaledWidth - width()) / 2.0);
        const double maxPanY = std::max(0.0, (scaledHeight - height()) / 2.0);

        // Clamp pan offset to bounds
        newOffset.setX(std::clamp(newOffset.x(), -maxPanX, maxPanX));
        newOffset.setY(std::clamp(newOffset.y(), -maxPanY, maxPanY));

        m_panOffset = newOffset;
        m_lastMousePos = event->pos();
        update();
        notifyViewStateChanged();
        return;
    }

    if (!m_isAdjustingWindowLevel || !m_dicomImage)
    {
        QOpenGLWidget::mouseMoveEvent(event);
        return;
    }

    QPoint delta = event->pos() - m_lastMousePos;

    auto wl = m_dicomImage->windowLevel();

    // Horizontal movement: adjust width (contrast)
    wl.width += delta.x() * kSensitivityWidth;
    wl.width = std::max(static_cast<double>(DicomViewer::kMinWindowWidth), wl.width);

    // Vertical movement: adjust center (brightness) - inverted for natural feel
    wl.center -= delta.y() * kSensitivityCenter;

    m_dicomImage->setWindowLevel(wl);
    m_lastMousePos = event->pos();

    if (m_useCpuFallback)
    {
        updateDisplayImage();
    }
    update();
    emit windowLevelChanged(wl.center, wl.width);
    syncWindowLevelControls();
}

/**
 * @brief Handles mouse release events
 * @param event Mouse event
 */
void CImageViewer::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton)
    {
        m_isAdjustingWindowLevel = false;
        m_isPanning = false;
        setCursor(Qt::ArrowCursor);
    }

    QOpenGLWidget::mouseReleaseEvent(event);
}

/**
 * @brief Handles mouse wheel events for zooming
 * @param event Wheel event
 */
void CImageViewer::wheelEvent(QWheelEvent *event)
{
    if (!m_dicomImage)
    {
        QOpenGLWidget::wheelEvent(event);
        return;
    }

    constexpr double kZoomFactor = 1.15;
    constexpr double kMinZoom = 0.1;
    constexpr double kMaxZoom = 10.0;

    // Zoom in or out based on wheel direction, always centered
    if (event->angleDelta().y() > 0)
    {
        m_zoom = std::min(m_zoom * kZoomFactor, kMaxZoom);
    }
    else if (event->angleDelta().y() < 0)
    {
        m_zoom = std::max(m_zoom / kZoomFactor, kMinZoom);
    }

    // Reset pan to keep image centered
    m_panOffset = QPointF(0, 0);

    update();
    event->accept();
}

/**
 * @brief Handles resize events
 * @param event Resize event
 */
void CImageViewer::resizeEvent(QResizeEvent *event)
{
    QOpenGLWidget::resizeEvent(event);
    positionHud();
    positionWLControls();
    update();
}

/**
 * @brief Updates the display image from DICOM data
 */
void CImageViewer::updateDisplayImage()
{
    if (!hasImage())
    {
        m_textureDirty = false;
        m_verticesDirty = false;
        m_displayImage = QImage();
        if (m_texture)
        {
            if (context())
            {
                makeCurrent();
                delete m_texture;
                m_texture = nullptr;
                doneCurrent();
            }
            else
            {
                delete m_texture;
                m_texture = nullptr;
            }
        }
        return;
    }

    if (m_useCpuFallback)
    {
        m_displayImage = m_converter.toQImage(*m_dicomImage, m_dicomImage->windowLevel());
        return;
    }

    m_textureDirty = true;
    m_verticesDirty = true;
}

double CImageViewer::fitScale() const
{
    if (!hasImage())
    {
        return 1.0;
    }

    const QSize widgetSize = size();
    const QSize imageSize = rotatedImageSize();
    if (imageSize.isEmpty())
    {
        return 1.0;
    }

    const double scaleX = static_cast<double>(widgetSize.width()) / imageSize.width();
    const double scaleY = static_cast<double>(widgetSize.height()) / imageSize.height();
    return std::min(scaleX, scaleY);
}

QSize CImageViewer::rotatedImageSize() const
{
    if (!hasImage())
    {
        return {};
    }
    const QSize imageSize = imagePixelSize();
    if ((m_rotationDegrees / 90) % 2 == 1)
    {
        return QSize(imageSize.height(), imageSize.width());
    }
    return imageSize;
}

void CImageViewer::positionHud()
{
    if (!m_hud)
    {
        return;
    }
    m_hud->adjustSize();
    const int margin = 24;
    int x = width() - m_hud->width() - margin;
    if (x < margin)
    {
        x = margin;
    }
    m_hud->move(x, margin);
    positionPaletteOptions();
    positionWLControls();
}

void CImageViewer::positionWLControls()
{
    if (!m_wlSlidersPanel)
    {
        return;
    }

    if (!m_wlSlidersPanel->isVisible())
    {
        return;
    }

    const int margin = 24;
    int x = margin;
    int y = height() - m_wlSlidersPanel->height() - margin;
    m_wlSlidersPanel->move(x, y);
}

void CImageViewer::positionPaletteOptions()
{
    if (!m_paletteOptions || !m_paletteButton || !m_paletteOptions->isVisible())
    {
        return;
    }
    const QPoint buttonPos = m_paletteButton->mapTo(this, QPoint(0, 0));
    const int gap = 4;
    const int width = m_paletteOptions->width();
    const int x = buttonPos.x() - width - gap;
    const int y = buttonPos.y() + (m_paletteButton->height() - m_paletteOptions->height()) / 2;
    m_paletteOptions->move(x, y);
}

void CImageViewer::configureWindowLevelControls()
{
    if (!m_wlSlidersPanel || !m_centerSlider || !m_widthSlider)
    {
        return;
    }

    if (!m_dicomImage || !m_dicomImage->isValid())
    {
        m_wlSlidersPanel->setVisible(false);
        return;
    }

    const auto pi = m_dicomImage->photometricInterpretation();
    if (pi == DicomViewer::EPhotometricInterpretation::Rgb ||
        pi == DicomViewer::EPhotometricInterpretation::PaletteColor)
    {
        m_wlSlidersPanel->setVisible(false);
        return;
    }

    // Calculate range based on image bit depth
    const auto dims = m_dicomImage->dimensions();
    const int bitsStored = dims.bitsStored > 0 ? dims.bitsStored : 8;
    const int maxPixelValue = (1 << bitsStored) - 1;

    // For signed images, center can be negative
    if (dims.isSigned)
    {
        m_windowCenterMin = -(1 << (bitsStored - 1));
        m_windowCenterMax = (1 << (bitsStored - 1)) - 1;
    }
    else
    {
        m_windowCenterMin = 0;
        m_windowCenterMax = maxPixelValue;
    }

    m_windowWidthMin = DicomViewer::kMinWindowWidth;
    m_windowWidthMax = maxPixelValue + 1;

    {
        QSignalBlocker blockCenter(m_centerSlider);
        QSignalBlocker blockWidth(m_widthSlider);
        m_centerSlider->setRange(m_windowCenterMin, m_windowCenterMax);
        m_widthSlider->setRange(m_windowWidthMin, m_windowWidthMax);
    }

    m_wlSlidersPanel->setVisible(true);
    m_wlSlidersOpen = true;
    if (auto *layout = m_wlSlidersPanel->layout())
    {
        layout->activate();
    }
    m_wlSlidersPanel->adjustSize();
    m_wlSlidersPanel->setMaximumWidth(QWIDGETSIZE_MAX);
    m_wlSlidersPanel->setFixedWidth(m_wlSlidersPanel->sizeHint().width());
    syncWindowLevelControls();
    positionWLControls();
}

void CImageViewer::syncWindowLevelControls()
{
    if (!m_centerSlider || !m_widthSlider || !m_dicomImage)
    {
        return;
    }

    const auto wl = m_dicomImage->windowLevel();
    const int centerValue = std::clamp(
        static_cast<int>(std::round(wl.center)),
        m_windowCenterMin,
        m_windowCenterMax);
    const int widthValue = std::clamp(
        static_cast<int>(std::round(wl.width)),
        m_windowWidthMin,
        m_windowWidthMax);

    QSignalBlocker blockCenter(m_centerSlider);
    QSignalBlocker blockWidth(m_widthSlider);
    m_centerSlider->setValue(centerValue);
    m_widthSlider->setValue(widthValue);

    if (m_centerValueLabel)
    {
        m_centerValueLabel->setText(QString::number(centerValue));
    }
    if (m_widthValueLabel)
    {
        m_widthValueLabel->setText(QString::number(widthValue));
    }
}

bool CImageViewer::hasImage() const
{
    return m_dicomImage && m_dicomImage->isValid();
}

QSize CImageViewer::imagePixelSize() const
{
    if (!hasImage())
    {
        return {};
    }
    const auto dims = m_dicomImage->dimensions();
    return QSize(static_cast<int>(dims.width), static_cast<int>(dims.height));
}

void CImageViewer::ensureGlResources()
{
    if (!m_shaderProgram)
    {
        m_shaderProgram = new QOpenGLShaderProgram(this);
        const bool vertexOk =
            m_shaderProgram->addShaderFromSourceCode(QOpenGLShader::Vertex, kVertexShaderSource);
        if (!vertexOk)
        {
            DICOMVIEWER_ERROR("Vertex shader compile failed:" << m_shaderProgram->log());
        }
        const bool fragOk =
            m_shaderProgram->addShaderFromSourceCode(QOpenGLShader::Fragment, kFragmentShaderSource);
        if (!fragOk)
        {
            DICOMVIEWER_ERROR("Fragment shader compile failed:" << m_shaderProgram->log());
        }
        if (vertexOk && fragOk && !m_shaderProgram->link())
        {
            DICOMVIEWER_ERROR("Shader link failed:" << m_shaderProgram->log());
        }
        if (!vertexOk || !fragOk || !m_shaderProgram->isLinked())
        {
            delete m_shaderProgram;
            m_shaderProgram = nullptr;
            m_useCpuFallback = true;
            return;
        }
    }

    if (!m_vao)
    {
        m_vao = new QOpenGLVertexArrayObject(this);
        m_vao->create();
    }

    if (!m_vbo)
    {
        m_vbo = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
        m_vbo->create();
    }

    if (m_shaderProgram && m_vao && m_vbo && m_vao->isCreated() && m_vbo->isCreated())
    {
        m_shaderProgram->bind();
        m_vao->bind();
        m_vbo->bind();
        if (m_vbo->size() <= 0)
        {
            m_vbo->allocate(sizeof(SQuadVertex) * 4);
        }

        constexpr int kPosLoc = 0;
        constexpr int kUvLoc = 1;
        m_shaderProgram->enableAttributeArray(kPosLoc);
        m_shaderProgram->setAttributeBuffer(kPosLoc, GL_FLOAT, 0, 2, sizeof(SQuadVertex));
        m_shaderProgram->enableAttributeArray(kUvLoc);
        m_shaderProgram->setAttributeBuffer(kUvLoc, GL_FLOAT, sizeof(float) * 2, 2,
                                            sizeof(SQuadVertex));

        m_vbo->release();
        m_vao->release();
        m_shaderProgram->release();
    }
}

void CImageViewer::updateGeometry()
{
    if (!m_vbo || !hasImage())
    {
        return;
    }

    const QSize imageSize = imagePixelSize();
    if (imageSize.isEmpty())
    {
        return;
    }

    SQuadVertex vertices[4] = {
        {0.0f, 0.0f, 0.0f, 0.0f},
        {static_cast<float>(imageSize.width()), 0.0f, 1.0f, 0.0f},
        {0.0f, static_cast<float>(imageSize.height()), 0.0f, 1.0f},
        {static_cast<float>(imageSize.width()),
         static_cast<float>(imageSize.height()), 1.0f, 1.0f}};

    m_vbo->bind();
    if (m_vbo->size() <= 0)
    {
        m_vbo->allocate(sizeof(vertices));
    }
    m_vbo->write(0, vertices, sizeof(vertices));
    m_vbo->release();
    m_verticesDirty = false;
}

void CImageViewer::uploadTexture()
{
    if (!hasImage())
    {
        return;
    }

    const auto dims = m_dicomImage->dimensions();
    const auto &pixelData = m_dicomImage->pixelData();
    const size_t pixelCount = static_cast<size_t>(dims.width) * dims.height;
    if (pixelCount == 0)
    {
        return;
    }

    if (m_texture)
    {
        delete m_texture;
        m_texture = nullptr;
    }

    m_textureIsRgb = (dims.samplesPerPixel == 3);
    const bool is16bit = (m_dicomImage->bitsPerSample() == 16);
    const bool isSigned = m_dicomImage->isPixelSigned();
    DICOMVIEWER_LOG("Upload texture - pixel bytes:" << pixelData.size()
                                                    << "expected:" << (pixelCount * (is16bit ? 2 : 1)));

    QOpenGLPixelTransferOptions pixelOpts;
    pixelOpts.setAlignment(1);
    pixelOpts.setRowLength(static_cast<int>(dims.width));

    if (m_textureIsRgb)
    {
        m_texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
        m_texture->create();
        m_texture->setSize(static_cast<int>(dims.width), static_cast<int>(dims.height));
        m_texture->setFormat(QOpenGLTexture::RGB8_UNorm);
        m_texture->allocateStorage(QOpenGLTexture::RGB, QOpenGLTexture::UInt8);
        m_texture->setMinificationFilter(QOpenGLTexture::Linear);
        m_texture->setMagnificationFilter(QOpenGLTexture::Linear);
        m_texture->setWrapMode(QOpenGLTexture::ClampToEdge);
        m_texture->setData(QOpenGLTexture::RGB, QOpenGLTexture::UInt8, pixelData.data(), &pixelOpts);
        m_textureValueMin = 0;
        m_textureValueMax = 255;
        DICOMVIEWER_LOG("GL texture upload RGB"
                        << dims.width << "x" << dims.height
                        << "bytes:" << pixelData.size());
    }
    else
    {
        m_texture = new QOpenGLTexture(QOpenGLTexture::Target2D);
        m_texture->create();
        m_texture->setSize(static_cast<int>(dims.width), static_cast<int>(dims.height));

        if (is16bit)
        {
            m_texture->setFormat(QOpenGLTexture::R16_UNorm);
            m_texture->allocateStorage(QOpenGLTexture::Red, QOpenGLTexture::UInt16);
            m_texture->setMinificationFilter(QOpenGLTexture::Linear);
            m_texture->setMagnificationFilter(QOpenGLTexture::Linear);
            m_texture->setWrapMode(QOpenGLTexture::ClampToEdge);

            if (isSigned)
            {
                std::vector<uint16_t> converted(pixelCount);
                const auto *src = reinterpret_cast<const int16_t *>(pixelData.data());
                int16_t minVal = std::numeric_limits<int16_t>::max();
                int16_t maxVal = std::numeric_limits<int16_t>::min();
                for (size_t i = 0; i < pixelCount; ++i)
                {
                    minVal = std::min(minVal, src[i]);
                    maxVal = std::max(maxVal, src[i]);
                    converted[i] = static_cast<uint16_t>(static_cast<int>(src[i]) + 32768);
                }
                m_texture->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt16,
                                   converted.data(), &pixelOpts);
                m_textureValueMin = -32768;
                m_textureValueMax = 32767;
                DICOMVIEWER_LOG("GL texture upload R16 signed"
                                << dims.width << "x" << dims.height);
                DICOMVIEWER_LOG("Pixel range signed:" << minVal << "to" << maxVal);
            }
            else
            {
                const auto *src = reinterpret_cast<const uint16_t *>(pixelData.data());
                uint16_t minVal = std::numeric_limits<uint16_t>::max();
                uint16_t maxVal = std::numeric_limits<uint16_t>::min();
                for (size_t i = 0; i < pixelCount; ++i)
                {
                    minVal = std::min(minVal, src[i]);
                    maxVal = std::max(maxVal, src[i]);
                }
                m_texture->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt16,
                                   pixelData.data(), &pixelOpts);
                m_textureValueMin = 0;
                m_textureValueMax = 65535;
                DICOMVIEWER_LOG("GL texture upload R16 unsigned"
                                << dims.width << "x" << dims.height);
                DICOMVIEWER_LOG("Pixel range unsigned:" << minVal << "to" << maxVal);
            }
        }
        else
        {
            m_texture->setFormat(QOpenGLTexture::R8_UNorm);
            m_texture->allocateStorage(QOpenGLTexture::Red, QOpenGLTexture::UInt8);
            m_texture->setMinificationFilter(QOpenGLTexture::Linear);
            m_texture->setMagnificationFilter(QOpenGLTexture::Linear);
            m_texture->setWrapMode(QOpenGLTexture::ClampToEdge);
            m_texture->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, pixelData.data(),
                               &pixelOpts);
            m_textureValueMin = 0;
            m_textureValueMax = 255;
            DICOMVIEWER_LOG("GL texture upload R8"
                            << dims.width << "x" << dims.height);
            const auto *src = pixelData.data();
            uint8_t minVal = std::numeric_limits<uint8_t>::max();
            uint8_t maxVal = std::numeric_limits<uint8_t>::min();
            for (size_t i = 0; i < pixelCount; ++i)
            {
                minVal = std::min(minVal, src[i]);
                maxVal = std::max(maxVal, src[i]);
            }
            DICOMVIEWER_LOG("Pixel range 8-bit:" << minVal << "to" << maxVal);
        }
    }

    if (m_texture)
    {
        DICOMVIEWER_LOG("Texture created:" << m_texture->isCreated()
                                           << "size:" << m_texture->width() << "x" << m_texture->height());
    }

    m_textureDirty = false;
}

void CImageViewer::uploadPalette()
{
    const auto &palette = m_converter.palette();
    if (palette.type() == DicomViewer::EPaletteType::Grayscale)
    {
        if (m_paletteTexture)
        {
            delete m_paletteTexture;
            m_paletteTexture = nullptr;
        }
        m_paletteDirty = false;
        return;
    }

    if (m_paletteTexture)
    {
        delete m_paletteTexture;
        m_paletteTexture = nullptr;
    }

    std::array<uint8_t, 256 * 3> lut{};
    for (int i = 0; i < 256; ++i)
    {
        auto rgb = palette.mapRgb(static_cast<uint8_t>(i));
        lut[i * 3 + 0] = rgb[0];
        lut[i * 3 + 1] = rgb[1];
        lut[i * 3 + 2] = rgb[2];
    }

    QOpenGLPixelTransferOptions pixelOpts;
    pixelOpts.setAlignment(1);
    pixelOpts.setRowLength(256);

    m_paletteTexture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    m_paletteTexture->setSize(256, 1);
    m_paletteTexture->setFormat(QOpenGLTexture::RGB8_UNorm);
    m_paletteTexture->allocateStorage(QOpenGLTexture::RGB, QOpenGLTexture::UInt8);
    m_paletteTexture->setMinificationFilter(QOpenGLTexture::Linear);
    m_paletteTexture->setMagnificationFilter(QOpenGLTexture::Linear);
    m_paletteTexture->setWrapMode(QOpenGLTexture::ClampToEdge);
    m_paletteTexture->setData(QOpenGLTexture::RGB, QOpenGLTexture::UInt8, lut.data(), &pixelOpts);

    m_paletteDirty = false;
}

bool CImageViewer::eventFilter(QObject *obj, QEvent *event)
{
    Q_UNUSED(obj);
    if (!m_paletteOpen || !m_paletteButton)
    {
        return QOpenGLWidget::eventFilter(obj, event);
    }

    if (event->type() == QEvent::MouseButtonPress)
    {
        auto *mouseEvent = static_cast<QMouseEvent *>(event);
        const QPoint globalPos = mouseEvent->globalPosition().toPoint();
        const QRect buttonRect = QRect(m_paletteButton->mapToGlobal(QPoint(0, 0)), m_paletteButton->size());
        QRect optionsRect;
        if (m_paletteOptions && m_paletteOptions->isVisible())
        {
            optionsRect = QRect(m_paletteOptions->mapToGlobal(QPoint(0, 0)), m_paletteOptions->size());
        }
        if (!buttonRect.contains(globalPos) && !optionsRect.contains(globalPos))
        {
            setPaletteOpen(false);
        }
    }

    return QOpenGLWidget::eventFilter(obj, event);
}

void CImageViewer::dragEnterEvent(QDragEnterEvent *event)
{
    const QMimeData *mime = event->mimeData();
    if (mime && mime->hasUrls())
    {
        for (const QUrl &url : mime->urls())
        {
            const QString path = url.toLocalFile();
            if (!path.isEmpty() && QFileInfo(path).isFile())
            {
                m_isDragActive = true;
                event->acceptProposedAction();
                update();
                return;
            }
        }
    }
    event->ignore();
}

void CImageViewer::dragMoveEvent(QDragMoveEvent *event)
{
    event->acceptProposedAction();
}

void CImageViewer::dragLeaveEvent(QDragLeaveEvent *event)
{
    Q_UNUSED(event);
    if (m_isDragActive)
    {
        m_isDragActive = false;
        update();
    }
}

void CImageViewer::dropEvent(QDropEvent *event)
{
    QStringList paths;
    const QMimeData *mime = event->mimeData();
    if (mime && mime->hasUrls())
    {
        for (const QUrl &url : mime->urls())
        {
            const QString path = url.toLocalFile();
            if (!path.isEmpty() && QFileInfo(path).isFile())
            {
                paths.append(path);
            }
        }
    }

    if (!paths.isEmpty())
    {
        emit filesDropped(paths);
        event->acceptProposedAction();
    }
    else
    {
        event->ignore();
    }

    if (m_isDragActive)
    {
        m_isDragActive = false;
        update();
    }
}

void CImageViewer::setPaletteOpen(bool open)
{
    if (!m_paletteOptions || m_paletteOpen == open)
    {
        return;
    }
    m_paletteOpen = open;

    if (m_paletteButton)
    {
        m_paletteButton->setProperty("open", open);
        m_paletteButton->style()->unpolish(m_paletteButton);
        m_paletteButton->style()->polish(m_paletteButton);
    }

    if (m_paletteExpandAnim)
    {
        m_paletteExpandAnim->stop();
        m_paletteExpandAnim->deleteLater();
    }

    m_paletteOptions->setVisible(true);
    m_paletteOptions->raise();
    if (auto *layout = m_paletteOptions->layout())
    {
        layout->activate();
    }
    m_paletteOptions->adjustSize();
    const int targetWidth = m_paletteOptions->sizeHint().width();
    const int targetHeight = m_paletteButton ? m_paletteButton->height() : m_paletteOptions->sizeHint().height();
    m_paletteOptions->setFixedHeight(targetHeight);
    const int startWidth = open ? 0 : m_paletteOptions->width();
    const int endWidth = open ? targetWidth : 0;

    m_paletteExpandAnim = new QPropertyAnimation(m_paletteOptions, "maximumWidth", this);
    m_paletteExpandAnim->setDuration(160);
    m_paletteExpandAnim->setStartValue(startWidth);
    m_paletteExpandAnim->setEndValue(endWidth);
    m_paletteExpandAnim->setEasingCurve(QEasingCurve::OutCubic);
    connect(m_paletteExpandAnim, &QPropertyAnimation::valueChanged, this, [this, open]()
            {
        if (m_paletteOptions) {
            const int currentWidth = m_paletteOptions->maximumWidth();
            m_paletteOptions->setFixedWidth(currentWidth);
            // Hide early when collapsing to avoid border artifact
            if (!open && currentWidth < 5) {
                m_paletteOptions->setVisible(false);
            }
        }
        positionPaletteOptions(); });
    connect(m_paletteExpandAnim, &QPropertyAnimation::finished, this, [this, open]()
            {
        if (m_paletteExpandAnim) {
            m_paletteExpandAnim->deleteLater();
        }
        if (!open && m_paletteOptions) {
            m_paletteOptions->setVisible(false);
        } });

    m_paletteExpandAnim->start();
    connect(m_paletteExpandAnim, &QPropertyAnimation::valueChanged, this, [this]()
            { update(); });
    connect(m_paletteExpandAnim, &QPropertyAnimation::finished, this, [this]()
            { update(); });
}

int CImageViewer::paletteIndex(DicomViewer::EPaletteType type) const
{
    for (int i = 0; i < m_paletteOptionTypes.size(); ++i)
    {
        if (m_paletteOptionTypes.at(i) == type)
        {
            return i;
        }
    }
    return -1;
}
