#include "verticallabel.h"
#include <QTextDocument>
#include <QEvent>
#include <math.h>

VerticalLabel::VerticalLabel(QWidget *parent)
    : QLabel(parent) {
    angle = 0;
}

VerticalLabel::VerticalLabel(const QString& text, QWidget *parent)
    : QLabel(text, parent) {
}

void VerticalLabel::paintEvent(QPaintEvent *ev) {
    if (angle) {
        QPainter painter(this);
        //painter.setPen(Qt::black);
        //painter.setBrush(Qt::Dense1Pattern);

        //painter.rotate(90);

        //painter.drawText(0, 0, text());
        painter.save();
        painter.translate(width() / 2, height() / 2);
        painter.rotate(360 - angle);

        if (textFormat() == Qt::RichText) {
            QTextDocument doc(this);
            doc.setUseDesignMetrics(true);
            doc.setDefaultTextOption(QTextOption(Qt::AlignHCenter));
            doc.setHtml(text());

            doc.drawContents(&painter);
        } else
            painter.drawText(0, 0, text());

        painter.restore();
    } else
        QLabel::paintEvent(ev);
}

QSize VerticalLabel::minimumSizeHint() const {
    return QLabel::minimumSizeHint();
}

QSize VerticalLabel::sizeHint() const {
    if (angle) {
        QSize s = QLabel::sizeHint();
        if ((angle == 90) || (angle == 270)) {
            return QSize(s.height(), s.width() + s.width());
        } else {
            double fangle = (double)angle * M_PI / 180;
            return QSize(abs(cos(fangle) * s.width() + sin(fangle) * s.height()), 200 + abs(sin(fangle) * s.width() + cos(fangle) * s.height()));
        }
    }
    return QLabel::sizeHint();
}

bool VerticalLabel::event(QEvent *event) {
    switch (event->type()) {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
            QLabel::event(event);
            return false;
    }
    return QLabel::event(event);
}
