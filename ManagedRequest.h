#ifndef MANAGED_REQUEST_H
#define MANAGED_REQUEST_H

#include <QNetworkAccessManager>
#include <QMap>
#include "AnsiString.h"

class ManagedRequest : public QNetworkAccessManager {
private:
    QString temp_dir;
public:
    bool caseSensitive;
    QMap<QString, QByteArray> cache;

    ManagedRequest(QString temp) : QNetworkAccessManager() {
        temp_dir      = temp;
        caseSensitive = false;
    }

    static char *Ext(char *path, int len) {
        char *res = 0;

        for (int i = len - 1; i >= 0; i--) {
            if ((path[i] == '/') || (path[i] == '\\') || (path[i] == ':'))
                return res;

            if (path[i] == '.') {
                if (i != len - 1)
                    return path + i;
                else
                    return res;
            }
        }
        return res;
    }

    virtual QNetworkReply *createRequest(Operation op, const QNetworkRequest& req, QIODevice *outgoingData = 0) {
        if (cache.size()) {
            QUrl    url   = req.url();
            QString path3 = url.toString();

            if ((!cache.contains(path3)) && (url.isLocalFile())) {
                path3 = url.path();
                if (!cache.contains(path3)) {
                    if ((path3.size()) && (path3.at(0) == '/')) {
                        path3 = path3.remove(0, 1);
                        if (path3.indexOf(QString("static/")) == 0)
                            path3 = path3.remove(0, 7);
                    }
                }
            }
            if (cache.contains(path3)) {
                const char *s       = path3.toUtf8().data();
                char       *ext     = Ext((char *)s, strlen(s));
                QString    filename = temp_dir;
                filename += "webkit.cache.";

                int i = 0;
                QMap<QString, QByteArray>::const_iterator it = cache.constBegin();
                while (it != cache.constEnd()) {
                    if (it.key() == path3)
                        break;

                    ++it;
                    i++;
                }

                filename += QString::number(i);
                if (ext)
                    filename += ext;
                else
                    filename += ".jpg";

                QFile file(filename);
                file.open(QIODevice::WriteOnly);
                file.write(cache[path3]);
                file.close();

                // ugly!
                ((QNetworkRequest *)&req)->setUrl(QUrl::fromLocalFile(filename));
            }
        }
        return QNetworkAccessManager::createRequest(op, req, outgoingData);
    }

    ~ManagedRequest() {
    }
};
#endif
