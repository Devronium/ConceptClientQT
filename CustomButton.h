#ifndef CUSTOMBUTTON_H
#define CUSTOMBUTTON_H

#include <QPushButton>
#include "GTKControl.h"

class CustomButton : public QPushButton {
public:
    GTKControl *child;
    CustomButton(QWidget *parent) : QPushButton(parent) {
        child = NULL;
    }

protected:
    QSize sizeHint() const {
        if ((child) && (child->visible > 0)) {
            QSize size   = ((QWidget *)child->ptr)->sizeHint();
            QSize button = QPushButton::sizeHint();

            if (button.width() < size.width())
                button.setWidth(size.width() + 2);

            if (button.height() < size.height())
                button.setHeight(size.height() + 2);

            return button;
        }

        return QPushButton::sizeHint();
    }
};
#endif
