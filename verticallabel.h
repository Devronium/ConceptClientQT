#ifndef VERTICALLABEL_H
#define VERTICALLABEL_H

#include <QPainter>
#include <QLabel>

class VerticalLabel : public QLabel
{
    Q_OBJECT
public:
    int angle;
    explicit VerticalLabel(QWidget *parent = 0);
    explicit VerticalLabel(const QString& text, QWidget *parent = 0);

protected:
    void paintEvent(QPaintEvent *);
    QSize sizeHint() const;
    QSize minimumSizeHint() const;

    bool event(QEvent *event);
};
#endif
