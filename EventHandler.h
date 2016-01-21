#ifndef __EVENTHANDLER_H
#define __EVENTHANDLER_H

#include "AnsiString.h"
#include <QObject>
#include <QMouseEvent>
#include <QTreeWidgetItem>
#include <QListWidgetItem>
#include <QTimer>
#include <QMenu>
#include <QDate>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QMediaPlayer>
#include "PropertiesBox/qtvariantproperty.h"

#include <map>

#define REGISTER_EVENT(CLIENT_EVENT)                                     \
    {                                                                    \
        ClientEventHandler *events = (ClientEventHandler *)item->events; \
        if (!events) {                                                   \
            events       = new ClientEventHandler(item->ID, Client);     \
            item->events = events;                                       \
        }                                                                \
        events->RegisterEvent(CLIENT_EVENT);                             \
    }


class ClientEventHandler : public QObject
{
    Q_OBJECT

private:
    AnsiString ID;
    void *obj;
    std::map<int, bool> Events;
public:
    int    cacheval;
    void   *Client;
    QMenu  *popup;
    QTimer *timer;
    QImage img;
    ClientEventHandler(long, void *);
    AnsiString last_dropped_data;
    AnsiString entry_point;

    void SendEvent(int CONCEPT_EVENT, AnsiString EventData);
    void RegisterEvent(int CONCEPT_EVENT);
    void UnregisterEvent(int CONCEPT_EVENT);
    bool IsRegistered(int CONCEPT_EVENT);

    void ResetDrop();
    int FileIsDropped(AnsiString filename);
    QTreeWidgetItem *SearchTreeItem(QTreeWidgetItem *root, const QString& text, int column, int match_count);
    bool SearchItemIndex(const QString& text, int match_count);

public slots:
    void on_clicked();
    void on_toggled(bool toggled);
    void on_triggered(bool toggled);
    void on_activated(bool toggled);
    void on_pressed(bool toggled);
    void on_released(bool toggled);
    void on_pressed_secondary(bool toggled);
    void on_released_secondary(bool toggled);
    void on_tabCloseRequested(int index);
    void on_currentChanged(int index);
    void on_itemActivated(QTreeWidgetItem *item, int column);
    void on_timer();
    void on_customContextMenuRequested(const QPoint& point);
    void on_closeEvent(QCloseEvent *ev);
    void on_textEdited(const QString& text);
    void on_linkActivated(const QString& link);
    void on_orientationChanged(Qt::Orientation);
    void on_returnPressed();
    void on_textChanged();
    void on_selectionChanged();
    void on_textChanged(const QString& text);
    void on_currentIndexChanged(int);
    void on_topLevelChanged(bool);
    void on_currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *);
    void on_itemActivated2(QListWidgetItem *);
    void on_itemCollapsed(QTreeWidgetItem *);
    void on_itemExpanded(QTreeWidgetItem *);
    void on_dayClicked(const QDate&);
    void on_dayDBClicked(const QDate&);
    void on_monthChanged(int, int);
    void on_valueChanged(int);
    void on_sliderMoved(int);
    void on_sectionClicked(int);
    void on_sectionResized(int, int, int);
    void on_geometriesChanged();
    void on_itemCollapseExpand(QTreeWidgetItem *);
    void on_columnResized(int, int, int);
    void on_currentRowChanged(int);
    void on_valueChanged2(int);
    void on_valueChanged3(int);
    void on_itemChanged(QTreeWidgetItem *item, int column);
    void on_itemChanged2(QTreeWidgetItem *item, int column);

    void on_downloadRequested(const QNetworkRequest& request);
    void on_urlChanged(const QUrl& url);
    void on_urlChanged2(const QUrl& url);
    void on_unsupportedContent(QNetworkReply *reply);

    void on_linkHovered(const QString&, const QString&, const QString&);
    void on_iconChanged();
    void on_loadStarted();
    void on_loadStarted2();
    void on_loadFinished(bool ok);
    void on_loadProgress(int);
    void on_selectionChanged2();
    void on_statusBarMessage(const QString&);
    void on_titleChanged(const QString&);

    void on_TreeWidgetCombo_currentIndexChanged(const QString&);
    void on_currentIndexChanged(const QString&);

    void on_mediaStatusChanged(QMediaPlayer::MediaStatus status);
    void on_error(QMediaPlayer::Error error);
    void on_error2(QMediaPlayer::Error error);

    void on_imageCaptured(int requestId, const QImage& img);
    void on_propertyChanged(QtProperty *prop);

    void on_cellActivated(int, int);
    void on_cellChanged(int, int);
    void on_itemSelectionChanged();
    void on_currentCellChanged(int, int, int, int);

    void on_itemEntered(QTreeWidgetItem *, int);

    void on_rangeChanged(int min, int max);

    void sslErrorHandler(QNetworkReply *qnr, const QList<QSslError>& errlist);

    void on_closeLink(const QString&);
};
#endif //__EVENTHANDLER_H
