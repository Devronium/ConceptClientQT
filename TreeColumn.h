#ifndef TREE_COLUMN_H
#define TREE_COLUMN_H

#include <QWidget>
#include <QApplication>
#include "AnsiString.h"
#include <map>

class TreeColumn : public QObject {
    Q_OBJECT

public:
    int type;
    int        index;
    AnsiString name;
    std::map<int, AnsiString> Properties;
    QWidget *widget;

    TreeColumn(AnsiString& name, int type, int index = -1) : QObject() {
        this->name   = name;
        this->type   = type;
        this->index  = index;
        this->widget = NULL;
    }
};
#endif
