#ifndef CAMERAFRAMEGRABBER_H
#define CAMERAFRAMEGRABBER_H

// Qt includes
#include <QAbstractVideoSurface>
#include <QList>
#include "ConceptClient.h"

class CameraFrameGrabber : public QAbstractVideoSurface
{
    Q_OBJECT
private:
    AnsiString ID;
    CConceptClient *owner;
public:

    explicit CameraFrameGrabber(CConceptClient *owner, int ID, QObject *parent = 0);

    QList<QVideoFrame::PixelFormat> supportedPixelFormats(QAbstractVideoBuffer::HandleType handleType) const;

    bool present(const QVideoFrame& frame);

signals:
    void frameAvailable(QImage frame);

public slots:
};
#endif // CAMERAFRAMEGRABBER_H
