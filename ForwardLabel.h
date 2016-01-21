#ifndef FORWARD_LABEL_H
#define FORWARD_LABEL_H

#include <QWidget>
#include <QLabel>

class ForwardLabel : public QLabel {
public:
    ForwardLabel(QWidget *parent = 0) : QLabel(parent) {
    }

    ~ForwardLabel() {
    }
};
#endif
