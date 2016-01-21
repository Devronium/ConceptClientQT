#ifndef WORKER_H
#define WORKER_H

#include <QObject>
#include <QMutex>
#include <QQueue>
#include "ConceptClient.h"

class Worker : public QObject
{
    Q_OBJECT

public:
    explicit Worker(CConceptClient *CC);
    void requestWork();
    void abort();
    void emitTransferNotify(int percent, int status, bool idle_call);
    void emitMessage(int message);

private:
    CConceptClient        *CC;
    bool                  _abort;
    bool                  _working;
    int                   _instances;
    QMutex                mutex;
    QMutex                mutex2;
    QQueue<TParameters *> queue;

signals:
    void workRequested();
    void valueChanged(QQueue<TParameters *> *queue, QMutex *mutex2);
    void transferNotify(int percent, int status, bool idle_call);
    void Message(int message);
    void finished();

public slots:
    void doWork();
    void doSend();
    void callback(QQueue<TParameters *> *queue, QMutex *mutex2);
    void doTransferNotify(int percent, int status, bool idle_call);
    void doMessage(int message);
};
#endif // WORKER_H
