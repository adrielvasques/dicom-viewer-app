/**
 * @file MainViewModel.cpp
 * @brief Implementation of MainViewModel
 * @date 2026
 */

#include "MainViewModel.h"

#include <QImage>
#include <QPageLayout>
#include <QPainter>
#include <QPdfWriter>
#include <algorithm>

MainViewModel::MainViewModel(std::unique_ptr<IDicomLoader> loader,
                             std::unique_ptr<IImageRenderer> renderer,
                             std::unique_ptr<IReportGenerator> reportGenerator,
                             QObject *parent)
    : QObject(parent),
      m_loader(std::move(loader)),
      m_renderer(std::move(renderer)),
      m_reportGenerator(std::move(reportGenerator))
{
}

namespace
{
    QImage bufferToImage(const SImageBuffer &buffer)
    {
        if (buffer.data.empty() || buffer.width <= 0 || buffer.height <= 0)
        {
            return QImage();
        }

        QImage::Format format = QImage::Format_RGB888;
        int bytesPerPixel = 3;
        switch (buffer.format)
        {
        case EPixelFormat::Grayscale8:
            format = QImage::Format_Grayscale8;
            bytesPerPixel = 1;
            break;
        case EPixelFormat::RGB24:
            format = QImage::Format_RGB888;
            bytesPerPixel = 3;
            break;
        case EPixelFormat::RGBA32:
            format = QImage::Format_RGBA8888;
            bytesPerPixel = 4;
            break;
        }

        const int bytesPerLine = buffer.bytesPerLine > 0
                                     ? buffer.bytesPerLine
                                     : buffer.width * bytesPerPixel;
        QImage image(buffer.data.data(), buffer.width, buffer.height, bytesPerLine, format);
        return image.copy();
    }
}

bool MainViewModel::loadFile(const QString &filePath)
{
    if (!m_loader)
    {
        emit errorOccurred("DICOM loader not configured.");
        return false;
    }

    SDicomLoadResult result = m_loader->load(filePath.toStdString());
    if (result.result != DicomViewer::ELoadResult::Success || !result.image)
    {
        emit errorOccurred(QString::fromStdString(result.errorMessage));
        return false;
    }

    result.image->resetWindowLevel();
    DicomViewer::SWindowLevel wl = result.image->windowLevel();
    m_loadedImages.push_back({filePath, result.image, DicomViewer::EPaletteType::Grayscale, wl});
    emit imageAdded(m_loadedImages.size() - 1);
    return true;
}

void MainViewModel::loadFiles(const QStringList &filePaths,
                              const SViewState &currentState,
                              const DicomViewer::SWindowLevel &currentWindowLevel)
{
    const int startIndex = m_loadedImages.size();
    for (const QString &path : filePaths)
    {
        loadFile(path);
    }

    if (m_loadedImages.size() > startIndex)
    {
        selectImage(startIndex, currentState, currentWindowLevel);
    }
}

void MainViewModel::selectImage(int index,
                                const SViewState &currentState,
                                const DicomViewer::SWindowLevel &currentWindowLevel)
{
    if (m_currentImageIndex >= 0 && m_currentImageIndex < m_loadedImages.size())
    {
        auto &currentEntry = m_loadedImages[m_currentImageIndex];
        currentEntry.zoom = currentState.zoom;
        currentEntry.pan = currentState.pan;
        currentEntry.rotation = currentState.rotation;
        if (currentEntry.image)
        {
            currentEntry.windowLevel = currentWindowLevel;
        }
    }

    if (index < 0 || index >= m_loadedImages.size())
    {
        m_currentImageIndex = -1;
        emit currentImageChanged();
        return;
    }

    if (index == m_currentImageIndex)
    {
        return;
    }

    m_currentImageIndex = index;
    emit currentImageChanged();
}

void MainViewModel::removeImage(int index,
                                const SViewState &currentState,
                                const DicomViewer::SWindowLevel &currentWindowLevel)
{
    if (index < 0 || index >= m_loadedImages.size())
    {
        return;
    }

    if (m_currentImageIndex >= 0 && m_currentImageIndex < m_loadedImages.size())
    {
        auto &currentEntry = m_loadedImages[m_currentImageIndex];
        currentEntry.zoom = currentState.zoom;
        currentEntry.pan = currentState.pan;
        currentEntry.rotation = currentState.rotation;
        if (currentEntry.image)
        {
            currentEntry.windowLevel = currentWindowLevel;
        }
    }

    m_loadedImages.removeAt(index);
    emit imageRemoved(index);

    if (m_loadedImages.isEmpty())
    {
        m_currentImageIndex = -1;
        emit currentImageChanged();
        return;
    }

    if (index == m_currentImageIndex)
    {
        const int lastIndex = static_cast<int>(m_loadedImages.size()) - 1;
        m_currentImageIndex = std::min(index, lastIndex);
        emit currentImageChanged();
        return;
    }

    if (index < m_currentImageIndex)
    {
        m_currentImageIndex--;
        emit currentImageChanged();
    }
}

void MainViewModel::updateCurrentViewState(double zoom, double panX, double panY, int rotation)
{
    if (m_currentImageIndex < 0 || m_currentImageIndex >= m_loadedImages.size())
    {
        return;
    }

    auto &entry = m_loadedImages[m_currentImageIndex];
    entry.zoom = zoom;
    entry.pan = QPointF(panX, panY);
    entry.rotation = rotation;
}

void MainViewModel::updateCurrentPalette(DicomViewer::EPaletteType type)
{
    if (m_currentImageIndex < 0 || m_currentImageIndex >= m_loadedImages.size())
    {
        return;
    }
    if (m_loadedImages[m_currentImageIndex].palette == type)
    {
        return;
    }
    m_loadedImages[m_currentImageIndex].palette = type;
    emit paletteUpdated(type);
}

void MainViewModel::updateCurrentWindowLevel(double center, double width)
{
    if (m_currentImageIndex < 0 || m_currentImageIndex >= m_loadedImages.size())
    {
        return;
    }

    auto &entry = m_loadedImages[m_currentImageIndex];
    entry.windowLevel.center = center;
    entry.windowLevel.width = width;
}

int MainViewModel::currentIndex() const
{
    return m_currentImageIndex;
}

int MainViewModel::imageCount() const
{
    return m_loadedImages.size();
}

const QVector<MainViewModel::SLoadedImage> &MainViewModel::images() const
{
    return m_loadedImages;
}

const MainViewModel::SLoadedImage *MainViewModel::currentEntry() const
{
    if (m_currentImageIndex < 0 || m_currentImageIndex >= m_loadedImages.size())
    {
        return nullptr;
    }
    return &m_loadedImages[m_currentImageIndex];
}

const MainViewModel::SLoadedImage *MainViewModel::entryAt(int index) const
{
    if (index < 0 || index >= m_loadedImages.size())
    {
        return nullptr;
    }
    return &m_loadedImages[index];
}

std::optional<DicomViewer::SWindowLevel> MainViewModel::resolveWindowLevel(
    const SLoadedImage &entry) const
{
    if (entry.windowLevel.width > 0.0)
    {
        return entry.windowLevel;
    }
    return std::nullopt;
}

bool MainViewModel::exportCurrentImage(const QString &filePath, const QString &format)
{
    const auto *entry = currentEntry();
    if (!entry || !entry->image)
    {
        emit errorOccurred("No image loaded to export.");
        return false;
    }

    if (!m_renderer)
    {
        emit errorOccurred("Image renderer not configured.");
        return false;
    }

    const SImageBuffer buffer = m_renderer->render(*entry->image,
                                                   entry->palette,
                                                   resolveWindowLevel(*entry));
    QImage exportImage = bufferToImage(buffer);
    if (exportImage.isNull())
    {
        emit errorOccurred("Failed to export image.");
        return false;
    }

    if (!exportImage.save(filePath, format.toUtf8().constData(), 95))
    {
        emit errorOccurred("Failed to export image.");
        return false;
    }

    emit statusMessage(QString("Exported to %1").arg(filePath), 5000);
    return true;
}

bool MainViewModel::exportCurrentImagePdf(const QString &filePath)
{
    const auto *entry = currentEntry();
    if (!entry || !entry->image)
    {
        emit errorOccurred("No image loaded to export.");
        return false;
    }

    if (!m_renderer)
    {
        emit errorOccurred("Image renderer not configured.");
        return false;
    }

    const SImageBuffer buffer = m_renderer->render(*entry->image,
                                                   entry->palette,
                                                   resolveWindowLevel(*entry));
    QImage exportImage = bufferToImage(buffer);
    if (exportImage.isNull())
    {
        emit errorOccurred("Failed to export image.");
        return false;
    }

    QPdfWriter writer(filePath);
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setPageOrientation(QPageLayout::Portrait);
    writer.setResolution(300);

    QPainter painter(&writer);
    if (!painter.isActive())
    {
        emit errorOccurred("Failed to create PDF file.");
        return false;
    }

    const QRect pageRect = painter.viewport();
    const QSize imageSize = exportImage.size().scaled(pageRect.size(), Qt::KeepAspectRatio);
    const int x = (pageRect.width() - imageSize.width()) / 2;
    const int y = (pageRect.height() - imageSize.height()) / 2;

    painter.drawImage(QRect(x, y, imageSize.width(), imageSize.height()), exportImage);
    painter.end();

    emit statusMessage(QString("Exported to %1").arg(filePath), 5000);
    return true;
}

bool MainViewModel::generateReport(const QString &filePath, const QString &comment)
{
    const auto *entry = currentEntry();
    if (!entry || !entry->image)
    {
        emit errorOccurred("No image loaded to generate report.");
        return false;
    }

    if (!m_renderer || !m_reportGenerator)
    {
        emit errorOccurred("Report generator not configured.");
        return false;
    }

    const SImageBuffer buffer = m_renderer->render(*entry->image,
                                                   entry->palette,
                                                   resolveWindowLevel(*entry));
    QImage exportImage = bufferToImage(buffer);
    if (exportImage.isNull())
    {
        emit errorOccurred("Failed to create report image.");
        return false;
    }

    SReportData reportData;
    reportData.filePath = filePath.toStdString();
    reportData.image = buffer;
    reportData.dicomImage = entry->image;
    reportData.comment = comment.toStdString();
    reportData.palette = entry->palette;
    reportData.windowLevel = resolveWindowLevel(*entry);

    std::string error;
    if (!m_reportGenerator->generate(reportData, error))
    {
        emit errorOccurred(QString::fromStdString(error));
        return false;
    }

    emit statusMessage(QString("Report generated: %1").arg(filePath), 5000);
    return true;
}
