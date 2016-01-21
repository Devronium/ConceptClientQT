#include "cameraframegrabber.h"
#include "AnsiString.h"


#define rgbtoyuv(r, g, b, y, u, v)                                                       \
    y = (uint8_t)((((int)(66 * r) + (int)(129 * g) + (int)(25 * b) + 128) >> 8) + 16);   \
    u = (uint8_t)((((int)(-38 * r) - (int)(74 * g) + (int)(112 * b) + 128) >> 8) + 128); \
    v = (uint8_t)((((int)(112 * r) - (int)(94 * g) - (int)(18 * b) + 128) >> 8) + 128);

CameraFrameGrabber::CameraFrameGrabber(CConceptClient *owner, int ID, QObject *parent) : QAbstractVideoSurface(parent) {
    this->ID    = AnsiString((long)ID);
    this->owner = owner;
}

QList<QVideoFrame::PixelFormat> CameraFrameGrabber::supportedPixelFormats(QAbstractVideoBuffer::HandleType handleType) const {
    Q_UNUSED(handleType);
    return QList<QVideoFrame::PixelFormat>()
           << QVideoFrame::Format_ARGB32
           << QVideoFrame::Format_ARGB32_Premultiplied
           << QVideoFrame::Format_RGB32
           << QVideoFrame::Format_RGB24
           << QVideoFrame::Format_RGB565
           << QVideoFrame::Format_RGB555
           << QVideoFrame::Format_ARGB8565_Premultiplied
           << QVideoFrame::Format_BGRA32
           << QVideoFrame::Format_BGRA32_Premultiplied
           << QVideoFrame::Format_BGR32
           << QVideoFrame::Format_BGR24
           << QVideoFrame::Format_BGR565
           << QVideoFrame::Format_BGR555
           << QVideoFrame::Format_BGRA5658_Premultiplied
           << QVideoFrame::Format_AYUV444
           << QVideoFrame::Format_AYUV444_Premultiplied
           << QVideoFrame::Format_YUV444
           << QVideoFrame::Format_YUV420P
           << QVideoFrame::Format_YV12
           << QVideoFrame::Format_UYVY
           << QVideoFrame::Format_YUYV
           << QVideoFrame::Format_NV12
           << QVideoFrame::Format_NV21
           << QVideoFrame::Format_IMC1
           << QVideoFrame::Format_IMC2
           << QVideoFrame::Format_IMC3
           << QVideoFrame::Format_IMC4
           << QVideoFrame::Format_Y8
           << QVideoFrame::Format_Y16
           << QVideoFrame::Format_Jpeg
           << QVideoFrame::Format_CameraRaw
           << QVideoFrame::Format_AdobeDng;
}

void RGBtoYUV420P(const uint8_t *RGB, uint8_t *YUV, uint RGBIncrement, bool swapRGB, int width, int height, bool flip) {
    const unsigned planeSize = width * height;
    const unsigned halfWidth = width >> 1;

    // get pointers to the data
    uint8_t       *yplane   = YUV;
    uint8_t       *uplane   = YUV + planeSize;
    uint8_t       *vplane   = YUV + planeSize + (planeSize >> 2);
    const uint8_t *RGBIndex = RGB;
    int           RGBIdx[3];

    RGBIdx[0] = 0;
    RGBIdx[1] = 1;
    RGBIdx[2] = 2;
    if (swapRGB) {
        RGBIdx[0] = 2;
        RGBIdx[2] = 0;
    }

    for (int y = 0; y < (int)height; y++) {
        uint8_t *yline = yplane + (y * width);
        uint8_t *uline = uplane + ((y >> 1) * halfWidth);
        uint8_t *vline = vplane + ((y >> 1) * halfWidth);

        if (flip) // Flip horizontally
            RGBIndex = RGB + (width * (height - 1 - y) * RGBIncrement);

        for (int x = 0; x < width; x += 2) {
            rgbtoyuv(RGBIndex[RGBIdx[0]], RGBIndex[RGBIdx[1]], RGBIndex[RGBIdx[2]],
                     *yline, *uline, *vline);
            RGBIndex += RGBIncrement;
            yline++;
            rgbtoyuv(RGBIndex[RGBIdx[0]], RGBIndex[RGBIdx[1]], RGBIndex[RGBIdx[2]],
                     *yline, *uline, *vline);
            RGBIndex += RGBIncrement;
            yline++;
            uline++;
            vline++;
        }
    }
}

bool CameraFrameGrabber::present(const QVideoFrame& frame) {
    if (frame.isValid()) {
        QVideoFrame cloneFrame(frame);
        cloneFrame.map(QAbstractVideoBuffer::ReadOnly);
        const QImage image(cloneFrame.bits(),
                           cloneFrame.width(),
                           cloneFrame.height(),
                           QVideoFrame::imageFormatFromPixelFormat(cloneFrame.pixelFormat()));

        const QImage::Format FORMAT = QImage::Format_RGB32;
        QImage img2;

        if (image.format() != QImage::Format_RGB32)
            img2 = image.convertToFormat(FORMAT);
        else
            img2 = image;

        int           size    = img2.width() * img2.height();
        unsigned char *buffer = (unsigned char *)malloc(((size * 3) / 2) + 100);
        RGBtoYUV420P(img2.bits(), buffer, img2.depth() / 8, true, img2.width(), img2.height(), false);
        AnsiString buf;

        buf.LinkBuffer((char *)buffer, size);
        //this->owner->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, "350", buf);

        cloneFrame.unmap();
        return true;
    }
    return false;
}
