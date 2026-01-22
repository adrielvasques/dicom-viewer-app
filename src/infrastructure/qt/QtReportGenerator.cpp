/**
 * @file QtReportGenerator.cpp
 * @brief Implementation of QtReportGenerator
 * @date 2026
 */

#include "QtReportGenerator.h"

#include <QColor>
#include <QDateTime>
#include <QFontMetrics>
#include <QImage>
#include <QPageLayout>
#include <QPainter>
#include <QPdfWriter>
#include <QPen>
#include <QRectF>
#include <QTextDocument>
#include <QVector>

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

bool QtReportGenerator::generate(const SReportData &data, std::string &errorMessage) const
{
    if (!data.dicomImage || !data.dicomImage->isValid())
    {
        errorMessage = "No image loaded to generate report.";
        return false;
    }

    if (data.filePath.empty())
    {
        errorMessage = "Invalid report file path.";
        return false;
    }

    QPdfWriter writer(QString::fromStdString(data.filePath));
    writer.setPageSize(QPageSize(QPageSize::A4));
    writer.setPageOrientation(QPageLayout::Portrait);
    writer.setResolution(72);

    QPainter painter(&writer);
    if (!painter.isActive())
    {
        errorMessage = "Failed to create PDF file.";
        return false;
    }

    const QRect pageRect = painter.viewport();
    const int margin = 50;
    const int contentWidth = pageRect.width() - (2 * margin);
    const int pageHeight = pageRect.height();

    int yPos = margin;

    QFont titleFont("Helvetica", 22, QFont::Bold);
    painter.setFont(titleFont);
    painter.setPen(QColor("#1a365d"));
    QFontMetrics titleMetrics(titleFont);
    painter.drawText(QRect(margin, yPos, contentWidth, titleMetrics.height() + 10),
                     Qt::AlignHCenter | Qt::AlignVCenter,
                     "DICOM Image Report");
    yPos += titleMetrics.height() + 20;

    QFont subtitleFont("Helvetica", 9);
    painter.setFont(subtitleFont);
    painter.setPen(QColor("#718096"));
    QString dateStr = QDateTime::currentDateTime().toString("dd/MM/yyyy - hh:mm");
    QFontMetrics subMetrics(subtitleFont);
    painter.drawText(QRect(margin, yPos, contentWidth, subMetrics.height() + 4),
                     Qt::AlignHCenter | Qt::AlignVCenter,
                     QString("Generated: %1").arg(dateStr));
    yPos += subMetrics.height() + 25;

    painter.setPen(QPen(QColor("#e2e8f0"), 1));
    painter.drawLine(margin, yPos, margin + contentWidth, yPos);
    yPos += 25;

    const int availableImageHeight = (pageHeight - yPos - margin - 300);
    const int maxImageWidth = contentWidth - 20;
    const int maxImageHeight = std::min(availableImageHeight, 320);
    QImage reportImage = bufferToImage(data.image);
    QSize scaledSize = reportImage.size().scaled(maxImageWidth, maxImageHeight, Qt::KeepAspectRatio);

    const int imageX = margin + (contentWidth - scaledSize.width()) / 2;
    const int framePadding = 8;
    QRect frameRect(imageX - framePadding, yPos - framePadding,
                    scaledSize.width() + (2 * framePadding),
                    scaledSize.height() + (2 * framePadding));

    painter.setPen(QPen(QColor("#cbd5e0"), 1));
    painter.setBrush(QColor("#f7fafc"));
    painter.drawRoundedRect(frameRect, 4, 4);
    painter.drawImage(QRect(imageX, yPos, scaledSize.width(), scaledSize.height()), reportImage);
    yPos += scaledSize.height() + framePadding + 30;

    QFont sectionFont("Helvetica", 12, QFont::Bold);
    painter.setFont(sectionFont);
    painter.setPen(QColor("#1a365d"));
    painter.setBrush(Qt::NoBrush);
    QFontMetrics sectionMetrics(sectionFont);
    painter.drawText(QRect(margin, yPos, contentWidth, sectionMetrics.height() + 6),
                     Qt::AlignLeft | Qt::AlignVCenter,
                     "Patient & Study Information");
    yPos += sectionMetrics.height() + 15;

    QFont labelFont("Helvetica", 9, QFont::Bold);
    QFont valueFont("Helvetica", 9);
    QFontMetrics tableMetrics(valueFont);

    const int colWidth = contentWidth / 2;
    const int rowHeight = tableMetrics.height() + 12;
    const int cellPadding = 8;

    const auto *metadata = data.dicomImage->metadata();
    const auto dims = data.dicomImage->dimensions();
    const auto wl = data.dicomImage->windowLevel();

    struct SRowData
    {
        QString label;
        QString value;
    };

    QVector<SRowData> leftColumn;
    QVector<SRowData> rightColumn;

    if (metadata)
    {
        auto toValue = [](const std::string &value)
        {
            return value.empty() ? QString("-") : QString::fromStdString(value);
        };

        leftColumn.append({QString("Patient Name"), toValue(metadata->patientName())});
        leftColumn.append({QString("Patient ID"), toValue(metadata->patientId())});
        leftColumn.append({QString("Birth Date"), toValue(metadata->patientBirthDate())});
        leftColumn.append({QString("Sex"), toValue(metadata->patientSex())});
        leftColumn.append({QString("Study Date"), toValue(metadata->studyDate())});
        leftColumn.append({QString("Study"), toValue(metadata->studyDescription())});

        rightColumn.append({QString("Series"), toValue(metadata->seriesDescription())});
        rightColumn.append({QString("Modality"), toValue(metadata->modality())});
    }

    rightColumn.append({QString("Dimensions"), QString("%1 x %2 px").arg(dims.width).arg(dims.height)});
    rightColumn.append({QString("Bit Depth"), QString("%1-bit").arg(data.dicomImage->bitsPerSample())});
    rightColumn.append({QString("Window Center"), QString::number(static_cast<int>(wl.center))});
    rightColumn.append({QString("Window Width"), QString::number(static_cast<int>(wl.width))});

    const int numRows = std::max(leftColumn.size(), rightColumn.size());
    const int tableHeight = numRows * rowHeight;

    painter.setPen(QPen(QColor("#e2e8f0"), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(margin, yPos, contentWidth, tableHeight, 4, 4);
    painter.drawLine(margin + colWidth, yPos, margin + colWidth, yPos + tableHeight);

    for (int i = 0; i < numRows; ++i)
    {
        const int rowY = yPos + (i * rowHeight);

        if (i % 2 == 1)
        {
            painter.fillRect(margin + 1, rowY, colWidth - 1, rowHeight, QColor("#f7fafc"));
            painter.fillRect(margin + colWidth + 1, rowY, colWidth - 2, rowHeight, QColor("#f7fafc"));
        }

        if (i > 0)
        {
            painter.setPen(QPen(QColor("#e2e8f0"), 1));
            painter.drawLine(margin, rowY, margin + contentWidth, rowY);
        }

        if (i < leftColumn.size())
        {
            const auto &row = leftColumn[i];
            painter.setFont(labelFont);
            painter.setPen(QColor("#4a5568"));
            painter.drawText(QRect(margin + cellPadding, rowY, colWidth / 2 - cellPadding, rowHeight),
                             Qt::AlignLeft | Qt::AlignVCenter, row.label + ":");

            painter.setFont(valueFont);
            painter.setPen(QColor("#1a202c"));
            painter.drawText(QRect(margin + colWidth / 2, rowY, colWidth / 2 - cellPadding, rowHeight),
                             Qt::AlignLeft | Qt::AlignVCenter, row.value);
        }

        if (i < rightColumn.size())
        {
            const auto &row = rightColumn[i];
            painter.setFont(labelFont);
            painter.setPen(QColor("#4a5568"));
            painter.drawText(QRect(margin + colWidth + cellPadding, rowY, colWidth / 2 - cellPadding, rowHeight),
                             Qt::AlignLeft | Qt::AlignVCenter, row.label + ":");

            painter.setFont(valueFont);
            painter.setPen(QColor("#1a202c"));
            painter.drawText(QRect(margin + colWidth + colWidth / 2, rowY, colWidth / 2 - cellPadding, rowHeight),
                             Qt::AlignLeft | Qt::AlignVCenter, row.value);
        }
    }

    yPos += tableHeight + 25;

    if (!data.comment.empty())
    {
        QFont sectionTitleFont("Helvetica", 12, QFont::Bold);
        painter.setFont(sectionTitleFont);
        painter.setPen(QColor("#1a365d"));
        QFontMetrics commentTitleMetrics(sectionTitleFont);

        const int titleHeight = commentTitleMetrics.height() + 6;
        const int footerReserve = margin + 40;

        QTextDocument commentDoc;
        QFont commentFont("Helvetica", 9);
        commentDoc.setDefaultFont(commentFont);
        commentDoc.setPlainText(QString::fromStdString(data.comment));
        commentDoc.setTextWidth(contentWidth - (2 * cellPadding));
        const int textHeight = static_cast<int>(commentDoc.size().height());
        const int commentHeight = textHeight + (2 * cellPadding);

        if (yPos + titleHeight + commentHeight + footerReserve > pageHeight)
        {
            writer.newPage();
            yPos = margin;
        }

        painter.drawText(QRect(margin, yPos, contentWidth, titleHeight),
                         Qt::AlignLeft | Qt::AlignVCenter,
                         "Comments");
        yPos += titleHeight + 8;

        QRect commentRect(margin, yPos, contentWidth, commentHeight);
        painter.setPen(QPen(QColor("#e2e8f0"), 1));
        painter.setBrush(QColor("#f7fafc"));
        painter.drawRoundedRect(commentRect, 4, 4);

        painter.save();
        painter.translate(margin + cellPadding, yPos + cellPadding);
        QRectF textRect(0, 0, contentWidth - (2 * cellPadding), textHeight);
        commentDoc.drawContents(&painter, textRect);
        painter.restore();

        yPos += commentHeight + 20;
    }

    QFont footerFont("Helvetica", 8);
    painter.setFont(footerFont);
    painter.setPen(QColor("#a0aec0"));
    painter.drawText(QRect(margin, pageHeight - margin - 20, contentWidth, 20),
                     Qt::AlignCenter,
                     "DICOM Viewer • Medical Image Report");

    painter.end();
    return true;
}
