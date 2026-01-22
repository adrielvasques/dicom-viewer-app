/**
 * @file main.cpp
 * @brief Application entry point (premium splash, Wayland-safe)
 * @date 2026
 *
 * Uses a custom QWidget splash (NOT QSplashScreen) to avoid Wayland compositors
 * placing the splash at (0,0). Includes fade-in / fade-out and full logging.
 */

#include <QApplication>
#include <QCursor>
#include <QFile>
#include <QGraphicsOpacityEffect>
#include <QGuiApplication>
#include <QIcon>
#include <QLabel>
#include <QPalette>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QScreen>
#include <QStyle>
#include <QSurfaceFormat>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <QWindow>
#include <memory>

#include "infrastructure/dcmtk/DcmtkDicomLoader.h"
#include "infrastructure/qt/QtImageRenderer.h"
#include "infrastructure/qt/QtReportGenerator.h"
#include "presentation/viewmodels/MainViewModel.h"
#include "ui/CMainWindow.h"
#include <DicomViewer/Debug.h>

static void logPixmapInfo(const char *tag, const QPixmap &p)
{
    DICOMVIEWER_LOG(tag
                    << " size(logical):" << p.size().width() << "x" << p.size().height()
                    << " DPR:" << p.devicePixelRatio()
                    << " size(physical):"
                    << int(p.size().width() * p.devicePixelRatio()) << "x"
                    << int(p.size().height() * p.devicePixelRatio()));
}

int main(int argc, char *argv[])
{
    QSurfaceFormat format;
    format.setRenderableType(QSurfaceFormat::OpenGL);
    format.setVersion(3, 3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setSwapBehavior(QSurfaceFormat::DoubleBuffer);
    QSurfaceFormat::setDefaultFormat(format);

    QApplication app(argc, argv);

    app.setApplicationName("DICOM Viewer");
    app.setOrganizationName("DicomViewer");
    app.setApplicationVersion("1.0.0");
    app.setDesktopFileName("dicom-visualizer");
    QIcon appIcon(":/images/app-icon.png");
    app.setWindowIcon(appIcon);

    QFile themeFile(":/themes/clean-medical.qss");
    if (themeFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        app.setStyleSheet(QString::fromUtf8(themeFile.readAll()));
    }
    else
    {
        DICOMVIEWER_WARN("Failed to load theme: clean-medical.qss");
    }

    DICOMVIEWER_LOG("Platform name:" << QGuiApplication::platformName());

    QScreen *screen = QGuiApplication::primaryScreen();
    if (screen)
    {
        DICOMVIEWER_LOG("Primary screen name:" << screen->name());
        DICOMVIEWER_LOG("Screen geometry:"
                        << screen->geometry().x() << "," << screen->geometry().y()
                        << screen->geometry().width() << "x" << screen->geometry().height());
        DICOMVIEWER_LOG("Screen available geometry:"
                        << screen->availableGeometry().width() << "x" << screen->availableGeometry().height());
        DICOMVIEWER_LOG("Screen DPR:" << screen->devicePixelRatio());
    }
    else
    {
        DICOMVIEWER_WARN("No primary screen detected!");
    }

    QPixmap splashPixmap(":/images/splash-screen.png");
    if (splashPixmap.isNull())
    {
        DICOMVIEWER_ERROR("ERROR: :/images/splash-screen.png not found!");
    }

    logPixmapInfo("Original pixmap", splashPixmap);

    QPixmap scaledPixmap = splashPixmap.scaled(
        600, 340,
        Qt::KeepAspectRatio,
        Qt::SmoothTransformation);

    logPixmapInfo("Scaled pixmap", scaledPixmap);

    auto loader = std::make_unique<DcmtkDicomLoader>();
    auto renderer = std::make_unique<QtImageRenderer>();
    auto reportGenerator = std::make_unique<QtReportGenerator>();
    auto viewModel = std::make_shared<MainViewModel>(std::move(loader),
                                                     std::move(renderer),
                                                     std::move(reportGenerator));

    CMainWindow mainWindow(viewModel);
    mainWindow.setWindowTitle(app.applicationName());
    mainWindow.setWindowIcon(appIcon);
    const QSize finalWindowSize(1280, 800);
    mainWindow.resize(finalWindowSize); // Set final size from the start
    DICOMVIEWER_LOG("Using standalone splash window");

    QScreen *targetScreen = QGuiApplication::screenAt(QCursor::pos());
    if (!targetScreen)
    {
        targetScreen = screen;
    }
    if (targetScreen)
    {
        DICOMVIEWER_LOG("Splash target screen:" << targetScreen->name());
    }

    if (targetScreen)
    {
        const QRect target = QStyle::alignedRect(
            Qt::LeftToRight,
            Qt::AlignCenter,
            finalWindowSize,
            targetScreen->availableGeometry());
        mainWindow.setGeometry(target);
    }

    QWidget splash;
    splash.setObjectName("DicomViewerSplash");
    splash.setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    splash.setAttribute(Qt::WA_TranslucentBackground);
    splash.setAttribute(Qt::WA_NoSystemBackground);
    splash.setAttribute(Qt::WA_ShowWithoutActivating);
    splash.setAutoFillBackground(false);
    splash.setStyleSheet("background: transparent;");
    {
        QPalette pal = splash.palette();
        pal.setColor(QPalette::Window, Qt::transparent);
        splash.setPalette(pal);
    }
    if (targetScreen)
    {
        splash.setGeometry(targetScreen->geometry());
    }

    auto *layout = new QVBoxLayout(&splash);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    auto *imageLabel = new QLabel(&splash);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setAttribute(Qt::WA_TranslucentBackground);
    imageLabel->setPixmap(scaledPixmap);

    layout->addStretch(1);
    layout->addWidget(imageLabel, 0, Qt::AlignCenter);
    layout->addStretch(1);

    auto *opacity = new QGraphicsOpacityEffect(&splash);
    splash.setGraphicsEffect(opacity);
    opacity->setOpacity(0.0);

    auto *fadeIn = new QPropertyAnimation(opacity, "opacity");
    fadeIn->setDuration(450);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    fadeIn->setEasingCurve(QEasingCurve::OutCubic);

    QTimer::singleShot(1400, [&]()
                       {
        auto* fadeOut = new QPropertyAnimation(splash.graphicsEffect(), "opacity", &splash);
        fadeOut->setDuration(350);
        fadeOut->setStartValue(1.0);
        fadeOut->setEndValue(0.0);
        fadeOut->setEasingCurve(QEasingCurve::InCubic);
        
        QObject::connect(fadeOut, &QPropertyAnimation::finished, [&]() {
            splash.close();
            mainWindow.showMaximized();
            mainWindow.raise();
            mainWindow.activateWindow();
            DICOMVIEWER_LOG("Main window shown");
        });
        
        fadeOut->start(QAbstractAnimation::DeleteWhenStopped); });

    splash.winId();
    if (targetScreen)
    {
        if (QWindow *handle = splash.windowHandle())
        {
            handle->setScreen(targetScreen);
        }
    }
    splash.show();
    splash.raise();

    app.processEvents();

    fadeIn->start(QAbstractAnimation::DeleteWhenStopped);

    return app.exec();
}
