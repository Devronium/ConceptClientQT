#include "EventHandler.h"
#include "BasicMessages.h"
#include "GTKControl.h"
#include "ConceptClient.h"
#include "EventAwareWidget.h"
#include "callback.h"
#include <QLineEdit>
#include <QHeaderView>
#include "PropertiesBox/qttreepropertybrowser.h"

ClientEventHandler::ClientEventHandler(long ID, void *CC) : QObject(NULL) {
    this->ID       = ID;
    this->Client   = CC;
    this->timer    = NULL;
    this->popup    = NULL;
    this->obj      = ((CConceptClient *)this->Client)->Controls[ID];
    this->cacheval = 0;
}

void ClientEventHandler::SendEvent(int CONCEPT_EVENT, AnsiString EventData) {
    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)CONCEPT_EVENT), EventData);
}

void ClientEventHandler::RegisterEvent(int CONCEPT_EVENT) {
    Events[CONCEPT_EVENT] = true;
}

void ClientEventHandler::UnregisterEvent(int CONCEPT_EVENT) {
    Events[CONCEPT_EVENT] = false;
}

bool ClientEventHandler::IsRegistered(int CONCEPT_EVENT) {
    return Events[CONCEPT_EVENT];
}

void ClientEventHandler::on_clicked() {
    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)EVENT_ON_CLICKED), "");
    if ((((CConceptClient *)this->Client)->parser) && (entry_point.Length()))
        ((CConceptClient *)this->Client)->parser->NotifyEvent(entry_point, this->ID.ToInt(), "");
}

void ClientEventHandler::on_toggled(bool toggled) {
    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)EVENT_ON_TOGGLED), AnsiString((long)toggled));
}

void ClientEventHandler::on_triggered(bool toggled) {
    AnsiString param((long)toggled);

    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)EVENT_ON_CLICKED), param);
    if ((((CConceptClient *)this->Client)->parser) && (entry_point.Length()))
        ((CConceptClient *)this->Client)->parser->NotifyEvent(entry_point, this->ID.ToInt(), param);
}

void ClientEventHandler::on_activated(bool toggled) {
    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)EVENT_ON_ACTIVATE), AnsiString((long)toggled));
}

void ClientEventHandler::on_pressed(bool toggled) {
    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)EVENT_ON_PRESSED), "0");
}

void ClientEventHandler::on_released(bool toggled) {
    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)EVENT_ON_RELEASED), "0");
}

void ClientEventHandler::on_pressed_secondary(bool toggled) {
    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)EVENT_ON_PRESSED), "1");
}

void ClientEventHandler::on_released_secondary(bool toggled) {
    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)EVENT_ON_RELEASED), "1");
}

void ClientEventHandler::on_tabCloseRequested(int index) {
    GTKControl *item = (GTKControl *)obj;//((CConceptClient *)this->Client)->Controls[this->ID.ToInt()];

    if ((item) && (item->type == CLASS_NOTEBOOK)) {
        index = item->AdjustIndexReversed(index);
        NotebookTab *nt = (NotebookTab *)item->pages->Item(index);
        if ((nt) && (nt->button_id > -1)) {
            AnsiString bid((long)nt->button_id);
            ((CConceptClient *)this->Client)->SendMessageNoWait(bid.c_str(), MSG_EVENT_FIRED, AnsiString((long)EVENT_ON_BUTTONRELEASE), "");
        }
    }
}

void ClientEventHandler::on_currentChanged(int index) {
    GTKControl *item = (GTKControl *)obj;//((CConceptClient *)this->Client)->Controls[this->ID.ToInt()];

    if ((item) && (item->type == CLASS_NOTEBOOK)) {
        index = item->AdjustIndexReversed(index);
        fflush(stderr);
        NotebookTab *nt = (NotebookTab *)item->pages->Item(index);
        if (nt) {
            ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)EVENT_ON_SWITCHPAGE), AnsiString((long)index));
        }
    }
}

void ClientEventHandler::on_itemActivated(QTreeWidgetItem *item, int column) {
    GTKControl *control = (GTKControl *)obj;//((CConceptClient *)this->Client)->Controls[this->ID.ToInt()];

    if ((control) && (control->type == CLASS_TREEVIEW)) {
        AnsiString param = ItemToPath((QTreeWidget *)control->ptr, item);
        ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)EVENT_ON_ROWACTIVATED), param);
        if ((((CConceptClient *)this->Client)->parser) && (entry_point.Length()))
            ((CConceptClient *)this->Client)->parser->NotifyEvent(entry_point, this->ID.ToInt(), param);
    }
}

void ClientEventHandler::on_timer() {
    if (timer) {
        ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)EVENT_ON_TIMER), AnsiString((long)timer->timerId()));
        timer->stop();
        delete timer;
        timer = NULL;
    }
}

void ClientEventHandler::on_customContextMenuRequested(const QPoint& point) {
    if (popup) {
        GTKControl *item = (GTKControl *)obj;//((CConceptClient *)this->Client)->Controls[this->ID.ToInt()];
        if ((item) && (item->ptr)) {
            QWidget *widget = (QWidget *)item->ptr;
            popup->exec(widget->mapToGlobal(point));
        }
    }
}

void ClientEventHandler::on_closeEvent(QCloseEvent *ev) {
    GTKControl *control = (GTKControl *)obj;//((CConceptClient *)this->Client)->Controls[this->ID.ToInt()];

    if ((control) && (control->type == CLASS_FORM)) {
        if (((CConceptClient *)this->Client)->CLIENT_SOCKET < 0)
            Done((CConceptClient *)this->Client, 0, true);
        ev->ignore();
    }
    AnsiString param((long)timer->timerId());
    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)EVENT_ON_DELETEEVENT), param);
    if ((((CConceptClient *)this->Client)->parser) && (entry_point.Length()))
        ((CConceptClient *)this->Client)->parser->NotifyEvent(entry_point, this->ID.ToInt(), param);
}

QTreeWidgetItem *ClientEventHandler::SearchTreeItem(QTreeWidgetItem *root, const QString& text, int column, int match) {
    if (root) {
        GTKControl *item = (GTKControl *)obj;
        EventAwareWidget<QTreeWidget> *tree = (EventAwareWidget<QTreeWidget> *)item->ptr;
        int  count = root->childCount();
        bool found = false;
        for (int i = 0; i < count; i++) {
            QTreeWidgetItem *e = root->child(i);
            if (e) {
                QString data  = e->data(column, Qt::DisplayRole).toString();
                QString dataU = e->data(column, Qt::UserRole).toString();


                if ((data.contains(text, Qt::CaseInsensitive)) || (dataU.contains(text, Qt::CaseInsensitive))) {
                    if (!match)
                        return e;
                    match--;
                }/* else {
                    QWidget *widget=tree->itemWidget(e, column);
                    if (widget) {
                        QLabel *label=qobject_cast<QLabel *>(widget);
                        if (label) {
                            data=label->text();
                            if (data.contains(text, Qt::CaseInsensitive)) {
                                if (!match)
                                    return e;
                                match--;
                            }
                        }
                    }
                    }*/

                e = SearchTreeItem(e, text, column, match);
                if (e)
                    return e;
            }
        }
    }
    return NULL;
}

bool ClientEventHandler::SearchItemIndex(const QString& text, int match_count) {
    GTKControl *item = (GTKControl *)obj;

    if ((item) && (item->type == CLASS_TREEVIEW) && (text.length())) {
        EventAwareWidget<QTreeWidget> *tree = (EventAwareWidget<QTreeWidget> *)item->ptr;
        if ((tree) && (tree->ratio >= 0)) {
            int column = tree->ratio;
            if (column < tree->columnCount()) {
                QTreeWidgetItem *root = tree->invisibleRootItem();
                if (root) {
                    root = SearchTreeItem(root, text, column, match_count);
                    tree->setCurrentItem(root);
                    if (root)
                        return true;
                }
            }
        }
    }
    return false;
}

void ClientEventHandler::on_textEdited(const QString& text) {
    GTKControl *item = (GTKControl *)obj;

    if ((item) && (item->type == CLASS_TREEVIEW) && (text.length())) {
        EventAwareWidget<QTreeWidget> *tree = (EventAwareWidget<QTreeWidget> *)item->ptr;
        if ((tree) && (tree->ratio >= 0)) {
            int column = tree->ratio;
            if (column < tree->columnCount()) {
                cacheval = 0;
                QTreeWidgetItem *root = tree->invisibleRootItem();
                if (root) {
                    root = SearchTreeItem(root, text, column, 0);
                    tree->setCurrentItem(root);
                }
            }
        }
    }
    if (Events[EVENT_ON_INSERTATCURSOR])
        ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)EVENT_ON_INSERTATCURSOR), AnsiString((char *)text.toUtf8().data()));
}

void ClientEventHandler::on_linkActivated(const QString& link) {
    AnsiString param((char *)link.toUtf8().data());

    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)EVENT_ON_CLICKED), param);
    if ((((CConceptClient *)this->Client)->parser) && (entry_point.Length()))
        ((CConceptClient *)this->Client)->parser->NotifyEvent(entry_point, this->ID.ToInt(), param);
}

void ClientEventHandler::on_orientationChanged(Qt::Orientation o) {
    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)EVENT_ON_ORIENTATIONCHANGED), AnsiString((long)o));
}

void ClientEventHandler::on_returnPressed() {
    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)EVENT_ON_ACTIVATE), "");
    if ((((CConceptClient *)this->Client)->parser) && (entry_point.Length()))
        ((CConceptClient *)this->Client)->parser->NotifyEvent(entry_point, this->ID.ToInt(), "");
}

void ClientEventHandler::on_textChanged() {
    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)EVENT_ON_INSERTATCURSOR), "");
}

void ClientEventHandler::on_selectionChanged() {
    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)EVENT_ON_SETANCHOR), "");
}

void ClientEventHandler::on_valueChanged(int i) {
    if (i == cacheval)
        return;
    GTKControl *item = (GTKControl *)obj;

    int step = ((QAbstractSlider *)item->ptr)->pageStep();
    int max  = ((QAbstractSlider *)item->ptr)->maximum();

    cacheval = i;
    AnsiString res((long)i);
    res += ":";
    if (step > max)
        step = 0;
    res += AnsiString((long)0);
    res += ":";
    res += AnsiString((long)max);
    res += ":";
    res += AnsiString((long)step);
    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)EVENT_ON_VALUECHANGED), res);
}

void ClientEventHandler::on_valueChanged3(int i) {
    GTKControl *item = (GTKControl *)obj;

    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)EVENT_ON_VALUECHANGED), AnsiString((long)i));
}

void ClientEventHandler::on_textChanged(const QString& text) {
    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)EVENT_ON_CHANGED), AnsiString((char *)text.toUtf8().data()));
}

void ClientEventHandler::on_currentIndexChanged(int index) {
    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)EVENT_ON_CHANGED), AnsiString((long)index));
}

void ClientEventHandler::on_currentIndexChanged(const QString& text) {
    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)EVENT_ON_CHANGED), AnsiString((char *)text.toUtf8().constData()));
}

void ClientEventHandler::on_topLevelChanged(bool d) {
    if (d)
        ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)EVENT_ON_CHILDDETACHED), "");
    else
        ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)EVENT_ON_CHILDATTACHED), "");
}

void ClientEventHandler::on_currentItemChanged(QTreeWidgetItem *item, QTreeWidgetItem *prev) {
    GTKControl *control = (GTKControl *)obj;

    AnsiString path("-1");

    if (item) {
        path = ItemToPath((QTreeWidget *)control->ptr, item);
    }
    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)EVENT_ON_CURSORCHANGED), path);
}

void ClientEventHandler::on_itemActivated2(QListWidgetItem *item) {
    GTKControl *control = (GTKControl *)obj;

    if ((control) && (control->type == CLASS_ICONVIEW)) {
        int        row = ((QListWidget *)control->ptr)->row(item);
        AnsiString param((long)row);
        ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)EVENT_ON_ROWACTIVATED), param);
        if ((((CConceptClient *)this->Client)->parser) && (entry_point.Length()))
            ((CConceptClient *)this->Client)->parser->NotifyEvent(entry_point, this->ID.ToInt(), param);
    }
}

void ClientEventHandler::on_itemCollapsed(QTreeWidgetItem *item) {
    GTKControl *control = (GTKControl *)obj;

    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)EVENT_ON_ROWCOLLAPSED), ItemToPath((QTreeWidget *)control->ptr, item));
}

void ClientEventHandler::on_itemExpanded(QTreeWidgetItem *item) {
    GTKControl *control = (GTKControl *)obj;

    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)EVENT_ON_ROWEXPANDED), ItemToPath((QTreeWidget *)control->ptr, item));
}

void ClientEventHandler::on_dayClicked(const QDate& date) {
    QString s = date.toString("yyyy-MM-dd");

    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)EVENT_ON_DAYSELECTED), AnsiString((char *)s.toUtf8().data()));
}

void ClientEventHandler::on_dayDBClicked(const QDate& date) {
    QString s = date.toString("yyyy-MM-dd");

    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)EVENT_ON_DAYSELECTEDDBCLICK), AnsiString((char *)s.toUtf8().data()));
}

void ClientEventHandler::on_monthChanged(int year, int month) {
    AnsiString res = AnsiString((long)year);

    res += "-";
    res += AnsiString((long)month);
    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)EVENT_ON_MONTHCHANGED), res);
    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)EVENT_ON_NEXTMONTH), res);
    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)EVENT_ON_PREVMONTH), res);
}

void ClientEventHandler::on_sliderMoved(int val) {
    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)EVENT_ON_MOVESLIDER), AnsiString((long)val));
}

void ClientEventHandler::on_sectionClicked(int column) {
    GTKControl *item = (GTKControl *)obj;

    ((TreeData *)item->ptr3)->extra = column;
    ((QTreeWidget *)item->ptr)->setCurrentItem(((QTreeWidget *)item->ptr)->currentItem(), column);
    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)EVENT_ON_HEADERCLICKED), AnsiString((long)column));
}

void ClientEventHandler::on_currentRowChanged(int row) {
    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)EVENT_ON_CHANGED), AnsiString((long)row));
}

void ClientEventHandler::on_valueChanged2(int i) {
    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)EVENT_ON_CHANGEVALUE), AnsiString((long)i));
}

void ClientEventHandler::on_columnResized(int index, int oldsize, int newsize) {
    AnsiString res((long)index);

    res += ":";
    res += AnsiString((long)newsize);
    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)EVENT_ON_COLUMNRESIZED), res);
}

void ClientEventHandler::on_geometriesChanged() {
    GTKControl  *item   = (GTKControl *)obj;
    QHeaderView *header = ((QTreeWidget *)item->ptr)->header();

    int len = ((TreeData *)item->ptr3)->meta.size();

    for (int i = 0; i < len; i++) {
        TreeColumn *tc = ((TreeData *)item->ptr3)->meta[i];
        if ((tc) && (tc->widget) && (header)) {
            int height = header->height();
            int width  = header->sectionSize(tc->index);

            int h2 = tc->widget->height();
            int w2 = tc->widget->width();
            if ((height > 0) && (h2 > 0)) {
                int pos = header->sectionViewportPosition(tc->index);
#ifdef __APPLE__
                tc->widget->move(pos + (width - w2) / 2 + 4, (height - h2) / 2);
#else
                tc->widget->move(pos + (width - w2) / 2 + 1, (height - h2) / 2);
#endif
            }
        }
    }
}

void ClientEventHandler::on_itemCollapseExpand(QTreeWidgetItem *item) {
    on_geometriesChanged();
}

void ClientEventHandler::on_sectionResized(int index, int oldsize, int newsize) {
    GTKControl  *item   = (GTKControl *)obj;
    QHeaderView *header = ((QTreeWidget *)item->ptr)->header();

    index = header->visualIndex(index);

    if (index < ((TreeData *)item->ptr3)->meta.size()) {
        int len = ((TreeData *)item->ptr3)->meta.size();
        for (int i = 0; i < len; i++) {
            TreeColumn *tc = ((TreeData *)item->ptr3)->meta[i];
            if ((tc) && (tc->widget) && (header) && (index <= tc->index)) {
                int height = header->height();
                int width  = header->sectionSize(tc->index);
                if (tc->index == index)
                    width = newsize;

                int h2 = tc->widget->height();
                int w2 = tc->widget->width();
                if ((height > 0) && (h2 > 0)) {
                    int pos = header->sectionViewportPosition(tc->index);
                    //if (index<tc->index)
                    //    pos+=newsize-oldsize;

#ifdef __APPLE__
                    tc->widget->move(pos + (width - w2) / 2 + 4, (height - h2) / 2);
#else
                    tc->widget->move(pos + (width - w2) / 2 + 1, (height - h2) / 2);
#endif
                }
            }
        }
    }
}

void ClientEventHandler::ResetDrop() {
    last_dropped_data = (char *)"";
}

int ClientEventHandler::FileIsDropped(AnsiString filename) {
    int  len  = last_dropped_data.Length();
    char *str = last_dropped_data.c_str();

    if (!len)
        return 0;

    AnsiString scanned_file;
    for (int i = 0; i < len; i++) {
        char c = str[i];
        if (c == '\r')
            continue;
        if (c == '\n') {
            if ((scanned_file.Length()) && (filename == scanned_file))
                return 1;
            else
                scanned_file = (char *)"";
            continue;
        }
        scanned_file += c;
    }
    if ((scanned_file.Length()) && (scanned_file == filename))
        return 1;
    return 0;
}

void ClientEventHandler::on_itemChanged(QTreeWidgetItem *item, int column) {
    GTKControl *control = (GTKControl *)obj;
    AnsiString data     = ItemToPath((QTreeWidget *)control->ptr, item);

    data += "/";
    data += AnsiString((long)column);
    if (control->ptr3)
        ((TreeData *)control->ptr3)->extra = column;

    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)EVENT_ON_STARTEDITING), data.c_str());
}

void ClientEventHandler::on_itemChanged2(QTreeWidgetItem *item, int column) {
    GTKControl *control = (GTKControl *)obj;
    TreeColumn *td      = ((TreeData *)control->ptr3)->GetColumn(column);
    QString    s;

    if (td) {
        switch (td->type) {
            case 0x02:
            case 0x82:
            case 0x03:
            case 0x83:
            case 0x04:
            case 0x84:
                s = item->data(column, Qt::UserRole).toString();
                break;

            default:
                s = item->data(column, Qt::DisplayRole).toString();
        }
    } else
        s = item->data(column, Qt::DisplayRole).toString();
    if (control->ptr3)
        ((TreeData *)control->ptr3)->extra = column;
    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)EVENT_ON_ENDEDITING), AnsiString((char *)s.toUtf8().constData()));
}

void ClientEventHandler::on_unsupportedContent(QNetworkReply *reply) {
    // to do
}

void ClientEventHandler::on_urlChanged(const QUrl& url) {
    QString surl = url.toString();

    if (url.isLocalFile()) {
        if (surl.indexOf(QString("file:///static/")) == 0)
            surl = surl.remove(0, 15);
    }
    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)360), AnsiString((char *)surl.toUtf8().constData()));
}

void ClientEventHandler::on_urlChanged2(const QUrl& url) {
    QString surl = url.toString();

    if (url.isLocalFile()) {
        if (surl.indexOf(QString("file:///static/")) == 0)
            surl = surl.remove(0, 15);
    }
    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)350), AnsiString((char *)surl.toUtf8().constData()));
}

void ClientEventHandler::on_linkHovered(const QString& link, const QString& title, const QString& content) {
    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)354), AnsiString((char *)link.toUtf8().constData()));
}

void ClientEventHandler::on_iconChanged() {
#ifndef NO_WEBKIT
    GTKControl *item = (GTKControl *)obj;
    QIcon      icon  = ((QWebView *)item->ptr)->icon();

    QPixmap pixmap = icon.pixmap(32, 32);

    AnsiString buficon;

    if (!pixmap.isNull()) {
        QByteArray bArray;
        QBuffer    buffer(&bArray);
        buffer.open(QIODevice::WriteOnly);
        pixmap.save(&buffer, "PNG");

        buficon.LoadBuffer((char *)bArray.constData(), bArray.size());
    }

    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)355), buficon);
#endif
}

void ClientEventHandler::on_loadStarted() {
    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)359), "");
}

void ClientEventHandler::on_loadStarted2() {
    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)356), "");
}

void ClientEventHandler::on_loadFinished(bool ok) {
    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)357), AnsiString((long)ok));
}

void ClientEventHandler::on_loadProgress(int p) {
    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)358), AnsiString((long)p));
}

void ClientEventHandler::on_selectionChanged2() {
    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)365), "");
}

void ClientEventHandler::on_statusBarMessage(const QString& msg) {
    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)366), AnsiString((char *)msg.toUtf8().constData()));
}

void ClientEventHandler::on_titleChanged(const QString& title) {
    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)367), AnsiString((char *)title.toUtf8().constData()));
}

void ClientEventHandler::on_downloadRequested(const QNetworkRequest& request) {
    QUrl url = request.url();

    QString surl = url.toString();

    if (url.isLocalFile()) {
        if (surl.indexOf(QString("file:///static/")) == 0)
            surl = surl.remove(0, 15);
    }
    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)372), AnsiString((char *)surl.toUtf8().constData()));
}

void ClientEventHandler::on_TreeWidgetCombo_currentIndexChanged(const QString& val) {
    GTKControl  *control = (GTKControl *)obj;
    QTreeWidget *widget  = (QTreeWidget *)control->ptr;

    QTreeWidgetItem *item = widget->currentItem();

    AnsiString data = ItemToPath(widget, item);

    data += "/";
    data += AnsiString((long)widget->currentColumn());
    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)EVENT_ON_STARTEDITING), data.c_str());

    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)EVENT_ON_ENDEDITING), AnsiString((char *)val.toUtf8().constData()));
}

void ClientEventHandler::on_mediaStatusChanged(QMediaPlayer::MediaStatus status) {
    if ((status == QMediaPlayer::EndOfMedia) || (status == QMediaPlayer::InvalidMedia))
        ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)353), "");
}

void ClientEventHandler::on_error(QMediaPlayer::Error error) {
    if (error == QMediaPlayer::FormatError)
        ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)354), "The format of a media resource isn't (fully) supported. Playback may still be possible, but without an audio or video component.");
}

void ClientEventHandler::on_error2(QMediaPlayer::Error error) {
    if (error != QMediaPlayer::FormatError) {
        switch (error) {
            case QMediaPlayer::NoError:
                break;

            case QMediaPlayer::ResourceError:
                ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)355), "A media resource couldn't be resolved.");
                break;

            case QMediaPlayer::NetworkError:
                ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)355), "A network error occurred.");
                break;

            case QMediaPlayer::AccessDeniedError:
                ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)355), "There are not the appropriate permissions to play a media resource.");
                break;

            case QMediaPlayer::ServiceMissingError:
                ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)355), "A valid playback service was not found, playback cannot proceed.");
                break;

            default:
                ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)355), "");
        }
    }
}

void ClientEventHandler::on_imageCaptured(int requestId, const QImage& img) {
    this->img = img.copy();
}

/*void ClientEventHandler::on_frameCaptured(int requestId, const QImage& img) {
    const QImage::Format FORMAT = QImage::Format_RGB32;
    QImage img2;

    if (img.format() != QImage::Format_RGB32)
        img2 = img.convertToFormat(FORMAT);
    else
        img2 = img;

    int size = img2.width() * img2.height();
    unsigned char *buffer = (unsigned char *)malloc(((size * 3) / 2) + 100);
    RGBtoYUV420P(img2.bits(), buffer, img2.depth() / 8, true, img2.width(), img2.height(), false);
    AnsiString buf;

    buf.LinkBuffer((char *)buffer, size);
    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, "350", buf);
   }*/

void ClientEventHandler::on_propertyChanged(QtProperty *prop) {
    GTKControl *item = (GTKControl *)obj;

    if (item->flags)
        return;

    QtTreePropertyBrowser *widget = (QtTreePropertyBrowser *)item->ptr;
    QString res;
    if (widget->property("returnindex").toBool())
        res = QString::number(((QtVariantProperty *)prop)->propertyIndex);
    else
        res += prop->propertyName();
    res += "=";
    if (((QtVariantProperty *)prop)->valueType() == QVariant::Color) {
        QColor color = ((QtVariantProperty *)prop)->value().value<QColor>();
        double val   = (unsigned long)color.alpha() * 0x1000000;
        val += color.red() * 0x10000;
        val += color.green() * 0x100;
        val += color.blue();

        res += AnsiString(val);
    } else
    if (((QtVariantProperty *)prop)->valueType() == QVariant::Bool) {
        res += AnsiString((long)((QtVariantProperty *)prop)->value().toInt());
    } else
        res += prop->valueText();
    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)EVENT_ON_CHANGED), AnsiString((char *)res.toUtf8().constData()));
}

void ClientEventHandler::on_cellActivated(int row, int column) {
    AnsiString pos((long)row);

    pos += ";";
    pos += AnsiString((long)column);

    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)350), pos);
}

void ClientEventHandler::on_cellChanged(int row, int column) {
    AnsiString pos((long)row);

    pos += ";";
    pos += AnsiString((long)column);

    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)351), pos);
}

void ClientEventHandler::on_itemSelectionChanged() {
    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)358), "");
}

void ClientEventHandler::on_currentCellChanged(int row, int column, int prev_row, int prev_column) {
    AnsiString pos((long)row);

    pos += ";";
    pos += AnsiString((long)column);

    ((CConceptClient *)this->Client)->SendMessageNoWait(this->ID.c_str(), MSG_EVENT_FIRED, AnsiString((long)360), pos);
}

void ClientEventHandler::on_itemEntered(QTreeWidgetItem *item, int column) {
    GTKControl *eitem = (GTKControl *)obj;

    if ((eitem) && (item)) {
        ((QTreeWidget *)eitem->ptr)->setCurrentItem(item);
        ((TreeData *)eitem->ptr3)->extra = column;
    }
}

void ClientEventHandler::on_rangeChanged(int min, int max) {
    GTKControl *eitem = (GTKControl *)obj;

    ((QAbstractScrollArea *)eitem->ptr)->verticalScrollBar()->setValue(max);
}

void ClientEventHandler::sslErrorHandler(QNetworkReply *qnr, const QList<QSslError>& errlist) {
    qnr->ignoreSslErrors();
}

void ClientEventHandler::on_closeLink(const QString&) {
    exit(0);
}
