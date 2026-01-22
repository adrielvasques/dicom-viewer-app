#pragma once

#include <QDebug>
#include <QString>
#include <string>

/**
 * @def DICOMVIEWER_DEBUG
 * @brief Enable/disable debug logging (1 = enabled, 0 = disabled)
 */
#ifndef DICOMVIEWER_DEBUG
#define DICOMVIEWER_DEBUG 1
#endif

// QDebug helper that supports std::string
class DicomViewerDebug
{
  public:
    DicomViewerDebug(QDebug dbg)
        : m_dbg(dbg)
    {
    }

    // std::string support
    DicomViewerDebug &operator<<(const std::string &s)
    {
        m_dbg << QString::fromStdString(s);
        return *this;
    }

    // const char*
    DicomViewerDebug &operator<<(const char *s)
    {
        m_dbg << s;
        return *this;
    }

    // QString
    DicomViewerDebug &operator<<(const QString &s)
    {
        m_dbg << s;
        return *this;
    }

    // Generic fallback (ints, floats, etc.)
    template <typename T>
    DicomViewerDebug &operator<<(const T &value)
    {
        m_dbg << value;
        return *this;
    }

  private:
    QDebug m_dbg;
};

// Macros
#if DICOMVIEWER_DEBUG
#define DICOMVIEWER_LOG(msg) \
    DicomViewerDebug(qDebug()) << "[DICOMVIEWER]" << msg

#define DICOMVIEWER_WARN(msg) \
    DicomViewerDebug(qWarning()) << "[DICOMVIEWER WARN]" << msg
#else
#define DICOMVIEWER_LOG(msg) ((void)0)
#define DICOMVIEWER_WARN(msg) ((void)0)
#endif

// Errors always enabled
#define DICOMVIEWER_ERROR(msg) \
    DicomViewerDebug(qCritical()) << "[DICOMVIEWER ERROR]" << msg
