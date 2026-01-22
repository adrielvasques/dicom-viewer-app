/**
 * @file QtImageRenderer.cpp
 * @brief Implementation of QtImageRenderer
 * @date 2026
 */

#include "QtImageRenderer.h"

#include <QImage>
#include <cstring>

namespace
{
    SImageBuffer toBuffer(const QImage &source)
    {
        SImageBuffer buffer;
        if (source.isNull())
        {
            return buffer;
        }

        QImage converted = source.convertToFormat(QImage::Format_RGB888);
        buffer.width = converted.width();
        buffer.height = converted.height();
        buffer.format = EPixelFormat::RGB24;
        const int bytesPerPixel = 3;
        buffer.bytesPerLine = buffer.width * bytesPerPixel;
        const int size = buffer.bytesPerLine * buffer.height;
        buffer.data.resize(static_cast<size_t>(size));
        for (int y = 0; y < buffer.height; ++y)
        {
            const auto *src = converted.constScanLine(y);
            auto *dst = buffer.data.data() + (y * buffer.bytesPerLine);
            std::memcpy(dst, src, static_cast<size_t>(buffer.bytesPerLine));
        }
        return buffer;
    }
}

SImageBuffer QtImageRenderer::render(const CDicomImage &image,
                                     DicomViewer::EPaletteType palette,
                                     const std::optional<DicomViewer::SWindowLevel> &windowLevel) const
{
    m_converter.setPalette(palette);
    QImage rendered = windowLevel.has_value()
                          ? m_converter.toQImage(image, windowLevel.value())
                          : m_converter.toQImage(image);
    return toBuffer(rendered);
}
