#ifndef IMAGEDOCUMENT_H
#define IMAGEDOCUMENT_H

#include <QObject>
#include <QByteArray>
#include <QPixmap>
#include <QString>

class ImageDocument : public QObject
{
    Q_OBJECT

public:
    explicit ImageDocument(QObject *parent = nullptr);

    bool loadFromFile(const QString &path);
    void clear();

    bool isValid() const;
    QString path() const;
    QByteArray bytes() const;
    QPixmap pixmap() const;

signals:
    void changed();

private:
    QString m_path;
    QByteArray m_bytes;
    QPixmap m_pixmap;
};

#endif // IMAGEDOCUMENT_H
