#ifndef BASE_WIDGET_H
#define BASE_WIDGET_H

#include <QWidget>
#include <QLabel>
#include <QStyle>
#include <QApplication>

//template<class T>
class BaseWidget : public QLabel {
    Q_OBJECT

    bool contentvisible;
public:
    QSizePolicy::Policy vpolicy;

    BaseWidget(QWidget *parent = 0) : QLabel(parent) {
        contentvisible = false;
        vpolicy        = QSizePolicy::Expanding;
    }

    ~BaseWidget() {
    }

    void Toggle(bool visible) {
        QWidget *parent = this->parentWidget();

        if (parent) {
            parent->setUpdatesEnabled(false);
            BaseWidget *image = (BaseWidget *)parent->findChild<QLabel *>("image");
            if (image) {
                QStyle *l_style = QApplication::style();
                if (l_style) {
                    if (visible)
                        image->setText(QChar(0x25BC)); //setPixmap(l_style->standardPixmap(QStyle::SP_ArrowDown));
                    else
                        image->setText(QChar(0x25BA)); //setPixmap(l_style->standardPixmap(QStyle::SP_ArrowUp));
                }
                image->contentvisible = visible;
            }
            QWidget *container = parent->findChild<QWidget *>("container");
            if (container)
                container->setVisible(visible);

            BaseWidget *label = (BaseWidget *)parent->findChild<QLabel *>("label");
            if (label)
                label->contentvisible = visible;

            if (visible) {
                QSizePolicy qs = parent->sizePolicy();
                qs.setVerticalPolicy(vpolicy);
                parent->setSizePolicy(qs);
            } else {
                QSizePolicy qs = parent->sizePolicy();
                vpolicy = qs.verticalPolicy();
                qs.setVerticalPolicy(QSizePolicy::Fixed);
                parent->setSizePolicy(qs);
            }
            parent->setUpdatesEnabled(true);
        }
    }

    void mousePressEvent(QMouseEvent *k) {
        Toggle(!contentvisible);
    }
};
#endif
