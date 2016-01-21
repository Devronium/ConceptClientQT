#ifndef EVENT_AWARE_WIDGET_H
#define EVENT_AWARE_WIDGET_H

#include <QWidget>
#include <QMimeData>
#include <QDrag>
#include <QComboBox>
#include <QTreeWidget>
#include <QScrollArea>
#include "callback.h"
#include "CustomButton.h"
#include "verticallabel.h"


#define DERIVED_TEMPLATE_SET(item, member, value)                             \
    switch (item->type) {                                                     \
        case CLASS_BUTTON:                                                    \
            ((EventAwareWidget<CustomButton> *)item->ptr)->member = value;    \
            break;                                                            \
        case CLASS_LABEL:                                                     \
            ((EventAwareWidget<VerticalLabel> *)item->ptr)->member = value;   \
            break;                                                            \
        case CLASS_IMAGE:                                                     \
            ((EventAwareWidget<QLabel> *)item->ptr)->member = value;          \
            break;                                                            \
        case CLASS_EDIT:                                                      \
            ((EventAwareWidget<QLineEdit> *)item->ptr)->member = value;       \
            break;                                                            \
        case CLASS_TEXTVIEW:                                                  \
            ((EventAwareWidget<QTextEdit> *)item->ptr)->member = value;       \
            break;                                                            \
        case CLASS_SPINBUTTON:                                                \
            ((EventAwareWidget<QSpinBox> *)item->ptr)->member = value;        \
            break;                                                            \
        case CLASS_COMBOBOX:                                                  \
        case CLASS_COMBOBOXTEXT:                                              \
            ((EventAwareWidget<QComboBox> *)item->ptr)->member = value;       \
            break;                                                            \
        case CLASS_FRAME:                                                     \
            ((EventAwareWidget<QGroupBox> *)item->ptr)->member = value;       \
            break;                                                            \
        case CLASS_CHECKBUTTON:                                               \
            ((EventAwareWidget<QCheckBox> *)item->ptr)->member = value;       \
            break;                                                            \
        case CLASS_RADIOBUTTON:                                               \
            ((EventAwareWidget<QRadioButton> *)item->ptr)->member = value;    \
            break;                                                            \
        case CLASS_EVENTBOX:                                                  \
        case CLASS_FIXED:                                                     \
        case CLASS_ASPECTFRAME:                                               \
        case CLASS_ALIGNMENT:                                                 \
        case CLASS_MIRRORBIN:                                                 \
        case CLASS_VIEWPORT:                                                  \
        case CLASS_FORM:                                                      \
        case CLASS_VBOX:                                                      \
        case CLASS_HBOX:                                                      \
        case CLASS_VBUTTONBOX:                                                \
        case CLASS_HBUTTONBOX:                                                \
        case CLASS_EXPANDER:                                                  \
        case CLASS_TABLE:                                                     \
            ((EventAwareWidget<QWidget> *)item->ptr)->member = value;         \
            break;                                                            \
        case CLASS_HSEPARATOR:                                                \
        case CLASS_VSEPARATOR:                                                \
            ((EventAwareWidget<QFrame> *)item->ptr)->member = value;          \
            break;                                                            \
        case CLASS_PROGRESSBAR:                                               \
            ((EventAwareWidget<QProgressBar> *)item->ptr)->member = value;    \
            break;                                                            \
        case CLASS_VPANED:                                                    \
        case CLASS_HPANED:                                                    \
            ((EventAwareWidget<QSplitter> *)item->ptr)->member = value;       \
            break;                                                            \
        case CLASS_SCROLLEDWINDOW:                                            \
            ((EventAwareWidget<QScrollArea> *)item->ptr)->member = value;     \
            break;                                                            \
        case CLASS_VSCALE:                                                    \
        case CLASS_HSCALE:                                                    \
            ((EventAwareWidget<QSlider> *)item->ptr)->member = value;         \
            break;                                                            \
        case CLASS_VSCROLLBAR:                                                \
        case CLASS_HSCROLLBAR:                                                \
            ((EventAwareWidget<QScrollBar> *)item->ptr)->member = value;      \
            break;                                                            \
        case CLASS_MENUBAR:                                                   \
            ((EventAwareWidget<QMenuBar> *)item->ptr)->member = value;        \
            break;                                                            \
        case CLASS_TOOLBAR:                                                   \
            ((EventAwareWidget<QToolBar> *)item->ptr)->member = value;        \
            break;                                                            \
        case CLASS_HANDLEBOX:                                                 \
            ((EventAwareWidget<QDockWidget> *)item->ptr)->member = value;     \
            break;                                                            \
        case CLASS_STATUSBAR:                                                 \
            ((EventAwareWidget<QStatusBar> *)item->ptr)->member = value;      \
            break;                                                            \
        case CLASS_CALENDAR:                                                  \
            ((EventAwareWidget<QCalendarWidget> *)item->ptr)->member = value; \
            break;                                                            \
        case CLASS_NOTEBOOK:                                                  \
            ((EventAwareWidget<QTabWidget> *)item->ptr)->member = value;      \
            break;                                                            \
        case CLASS_TREEVIEW:                                                  \
            ((EventAwareWidget<QTabWidget> *)item->ptr)->member = value;      \
            break;                                                            \
        case CLASS_ICONVIEW:                                                  \
            ((EventAwareWidget<QTabWidget> *)item->ptr)->member = value;      \
            break;                                                            \
    }

#define DERIVED_TEMPLATE_GET(item, member, value)                             \
    switch (item->type) {                                                     \
        case CLASS_BUTTON:                                                    \
            value = ((EventAwareWidget<CustomButton> *)item->ptr)->member;    \
            break;                                                            \
        case CLASS_LABEL:                                                     \
            value = ((EventAwareWidget<VerticalLabel> *)item->ptr)->member;   \
            break;                                                            \
        case CLASS_IMAGE:                                                     \
            value = ((EventAwareWidget<QLabel> *)item->ptr)->member;          \
            break;                                                            \
        case CLASS_EDIT:                                                      \
            value = ((EventAwareWidget<QLineEdit> *)item->ptr)->member;       \
            break;                                                            \
        case CLASS_TEXTVIEW:                                                  \
            value = ((EventAwareWidget<QTextEdit> *)item->ptr)->member;       \
            break;                                                            \
        case CLASS_SPINBUTTON:                                                \
            value = ((EventAwareWidget<QSpinBox> *)item->ptr)->member;        \
            break;                                                            \
        case CLASS_COMBOBOX:                                                  \
        case CLASS_COMBOBOXTEXT:                                              \
            value = ((EventAwareWidget<QComboBox> *)item->ptr)->member;       \
            break;                                                            \
        case CLASS_FRAME:                                                     \
            value = ((EventAwareWidget<QGroupBox> *)item->ptr)->member;       \
            break;                                                            \
        case CLASS_CHECKBUTTON:                                               \
            value = ((EventAwareWidget<QCheckBox> *)item->ptr)->member;       \
            break;                                                            \
        case CLASS_RADIOBUTTON:                                               \
            value = ((EventAwareWidget<QRadioButton> *)item->ptr)->member;    \
            break;                                                            \
        case CLASS_EVENTBOX:                                                  \
        case CLASS_FIXED:                                                     \
        case CLASS_ASPECTFRAME:                                               \
        case CLASS_ALIGNMENT:                                                 \
        case CLASS_MIRRORBIN:                                                 \
        case CLASS_VIEWPORT:                                                  \
        case CLASS_FORM:                                                      \
        case CLASS_VBOX:                                                      \
        case CLASS_HBOX:                                                      \
        case CLASS_VBUTTONBOX:                                                \
        case CLASS_HBUTTONBOX:                                                \
        case CLASS_EXPANDER:                                                  \
        case CLASS_TABLE:                                                     \
            value = ((EventAwareWidget<QWidget> *)item->ptr)->member;         \
            break;                                                            \
        case CLASS_HSEPARATOR:                                                \
        case CLASS_VSEPARATOR:                                                \
            value = ((EventAwareWidget<QFrame> *)item->ptr)->member;          \
            break;                                                            \
        case CLASS_PROGRESSBAR:                                               \
            value = ((EventAwareWidget<QProgressBar> *)item->ptr)->member;    \
            break;                                                            \
        case CLASS_VPANED:                                                    \
        case CLASS_HPANED:                                                    \
            value = ((EventAwareWidget<QSplitter> *)item->ptr)->member;       \
            break;                                                            \
        case CLASS_SCROLLEDWINDOW:                                            \
            value = ((EventAwareWidget<QScrollArea> *)item->ptr)->member;     \
            break;                                                            \
        case CLASS_VSCALE:                                                    \
        case CLASS_HSCALE:                                                    \
            value = ((EventAwareWidget<QSlider> *)item->ptr)->member;         \
            break;                                                            \
        case CLASS_VSCROLLBAR:                                                \
        case CLASS_HSCROLLBAR:                                                \
            value = ((EventAwareWidget<QScrollBar> *)item->ptr)->member;      \
            break;                                                            \
        case CLASS_MENUBAR:                                                   \
            value = ((EventAwareWidget<QMenuBar> *)item->ptr)->member;        \
            break;                                                            \
        case CLASS_TOOLBAR:                                                   \
            value = ((EventAwareWidget<QToolBar> *)item->ptr)->member;        \
            break;                                                            \
        case CLASS_HANDLEBOX:                                                 \
            value = ((EventAwareWidget<QDockWidget> *)item->ptr)->member;     \
            break;                                                            \
        case CLASS_STATUSBAR:                                                 \
            value = ((EventAwareWidget<QStatusBar> *)item->ptr)->member;      \
            break;                                                            \
        case CLASS_CALENDAR:                                                  \
            value = ((EventAwareWidget<QCalendarWidget> *)item->ptr)->member; \
            break;                                                            \
        case CLASS_NOTEBOOK:                                                  \
            value = ((EventAwareWidget<QTabWidget> *)item->ptr)->member;      \
            break;                                                            \
        case CLASS_TREEVIEW:                                                  \
            value = ((EventAwareWidget<QTabWidget> *)item->ptr)->member;      \
            break;                                                            \
        case CLASS_ICONVIEW:                                                  \
            value = ((EventAwareWidget<QTabWidget> *)item->ptr)->member;      \
            break;                                                            \
    }

#define EVENT_REGISTERED(CONCEPT_EVENT)    (item) && (item->events) && (((ClientEventHandler *)item->events)->IsRegistered(CONCEPT_EVENT))
#define NOTIFY_EVENT(CONCEPT_EVENT, DATA)                                                                \
    if ((item) && (item->events) && (((ClientEventHandler *)item->events)->IsRegistered(CONCEPT_EVENT))) \
        ((ClientEventHandler *)item->events)->SendEvent(CONCEPT_EVENT, DATA);                            \

template<class T>
class EventAwareWidget : public T {
public:
    bool       dragable;
    QPixmap    dragicon;
    QString    dragData;
    QWidget    *wfriend;
    float      ratio;
    GTKControl *item;
    char       dropsite;
    AnsiString path;

    EventAwareWidget(GTKControl *control, QWidget *parent = 0) : T(parent) {
        dragable = false;
        ratio    = -1;
        dropsite = 0;
        wfriend  = NULL;
        item     = control;
    }

    int heightForWidth(int width) const {
        if (ratio > 0)
            return ratio * width;
        return T::heightForWidth(width);
    }

    void mousePressEvent(QMouseEvent *ev) {
        if ((dragable) && (ev->button() == Qt::LeftButton)) {
            QDrag     *drag     = new QDrag(this);
            QMimeData *mimeData = new QMimeData;

            mimeData->setText(dragData);
            drag->setMimeData(mimeData);

            if (dragicon.isNull()) {
                QPixmap image = QPixmap::grabWidget(this);
                drag->setPixmap(image);
            } else {
                drag->setPixmap(dragicon);
            }
            NOTIFY_EVENT(EVENT_ON_DRAGBEGIN, "")
            NOTIFY_EVENT(EVENT_ON_DRAGDATAGET, "")
            Qt::DropAction dropAction = drag->exec();
            NOTIFY_EVENT(EVENT_ON_DRAGEND, "")
            NOTIFY_EVENT(EVENT_ON_DRAGDATADELETE, "")
            delete drag;
        }
        T::mousePressEvent(ev);

        if (EVENT_REGISTERED(EVENT_ON_BUTTONPRESS)) {
            AnsiString data = AnsiString((long)ev->x());
            data += (char *)":";
            data += AnsiString((long)ev->y());
            data += (char *)":";
            data += AnsiString((long)ev->button());
            data += (char *)":";
            data += AnsiString((long)1);
            NOTIFY_EVENT(EVENT_ON_BUTTONPRESS, data);
        }
    }

    void mouseReleaseEvent(QMouseEvent *ev) {
        T::mouseReleaseEvent(ev);

        if (EVENT_REGISTERED(EVENT_ON_BUTTONRELEASE)) {
            AnsiString data = AnsiString((long)ev->x());
            data += (char *)":";
            data += AnsiString((long)ev->y());
            data += (char *)":";
            data += AnsiString((long)ev->button());
            data += (char *)":";
            data += AnsiString((long)0);
            NOTIFY_EVENT(EVENT_ON_BUTTONRELEASE, data);
        }
    }

    void setPosition(int i) {
        ratio = i;

        QList<int> sizes = ((QSplitter *)item->ptr)->sizes();

        int total = 1000;

        /*int len = sizes.size();
           for (int i=0;i<len;i++)
            total+=sizes[i];
           if (!total)
            return;*/

        QList<int> list;
        if (i < 0) {
            list << total + i;
            list << -i;

            ratio = total + i;
        } else {
            list << i;
            list << total - i;
        }
        ((QSplitter *)item->ptr)->setSizes(list);
    }

    void showEvent(QShowEvent *ev) {
        T::showEvent(ev);

        if ((item) && (item->parent)) {
            GTKControl *parent = (GTKControl *)item->parent;

            if ((parent->type == CLASS_VPANED) || (parent->type == CLASS_HPANED)) {
                if (((EventAwareWidget<QSplitter> *)parent->ptr)->ratio > 0)
                    ((EventAwareWidget<QSplitter> *)parent->ptr)->setPosition(((EventAwareWidget<QSplitter> *)parent->ptr)->ratio);
            }
        } else
        if ((item->type == CLASS_VPANED) || (item->type == CLASS_HPANED)) {
            if (ratio > 0)
                setPosition(ratio);
        }

        if (!ev->spontaneous()) {
            if (EVENT_REGISTERED(EVENT_ON_REALIZE)) {
                NOTIFY_EVENT(EVENT_ON_REALIZE, "");
                ((ClientEventHandler *)item->events)->UnregisterEvent(EVENT_ON_REALIZE);
            }
            NOTIFY_EVENT(EVENT_ON_SHOW, "");
            NOTIFY_EVENT(EVENT_ON_VISIBILITY, "1");
        } else {
            NOTIFY_EVENT(EVENT_ON_VISIBILITY, "-1");
        }
    }

    void hideEvent(QHideEvent *ev) {
        T::hideEvent(ev);

        if (!ev->spontaneous()) {
            NOTIFY_EVENT(EVENT_ON_HIDE, "");
        }
        NOTIFY_EVENT(EVENT_ON_VISIBILITY, "0");
    }

    void closeEvent(QCloseEvent *ev) {
        static unsigned int last_message = -1;
        static unsigned int close_clicked_count = 0;
        if (EVENT_REGISTERED(EVENT_ON_DELETEEVENT)) {
            NOTIFY_EVENT(EVENT_ON_DELETEEVENT, "");

            CConceptClient *client = GetClient();
            if ((client) && (client->CLIENT_SOCKET < 0)) {
                Done(client, 0, true);
                T::closeEvent(ev);
            } else {
                if ((item) && (client->MainForm == item) && (last_message == client->msg_count)) {
                    if (close_clicked_count) {
                        Done(client, 0, true);
                        T::closeEvent(ev);
                        return;
                    }
                    close_clicked_count++;
                    ev->ignore();
                    return;
                } else {
                    close_clicked_count = 0;
                    last_message = client->msg_count;
                    ev->ignore();
                }
            }
        } else {
            NOTIFY_EVENT(EVENT_ON_UNREALIZE, "");
            T::closeEvent(ev);
        }
    }

    void resizeEvent(QResizeEvent *ev) {
        T::resizeEvent(ev);

        if (EVENT_REGISTERED(EVENT_ON_SIZEREQUEST)) {
            QSize      size = ev->size();
            AnsiString data = AnsiString((long)size.width());
            data += (char *)":";
            data += AnsiString((long)size.height());
            NOTIFY_EVENT(EVENT_ON_SIZEREQUEST, data);
        }
        if (EVENT_REGISTERED(EVENT_ON_SIZEALLOCATE)) {
            QSize      size = ev->size();
            AnsiString data = AnsiString((long)size.width());
            data += (char *)":";
            data += AnsiString((long)size.height());
            NOTIFY_EVENT(EVENT_ON_SIZEALLOCATE, data);
        }
        NOTIFY_EVENT(EVENT_ON_CONFIGURE, "");
        if (item->type == 1002) {
            NOTIFY_EVENT(350, "");
        }
    }

    void focusInEvent(QFocusEvent *ev) {
        T::focusInEvent(ev);

        NOTIFY_EVENT(EVENT_ON_GRABFOCUS, "");
        NOTIFY_EVENT(EVENT_ON_FOCUS, "");
        NOTIFY_EVENT(EVENT_ON_FOCUSIN, "");
        NOTIFY_EVENT(EVENT_ON_SETFOCUS, "");
        if ((item->type == CLASS_COMBOBOX) || (item->type == CLASS_COMBOBOXTEXT)) {
            QVariant val = this->property("item");
            if (val.isValid()) {
                QVariant val2 = this->property("treeview");
                if (val2.isValid()) {
                    QTreeWidget     *widget = (QTreeWidget *)val2.toULongLong();
                    QTreeWidgetItem *titem  = (QTreeWidgetItem *)val.toULongLong();
                    if ((widget) && (titem)) {
                        int column = this->property("column").toInt();
                        widget->setCurrentItem(titem, column);
                    }
                }
            }
        }
    }

    void focusOutEvent(QFocusEvent *ev) {
        T::focusOutEvent(ev);

        NOTIFY_EVENT(EVENT_ON_FOCUSOUT, "");
    }

    void changeEvent(QEvent *ev) {
        T::changeEvent(ev);

        if ((item) && (item->type == CLASS_FORM)) {
            if (EVENT_REGISTERED(EVENT_ON_FOCUSOUT)) {
                if ((ev->type() == QEvent::ActivationChange) && (!this->isActiveWindow())) {
                    NOTIFY_EVENT(EVENT_ON_FOCUSOUT, "");
                }
            }
            if (EVENT_REGISTERED(EVENT_ON_FOCUSIN)) {
                if ((ev->type() == QEvent::ActivationChange) && (this->isActiveWindow())) {
                    NOTIFY_EVENT(EVENT_ON_FOCUSIN, "");
                }
            }
        }
    }

    void enterEvent(QEvent *ev) {
        T::enterEvent(ev);

        NOTIFY_EVENT(EVENT_ON_ENTER, "");
    }

    void leaveEvent(QEvent *ev) {
        T::leaveEvent(ev);

        NOTIFY_EVENT(EVENT_ON_LEAVE, "");
    }

    bool event(QEvent *ev) {
        NOTIFY_EVENT(EVENT_ON_EVENT, AnsiString((long)ev->type()));
        return T::event(ev);
    }

    void mouseMoveEvent(QMouseEvent *ev) {
        T::mouseMoveEvent(ev);

        if (EVENT_REGISTERED(EVENT_ON_MOTIONNOTIFY)) {
            AnsiString data = AnsiString((long)ev->x());
            data += (char *)":";
            data += AnsiString((long)ev->y());
            data += (char *)":";
            data += AnsiString((long)ev->button());
            NOTIFY_EVENT(EVENT_ON_MOTIONNOTIFY, data);
        }
    }

    void paintEvent(QPaintEvent *ev) {
        T::paintEvent(ev);

        NOTIFY_EVENT(EVENT_ON_EXPOSEEVENT, "");
    }

    void keyPressEvent(QKeyEvent *ev) {
        T::keyPressEvent(ev);

        if (EVENT_REGISTERED(EVENT_ON_KEYPRESS)) {
            NOTIFY_EVENT(EVENT_ON_KEYPRESS, AnsiString((long)ev->key()));
        } else
        if ((item->type == CLASS_EDIT) && (wfriend)) {
            int key = ev->key();
            if ((key == Qt::Key_Up) || (key == Qt::Key_Down)) {
                CConceptClient *client = GetClient();
                if (client) {
                    GTKControl *sourcecontrol = client->ControlsReversed[wfriend];
                    if ((sourcecontrol) && (sourcecontrol->events)) {
                        if (key == Qt::Key_Down)
                            ((ClientEventHandler *)sourcecontrol->events)->cacheval++;
                        else {
                            if (((ClientEventHandler *)sourcecontrol->events)->cacheval > 0)
                                ((ClientEventHandler *)sourcecontrol->events)->cacheval--;
                        }
                        if (!((ClientEventHandler *)sourcecontrol->events)->SearchItemIndex(((QLineEdit *)item->ptr)->text(), ((ClientEventHandler *)sourcecontrol->events)->cacheval)) {
                            if (((ClientEventHandler *)sourcecontrol->events)->cacheval) {
                                ((ClientEventHandler *)sourcecontrol->events)->cacheval = 0;
                                ((ClientEventHandler *)sourcecontrol->events)->SearchItemIndex(((QLineEdit *)item->ptr)->text(), ((ClientEventHandler *)sourcecontrol->events)->cacheval);
                            }
                        }
                    }
                }
            }
        }
    }

    void keyReleaseEvent(QKeyEvent *ev) {
        T::keyReleaseEvent(ev);

        if (EVENT_REGISTERED(EVENT_ON_KEYRELEASE)) {
            AnsiString data = AnsiString((long)ev->key());
            data += (char *)":";
            data += AnsiString((long)ev->modifiers());
            data += (char *)":";
            data += AnsiString((long)ev->count());
            NOTIFY_EVENT(EVENT_ON_KEYRELEASE, data);
        }
        if ((item->type == 1002) && (EVENT_REGISTERED(351))) {
            NOTIFY_EVENT(351, AnsiString((long)ev->key()));
        }
    }

    void wheelEvent(QWheelEvent *ev) {
        if (item) {
            if ((item->type == CLASS_COMBOBOX) || (item->type == CLASS_COMBOBOXTEXT)) {
                ev->ignore();
                return;
            }
        }
        T::wheelEvent(ev);
    }

    void UpdateHeight() {
        if (item->type == CLASS_TREEVIEW) {
            QSizePolicy qs      = this->sizePolicy();
            GTKControl  *scroll = (GTKControl *)item->parent;
            GTKControl  *parent = scroll;
            int         level   = 0;
            int         boxes   = 0;
            bool        out     = false;
            while ((parent) && (qs.verticalPolicy() != QSizePolicy::Fixed) && (!out)) {
                QWidget *pwidget = (QWidget *)parent->ptr;
                if (pwidget) {
                    if (parent->type == CLASS_VBOX) {
                        GTKControl *parent2 = (GTKControl *)parent->parent;
                        if (parent2) {
                            if ((parent2->type != CLASS_TABLE) && (parent2->type != CLASS_VBOX))
                                break;
                        }
                    }

                    qs = pwidget->sizePolicy();
                    if (qs.verticalPolicy() == QSizePolicy::Fixed)
                        break;

                    switch (parent->type) {
                        case CLASS_SCROLLEDWINDOW:
                        case CLASS_VIEWPORT:
                            level++;

                        case CLASS_HBOX:
                        case CLASS_TABLE:
                            boxes++;
                            parent = (GTKControl *)parent->parent;
                            break;

                        default:
                            out = true;
                            //parent = NULL;
                    }
                } else
                    break;
            }
            if ((!level) && ((parent) && (parent->type == CLASS_SCROLLEDWINDOW))) {
                GTKControl *parent2 = (GTKControl *)parent->parent;
                if (parent2) {
                    switch (parent2->type) {
                        case CLASS_FORM:
                        case CLASS_NOTEBOOK:
                        case CLASS_FRAME:
                        case CLASS_BUTTON:
                        case CLASS_HPANED:
                        case CLASS_VPANED:
                        case CLASS_EXPANDER:
                            return;
                    }
                }
            }
            if (((scroll) && (scroll->type == CLASS_SCROLLEDWINDOW) && (((QScrollArea *)scroll->ptr)->verticalScrollBarPolicy() == Qt::ScrollBarAlwaysOn)) || ((qs.verticalPolicy() == QSizePolicy::Fixed) && (((QTreeWidget *)item->ptr)->isHeaderHidden()))) {
                //if ((qs.verticalPolicy()==QSizePolicy::Fixed) && (((QTreeWidget *)item->ptr)->isHeaderHidden())) {
                int height = 0;

                QTreeWidgetItem *parent = ((QTreeWidget *)item->ptr)->invisibleRootItem();
                if (parent) {
                    int ccount = parent->childCount();
                    for (int i = 0; i < ccount; i++) {
                        int h = ((QTreeWidget *)this)->sizeHintForRow(i);
                        if (h > 0)
                            height += h;
                    }
                }
                if (height <= 0)
                    height = 20;

                ((QTreeWidget *)item->ptr)->setFixedHeight(height + 2);
                if (qs.verticalPolicy() == QSizePolicy::Fixed) {
                    if ((scroll) && ((scroll->type == CLASS_SCROLLEDWINDOW) || (scroll->type == CLASS_VIEWPORT))) {
                        if (scroll->type == CLASS_SCROLLEDWINDOW) {
                            if (((QScrollArea *)scroll->ptr)->verticalScrollBarPolicy() != Qt::ScrollBarAlwaysOff)
                                return;
                        }

                        ((QWidget *)scroll->ptr)->setFixedHeight(height + 4);
                    }
                }
            }
        }
    }

    virtual QSize sizeHint() const {
        if (item) {
            if (item->type == CLASS_SCROLLEDWINDOW) {
                QWidget *widget = ((QScrollArea *)item->ptr)->widget();
                if (widget) {
                    QSize s  = widget->sizeHint();
                    QSize s2 = T::sizeHint();
                    if (s.height() < s2.height())
                        return s;
                    return s2;
                }
                return QSize(1, 1);
            }
        }
        return T::sizeHint();
    }

    void dropEvent(QDropEvent *ev) {
        if ((!item) || (item->type != CLASS_ICONVIEW))
            T::dropEvent(ev);

        if ((item) /*&& (item->type!=CLASS_ICONVIEW)*/) {
            QString s;
            if (EVENT_REGISTERED(EVENT_ON_DRAGDATARECEIVED)) {
                QObject        *source        = ev->source();
                GTKControl     *sourcecontrol = NULL;
                CConceptClient *client        = GetClient();
                if ((source) && (item->events) && (client))
                    sourcecontrol = client->ControlsReversed[source];

                if (sourcecontrol) {
                    DERIVED_TEMPLATE_GET(sourcecontrol, dragData, s);
                }
                //}

                //if (EVENT_REGISTERED(EVENT_ON_DRAGDATARECEIVED)) {
                AnsiString data;
                if (s.size())
                    data = ((char *)s.toLatin1().data());
                else {
                    if (ev->mimeData()->hasUrls()) {
                        QList<QUrl> urls = ev->mimeData()->urls();
                        int         len  = urls.size();
                        for (int i = 0; i < len; i++) {
                            if (data.Length())
                                data += "\r\n";
                            data += urls[i].toString().toLatin1().data();
                        }
                    } else
                        data = ((char *)ev->mimeData()->text().toLatin1().data());
                }
                //if (item->type==CLASS_TREEVIEW)
                NOTIFY_EVENT(EVENT_ON_DRAGDATARECEIVED, data);
            }
        }

        if ((item) /*&& (item->type!=CLASS_TREEVIEW)*/ /*&& (item->type!=CLASS_ICONVIEW)*/) {
            if (EVENT_REGISTERED(EVENT_ON_DRAGDROP)) {
                QPoint p = ev->pos();
                if (item->type == CLASS_ICONVIEW) {
                    AnsiString      pos("-1");
                    QListWidgetItem *target = ((QListWidget *)item->ptr)->itemAt(p);
                    if (target)
                        pos = AnsiString((long)((QListWidget *)item->ptr)->row(target));

                    NOTIFY_EVENT(EVENT_ON_DRAGDROP, pos);
                } else
                if (item->type == CLASS_TREEVIEW) {
                    AnsiString pos("-1");
                    if (path.Length()) {
                        pos  = path;
                        path = "";
                    }

                    NOTIFY_EVENT(EVENT_ON_DRAGDROP, pos);
                } else {
                    AnsiString pos((long)p.x());
                    pos += ":";
                    pos += AnsiString((long)p.y());
                    NOTIFY_EVENT(EVENT_ON_DRAGDROP, pos);
                }
            }
        }

        if (item->events) {
            ((ClientEventHandler *)item->events)->ResetDrop();
            if (dropsite >= 2) {
                AnsiString ref_urls;
                if (ev->mimeData()->hasUrls()) {
                    QList<QUrl> urls = ev->mimeData()->urls();
                    int         len  = urls.size();
                    for (int i = 0; i < len; i++) {
                        if (ref_urls.Length())
                            ref_urls += "\r\n";
                        ref_urls += urls[i].toString().toLatin1().data();
                    }
                }
                ((ClientEventHandler *)item->events)->last_dropped_data = ref_urls;
            }
        }
    }

    void dragMoveEvent(QDragMoveEvent *de) {
        T::dragMoveEvent(de);

        NOTIFY_EVENT(EVENT_ON_DRAGMOTION, AnsiString((long)de->pos().x()) + ":" + AnsiString((long)de->pos().y()));
    }

    void dragEnterEvent(QDragEnterEvent *ev) {
        switch (dropsite) {
            case 1:
                if ((ev->mimeData()->hasText()) && (!ev->mimeData()->hasUrls()))
                    ev->acceptProposedAction();
                break;

            case 2:
                if ((ev->mimeData()->hasText()) && (ev->mimeData()->hasUrls()))
                    ev->acceptProposedAction();
                break;

            case 3:
                ev->acceptProposedAction();
                break;
        }
        T::dragEnterEvent(ev);
    }

    void dragLeaveEvent(QDragLeaveEvent *ev) {
        NOTIFY_EVENT(EVENT_ON_DRAGLEAVE, "");
        T::dragLeaveEvent(ev);
    }

    QMimeData *mimeData(const QList<QTreeWidgetItem *> items) const {
        QMimeData *res = T::mimeData(items);
        int       len  = items.size();

        if ((res) && (len)) {
            NOTIFY_EVENT(EVENT_ON_DRAGBEGIN, "")
            NOTIFY_EVENT(EVENT_ON_DRAGDATAGET, "")

            QString data;
            for (int i = 0; i < len; i++) {
                if (i)
                    data += ";";

                AnsiString path = ItemToPath((QTreeWidget *)this, items[i]);
                data += path.c_str();
            }
            data += "%";
            data += dragData;
            res->setText(data);
        }
        return res;
    }

    QMimeData *mimeData(const QList<QListWidgetItem *> items) const {
        QMimeData *res = T::mimeData(items);
        int       len  = items.size();

        if ((res) && (len)) {
            NOTIFY_EVENT(EVENT_ON_DRAGBEGIN, "")
            NOTIFY_EVENT(EVENT_ON_DRAGDATAGET, "")

            QString data;
            for (int i = 0; i < len; i++) {
                if (i)
                    data += ";";

                AnsiString path((long)((QListWidget *)this)->row(items[i]));
                data += path.c_str();
            }
            data += "%";
            data += dragData;
            res->setText(data);
        }
        return res;
    }

    QStringList mimeTypes() const {
        if ((item) && ((item->type == CLASS_TREEVIEW) || (item->type == CLASS_ICONVIEW))) {
            QStringList list;
            switch (dropsite) {
                case 1:
                    list << QString("text/plain ");
                    break;

                case 2:
                    list << QString("text/uri-list");
                    break;

                case 3:
                    list << QString("text/uri-list");
                    list << QString("text/plain ");
                    break;
            }
            list << QString("text/plain ");
            //list << QString("text/uri-list");
            return list;
        }
        return T::mimeTypes();
    }

    bool dropMimeData(QTreeWidgetItem *parent, int index, const QMimeData *data, Qt::DropAction action) {
        if (!parent)
            return false;

        if (EVENT_REGISTERED(EVENT_ON_DRAGDROP)) {
            AnsiString path;
            if (parent)
                path = ItemToPath((QTreeWidget *)this, parent);

            int childCount = parent->childCount();
            if ((index >= 0) && (index < childCount) && (path.Length())) {
                path += ":";
                path += AnsiString((long)index);
            }
            this->path = path;
            //NOTIFY_EVENT(EVENT_ON_DRAGDROP, path);
        }
        return false;
    }

    /*bool dropMimeData(int index, const QMimeData * data, Qt::DropAction action) {
        if (EVENT_REGISTERED(EVENT_ON_DRAGDROP)) {
            NOTIFY_EVENT(EVENT_ON_DRAGDROP, AnsiString((long)index));
        }
        return false;
       }*/
#ifndef NO_WEBKIT
    QWebView *createWindow(QWebPage::WebWindowType type) {
        if (type != QWebPage::WebBrowserWindow)
            return T::createWindow(type);

        NOTIFY_EVENT(352, "");

        int        rid     = ((QWebView *)item->ptr)->property("newwindow").toInt();
        GTKControl *target = NULL;
        if (rid > 0) {
            ((QWebView *)item->ptr)->setProperty("newwindow", rid);
            CConceptClient *client = GetClient();
            if (client)
                target = client->Controls[rid];
        }

        if ((target) && (target->ptr)) {
            int rid2 = ((QWebView *)target->ptr)->property("newwindow").toInt();
            if (rid2 <= 0)
                ((QWebView *)target->ptr)->setProperty("newwindow", rid);

            return (QWebView *)target->ptr;
        }

        QWebView *webView = new QWebView;
        QWebPage *newWeb  = new QWebPage(webView);
        webView->setAttribute(Qt::WA_DeleteOnClose, true);
        webView->setPage(newWeb);
        webView->show();

        return webView;
    }
#endif
    ~EventAwareWidget() {
    }
};
#endif
