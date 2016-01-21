#ifndef __GTKCONTROL_H
#define __GTKCONTROL_H
#include "BasicMessages.h"
#include "EventHandler.h"
#include "AnsiList.h"
#include "TreeColumn.h"

#define MAX_COLUMNS    0xFFF

enum MyPackOptions {
    MyPACK_SHRINK,
    MyPACK_EXPAND_PADDING,
    MyPACK_EXPAND_WIDGET
};

class NotebookTab {
public:
    void       *object;
    void       *image;
    void       *button;
    bool       close_button;
    long       button_id;
    AnsiString title;
    void       *header;
    bool       visible;

    NotebookTab() {
        object       = NULL;
        image        = NULL;
        button       = NULL;
        header       = NULL;
        close_button = false;
        visible      = false;
        button_id    = -1;
    }
};

class TreeData {
public:
    int                 currentindex;
    QStringList         columns;
    QStringList         attributes;
    QList<TreeColumn *> meta;
    short               icon;
    short               text;
    short               markup;
    short               hint;
    short               hidden;
    int                 extra;
    char                orientation;

    TreeData() {
        currentindex = 0;
        icon         = -1;
        text         = -1;
        markup       = -1;
        hint         = -1;
        extra        = -1;
        hidden       = -1;
        orientation  = 1;
    }

    void RemoveColumn(int index) {
        int size = meta.size();

        for (int i = 0; i < size; i++) {
            TreeColumn *td = meta[i];
            if ((td) && (td->index == index)) {
                meta.removeAt(i);
                delete td;
                size--;
            }
        }
        meta.clear();
    }

    TreeColumn *GetColumn(int index) {
        int size = meta.size();

        for (int i = 0; i < size; i++) {
            TreeColumn *td = meta[i];
            if ((td) && (td->index == index))
                return td;
        }
        return NULL;
    }

    void ClearColumns() {
        currentindex = 0;
        icon         = -1;
        text         = -1;
        markup       = -1;
        hint         = -1;
        extra        = -1;

        int size = meta.size();
        for (int i = 0; i < size; i++)
            delete meta[i];
        meta.clear();
    }

    ~TreeData() {
        ClearColumns();
    }
};

class GTKControl {
public:
    int            type;
    long           ID;
    long           ID_Adjustment;
    long           ID_HAdjustment;
    void           *ptr;
    void           *ptr2;
    void           *ptr3;
    void           *parent;
    void           *events;
    int            *extraptr;
    void           *ref;
    int            index;
    unsigned short flags;
    signed char    visible;
    signed char    use_stock;
    signed char    packtype;
    AnsiList       *pages;
    signed char    minset = 0;

    GTKControl() {
        ID             = 0;
        ID_Adjustment  = -1;
        ID_HAdjustment = -1;
        type           = 0;
        flags          = 0;
        use_stock      = 0;
        extraptr       = 0;
        events         = 0;
        visible        = -1;
        ref            = 0;
        index          = -1;
        pages          = 0;
        ptr            = 0;
        ptr2           = 0;
        ptr3           = 0;
        packtype       = -1;
        minset         = 0;
    }

    int AdjustIndex(NotebookTab *NT) {
        int index = 0;
        int i     = 0;

        while (true) {
            NotebookTab *item = (NotebookTab *)pages->Item(i++);
            if ((!item) || (item == NT))
                break;
            if (item->visible)
                index++;
        }
        return index;
    }

    int AdjustIndexReversed(int index) {
        int i = 0;

        while (true) {
            NotebookTab *item = (NotebookTab *)pages->Item(i);
            if (!item)
                break;

            if (item->visible) {
                if (!index)
                    return i;
                index--;
            }
            i++;
        }
        return -1;
    }

    void Clear() {
        if (extraptr) {
            delete[] extraptr;
            extraptr = NULL;
        }
        if (events) {
            delete (ClientEventHandler *)events;
            events = NULL;
        }
        if (pages) {
            int i = 0;
            while (true) {
                NotebookTab *item = (NotebookTab *)pages->Item(i++);
                if (!item)
                    break;
                delete item;
            }
            delete pages;
            pages = NULL;
        }
    }

    ~GTKControl() {
        Clear();
    }
};
#endif //__GTKCONTROL_H
