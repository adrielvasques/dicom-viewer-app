/**
 * @file MainViewModel.h
 * @brief ViewModel for the main window (MVVM)
 * @date 2026
 */

#pragma once

#include "application/ports/IImageRenderer.h"
#include "application/ports/IReportGenerator.h"
#include "application/ports/IDicomLoader.h"
#include "utils/CColorPalette.h"

#include <QObject>
#include <QPointF>
#include <QString>
#include <QVector>
#include <memory>
#include <optional>

class MainViewModel : public QObject
{
    Q_OBJECT

  public:
    struct SViewState
    {
        double zoom = 1.0;
        QPointF pan;
        int rotation = 0;
    };

    struct SLoadedImage
    {
        QString filePath;
        std::shared_ptr<CDicomImage> image;
        DicomViewer::EPaletteType palette = DicomViewer::EPaletteType::Grayscale;
        DicomViewer::SWindowLevel windowLevel{};
        double zoom = 1.0;
        QPointF pan;
        int rotation = 0;
    };

    explicit MainViewModel(std::unique_ptr<IDicomLoader> loader,
                           std::unique_ptr<IImageRenderer> renderer,
                           std::unique_ptr<IReportGenerator> reportGenerator,
                           QObject *parent = nullptr);

    bool loadFile(const QString &filePath);
    void loadFiles(const QStringList &filePaths,
                   const SViewState &currentState,
                   const DicomViewer::SWindowLevel &currentWindowLevel);
    void selectImage(int index,
                     const SViewState &currentState,
                     const DicomViewer::SWindowLevel &currentWindowLevel);
    void removeImage(int index,
                     const SViewState &currentState,
                     const DicomViewer::SWindowLevel &currentWindowLevel);
    void updateCurrentViewState(double zoom, double panX, double panY, int rotation);
    void updateCurrentPalette(DicomViewer::EPaletteType type);
    void updateCurrentWindowLevel(double center, double width);

    int currentIndex() const;
    int imageCount() const;
    const QVector<SLoadedImage> &images() const;
    const SLoadedImage *currentEntry() const;
    const SLoadedImage *entryAt(int index) const;

    bool exportCurrentImage(const QString &filePath, const QString &format);
    bool exportCurrentImagePdf(const QString &filePath);
    bool generateReport(const QString &filePath, const QString &comment);

  signals:
    void errorOccurred(const QString &message);
    void statusMessage(const QString &message, int timeoutMs);
    void imageAdded(int index);
    void imageRemoved(int index);
    void currentImageChanged();
    void paletteUpdated(DicomViewer::EPaletteType palette);

  private:
    std::optional<DicomViewer::SWindowLevel> resolveWindowLevel(
        const SLoadedImage &entry) const;

    QVector<SLoadedImage> m_loadedImages;
    int m_currentImageIndex = -1;

    std::unique_ptr<IDicomLoader> m_loader;
    std::unique_ptr<IImageRenderer> m_renderer;
    std::unique_ptr<IReportGenerator> m_reportGenerator;
};
