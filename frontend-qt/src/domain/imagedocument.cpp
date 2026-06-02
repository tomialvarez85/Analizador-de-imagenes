#include "domain/imagedocument.h"

#include <QFile>

ImageDocument::ImageDocument(QObject *parent)
    : QObject(parent)
{
}

bool ImageDocument::loadFromFile(const QString &path)
{
    clear();

    if (path.isEmpty()) {
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    m_bytes = file.readAll();
    file.close();

    if (m_bytes.isEmpty()) {
        clear();
        return false;
    }

    if (!m_pixmap.loadFromData(m_bytes)) {
        clear();
        return false;
    }

    m_path = path;
    emit changed();
    return true;
}

void ImageDocument::clear()
{
    m_path.clear();
    m_bytes.clear();
    m_pixmap = QPixmap();
    emit changed();
}

bool ImageDocument::isValid() const
{
    return !m_bytes.isEmpty() && !m_pixmap.isNull();
}

QString ImageDocument::path() const
{
    return m_path;
}

QByteArray ImageDocument::bytes() const
{
    return m_bytes;
}

QPixmap ImageDocument::pixmap() const
{
    return m_pixmap;
}
