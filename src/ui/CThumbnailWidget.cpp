/**
 * @file CThumbnailWidget.cpp
 * @brief Implementation of CThumbnailWidget
 * @author DICOM Viewer Project
 * @date 2026
 */

#include "CThumbnailWidget.h"
#include "core/CDicomImage.h"
#include "utils/CImageConverter.h"

#include <QAbstractItemView>
#include <QFontMetrics>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QListWidget>
#include <QPainter>
#include <QPixmap>
#include <QResizeEvent>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>
#include <algorithm>

CThumbnailWidget::CThumbnailWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(160, 200);
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    setObjectName("ThumbnailWidget");

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(6, 6, 6, 6);
    layout->setSpacing(0);

    m_emptyLabel = new QLabel(tr("No images loaded"), this);
    m_emptyLabel->setObjectName("ThumbnailEmpty");
    m_emptyLabel->setAlignment(Qt::AlignCenter);

    m_listWidget = new QListWidget(this);
    m_listWidget->setObjectName("ThumbnailList");
    m_listWidget->setViewMode(QListView::IconMode);
    m_listWidget->setFlow(QListView::TopToBottom);
    m_listWidget->setWrapping(false);
    m_listWidget->setResizeMode(QListView::Adjust);
    m_listWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_listWidget->setMovement(QListView::Static);
    m_listWidget->setItemAlignment(Qt::AlignHCenter);
    m_listWidget->setIconSize(m_thumbnailSize);
    m_listWidget->setSpacing(8);
    m_listWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_listWidget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);

    layout->addWidget(m_emptyLabel);
    layout->addWidget(m_listWidget);

    updateEmptyState();
    updateLayoutSizes();

    connect(m_listWidget, &QListWidget::currentRowChanged,
            this, [this](int index)
            {
                updateSelectionState();
                emit imageSelected(index); });

    connect(m_listWidget, &QListWidget::itemClicked,
            this, [this](QListWidgetItem *item)
            {
                const int index = m_listWidget->row(item);
                if (index >= 0) {
                    emit imageSelected(index);
                } });
}

void CThumbnailWidget::addImage(const QString &label,
                                const QString &tooltip,
                                std::shared_ptr<CDicomImage> image)
{
    if (!image || !image->isValid())
    {
        return;
    }

    QImage thumbnail = buildThumbnail(*image, DicomViewer::EPaletteType::Grayscale);
    if (thumbnail.isNull())
    {
        return;
    }

    m_images.push_back(image);
    auto dims = image->dimensions();
    QString detailsText;
    if (dims.width > 0 && dims.height > 0)
    {
        detailsText = QString("%1 x %2").arg(dims.width).arg(dims.height);
    }

    auto *item = new QListWidgetItem;
    m_listWidget->addItem(item);
    QWidget *itemWidget = buildItemWidget(label, detailsText, tooltip, thumbnail, item);
    m_listWidget->setItemWidget(item, itemWidget);
    updateEmptyState();
    updateLayoutSizes();
}

void CThumbnailWidget::clearImages()
{
    m_listWidget->clear();
    m_images.clear();
    updateEmptyState();
    updateLayoutSizes();
}

void CThumbnailWidget::setCurrentIndex(int index)
{
    m_listWidget->setCurrentRow(index);
    updateSelectionState();
}

int CThumbnailWidget::currentIndex() const
{
    return m_listWidget->currentRow();
}

void CThumbnailWidget::removeImage(int index)
{
    if (index < 0 || index >= m_listWidget->count())
    {
        return;
    }

    if (index >= 0 && index < static_cast<int>(m_images.size()))
    {
        m_images.erase(m_images.begin() + index);
    }

    QListWidgetItem *item = m_listWidget->takeItem(index);
    if (item)
    {
        QWidget *itemWidget = m_listWidget->itemWidget(item);
        if (itemWidget)
        {
            m_listWidget->removeItemWidget(item);
            itemWidget->deleteLater();
        }
        delete item;
    }
    updateSelectionState();
    updateEmptyState();
    updateLayoutSizes();
}

void CThumbnailWidget::updateThumbnail(int index, DicomViewer::EPaletteType palette)
{
    if (index < 0 || index >= m_listWidget->count())
    {
        return;
    }
    if (index >= static_cast<int>(m_images.size()))
    {
        return;
    }

    const auto &image = m_images[static_cast<size_t>(index)];
    if (!image || !image->isValid())
    {
        return;
    }

    QImage thumbnail = buildThumbnail(*image, palette);
    if (thumbnail.isNull())
    {
        return;
    }

    setThumbnailImage(index, thumbnail);
}

void CThumbnailWidget::updateAllThumbnails(DicomViewer::EPaletteType palette)
{
    const int count = m_listWidget->count();
    for (int i = 0; i < count; ++i)
    {
        updateThumbnail(i, palette);
    }
}

void CThumbnailWidget::setThumbnailImage(int index, const QImage &thumbnail)
{
    if (index < 0 || index >= m_listWidget->count())
    {
        return;
    }
    if (thumbnail.isNull())
    {
        return;
    }

    QListWidgetItem *item = m_listWidget->item(index);
    if (!item)
    {
        return;
    }

    QWidget *row = m_listWidget->itemWidget(item);
    if (!row)
    {
        return;
    }

    auto *imageLabel = row->findChild<QLabel *>("ThumbnailImage");
    if (!imageLabel)
    {
        return;
    }

    const QImage scaled = thumbnail.scaled(
        m_thumbnailSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    imageLabel->setPixmap(QPixmap::fromImage(scaled));
}

QSize CThumbnailWidget::thumbnailSize() const
{
    return m_thumbnailSize;
}

QImage CThumbnailWidget::buildThumbnail(const CDicomImage &image, DicomViewer::EPaletteType palette) const
{
    if (!image.isValid())
    {
        return QImage();
    }

    CImageConverter converter;
    converter.setPalette(palette);
    QImage fullImage = converter.toQImage(image, image.windowLevel());
    if (fullImage.isNull())
    {
        return QImage();
    }

    return fullImage.scaled(m_thumbnailSize, Qt::KeepAspectRatio,
                            Qt::SmoothTransformation);
}

QWidget *CThumbnailWidget::buildItemWidget(const QString &label,
                                           const QString &details,
                                           const QString &tooltip,
                                           const QImage &thumbnail,
                                           QListWidgetItem *item)
{
    auto *row = new QWidget(m_listWidget);
    row->setObjectName("ThumbnailRow");
    auto *rowLayout = new QHBoxLayout(row);
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setAlignment(Qt::AlignHCenter);

    auto *card = new QFrame(row);
    card->setObjectName("ThumbnailCard");
    card->setFixedSize(cardSize());
    if (!tooltip.isEmpty())
    {
        card->setToolTip(tooltip);
    }

    auto *cardLayout = new QVBoxLayout(card);
    cardLayout->setContentsMargins(10, 10, 10, 8);
    cardLayout->setSpacing(6);

    auto *imageContainer = new QWidget(card);
    imageContainer->setObjectName("ThumbnailImageContainer");
    imageContainer->setFixedSize(m_thumbnailSize);
    auto *imageLayout = new QGridLayout(imageContainer);
    imageLayout->setContentsMargins(0, 0, 0, 0);

    auto *imageLabel = new QLabel(imageContainer);
    imageLabel->setObjectName("ThumbnailImage");
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setFixedSize(m_thumbnailSize);
    imageLabel->setPixmap(QPixmap::fromImage(thumbnail));
    imageLayout->addWidget(imageLabel, 0, 0, Qt::AlignCenter);

    auto *deleteButton = new QToolButton(imageContainer);
    deleteButton->setObjectName("ThumbnailDelete");
    deleteButton->setAutoRaise(true);
    QPixmap deletePix(12, 12);
    deletePix.fill(Qt::transparent);
    {
        QPainter painter(&deletePix);
        painter.setRenderHint(QPainter::Antialiasing, true);
        QPen pen(Qt::white, 2);
        pen.setCapStyle(Qt::RoundCap);
        painter.setPen(pen);
        painter.drawLine(QPointF(3, 3), QPointF(9, 9));
        painter.drawLine(QPointF(9, 3), QPointF(3, 9));
    }
    deleteButton->setIcon(QIcon(deletePix));
    deleteButton->setIconSize(QSize(12, 12));
    deleteButton->setCursor(Qt::PointingHandCursor);
    deleteButton->setToolTip(tr("Remove image"));
    imageLayout->addWidget(deleteButton, 0, 0, Qt::AlignTop | Qt::AlignRight);

    cardLayout->addWidget(imageContainer, 0, Qt::AlignCenter);

    const int labelWidth = cardSize().width() - 16;
    auto *labelWidget = new QLabel(card);
    labelWidget->setObjectName("ThumbnailLabel");
    labelWidget->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    labelWidget->setWordWrap(false);
    labelWidget->setText(QFontMetrics(labelWidget->font())
                             .elidedText(label, Qt::ElideRight, labelWidth));
    cardLayout->addWidget(labelWidget);

    if (!details.isEmpty())
    {
        auto *detailsWidget = new QLabel(card);
        detailsWidget->setObjectName("ThumbnailDetails");
        detailsWidget->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
        detailsWidget->setText(details);
        cardLayout->addWidget(detailsWidget);
    }

    rowLayout->addWidget(card);

    connect(deleteButton, &QToolButton::clicked, this, [this, item]()
            {
        const int index = m_listWidget->row(item);
        if (index >= 0) {
            emit imageDeleteRequested(index);
        } });

    return row;
}

QSize CThumbnailWidget::cardSize() const
{
    return QSize(m_thumbnailSize.width() + 24, m_thumbnailSize.height() + 64);
}

void CThumbnailWidget::updateLayoutSizes()
{
    if (!m_listWidget)
    {
        return;
    }
    int viewWidth = m_listWidget->viewport()->width();
    if (viewWidth <= 0)
    {
        viewWidth = m_listWidget->width();
    }
    const int gridWidth = std::max(viewWidth, cardSize().width() + 16);
    const int gridHeight = cardSize().height() + 8;
    m_listWidget->setGridSize(QSize(gridWidth, gridHeight));

    for (int i = 0; i < m_listWidget->count(); ++i)
    {
        QListWidgetItem *item = m_listWidget->item(i);
        if (item)
        {
            item->setSizeHint(QSize(gridWidth, gridHeight));
        }
        if (QWidget *row = m_listWidget->itemWidget(item))
        {
            row->setFixedWidth(gridWidth);
        }
    }
}

void CThumbnailWidget::updateSelectionState()
{
    const int current = m_listWidget->currentRow();
    for (int i = 0; i < m_listWidget->count(); ++i)
    {
        QListWidgetItem *item = m_listWidget->item(i);
        QWidget *row = item ? m_listWidget->itemWidget(item) : nullptr;
        if (!row)
        {
            continue;
        }
        auto *card = row->findChild<QFrame *>("ThumbnailCard");
        if (!card)
        {
            continue;
        }
        const bool selected = (i == current);
        if (card->property("selected").toBool() != selected)
        {
            card->setProperty("selected", selected);
            card->style()->unpolish(card);
            card->style()->polish(card);
        }
    }
}

void CThumbnailWidget::updateEmptyState()
{
    const bool hasItems = m_listWidget->count() > 0;
    m_listWidget->setVisible(hasItems);
    m_emptyLabel->setVisible(!hasItems);
    if (hasItems)
    {
        updateLayoutSizes();
    }
}

void CThumbnailWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    updateLayoutSizes();
}
