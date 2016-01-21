#ifndef MANAGED_PAGE_H
#define MANAGED_PAGE_H

#include <QWebPage>
#include <QString>
#include <QMap>

#include "EventAwareWidget.h"

class ManagedPage : public QWebPage {
private:
    GTKControl *item;
public:
    QMap<int, bool> responses;
    bool            ignoreNext;
    QString         userAgent;

    ManagedPage(GTKControl *obj, QWidget *parent) : QWebPage(parent) {
        item       = obj;
        ignoreNext = false;
    }

    QString userAgentForUrl(const QUrl& url) const {
        if (userAgent.size())
            return userAgent;
        return QWebPage::userAgentForUrl(url);
    }

    virtual bool acceptNavigationRequest(QWebFrame *frame, const QNetworkRequest& request, NavigationType type) {
        QUrl url = request.url();

        if ((item->type == 1012) && (EVENT_REGISTERED(360))) {
            QString surl = url.toString();
            if (url.isLocalFile()) {
                if (surl.indexOf(QString("file:///static/")) == 0)
                    surl = surl.remove(0, 15);
            }
            NOTIFY_EVENT(360, (char *)surl.toUtf8().constData());

            if (responses.contains(360)) {
                if (!responses[360])
                    return QWebPage::acceptNavigationRequest(frame, request, type);
                return false;
            }
        } else
        if ((item->type == 1001) && (EVENT_REGISTERED(350))) {
            if (ignoreNext) {
                ignoreNext = false;
                return QWebPage::acceptNavigationRequest(frame, request, type);
            }
            QString surl = url.toString();
            if (url.isLocalFile()) {
                if (surl.indexOf(QString("file:///static/")) == 0)
                    surl = surl.remove(0, 15);
            }
            NOTIFY_EVENT(350, (char *)surl.toUtf8().constData());
            if (responses.contains(350)) {
                if (!responses[350])
                    return QWebPage::acceptNavigationRequest(frame, request, type);
                return false;
            }
        } else {
            if (responses.contains(360)) {
                if (!responses[360])
                    return QWebPage::acceptNavigationRequest(frame, request, type);
                return false;
            }
        }
        return QWebPage::acceptNavigationRequest(frame, request, type);
    }

    virtual void javaScriptConsoleMessage(const QString& message, int lineNumber, const QString& sourceID) {
        if (item->type == 1012) {
            NOTIFY_EVENT(350, (char *)message.toUtf8().constData());
        }
        QWebPage::javaScriptConsoleMessage(message, lineNumber, sourceID);
    }

    virtual bool javaScriptPrompt(QWebFrame *frame, const QString& msg, const QString& defaultValue, QString *result) {
        NOTIFY_EVENT(369, (char *)msg.toUtf8().constData());
        if (responses.contains(369))
            return responses[369];
        return QWebPage::javaScriptPrompt(frame, msg, defaultValue, result);
    }

    virtual bool javaScriptConfirm(QWebFrame *frame, const QString& msg) {
        NOTIFY_EVENT(370, (char *)msg.toUtf8().constData());
        if (responses.contains(370))
            return responses[370];
        return QWebPage::javaScriptConfirm(frame, msg);
    }

    virtual void javaScriptAlert(QWebFrame *frame, const QString& msg) {
        NOTIFY_EVENT(363, (char *)msg.toUtf8().constData());
        QWebPage::javaScriptAlert(frame, msg);
    }

    ~ManagedPage() {
    }
};
#endif
