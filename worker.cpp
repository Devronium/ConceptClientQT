#include "worker.h"
#include <QTimer>
#include <QEventLoop>
#include "callback.h"
#include "messages.h"

#include <QThread>
#include <QMessageBox>
#include <QIcon>
#include "consolewindow.h"


Worker::Worker(CConceptClient *CC) : QObject(NULL) {
    this->CC   = CC;
    _working   = true;
    _abort     = false;
    _instances = 0;
}

void Worker::requestWork() {
    mutex.lock();
    _working = true;
    _abort   = false;
    mutex.unlock();

    emit workRequested();
}

void Worker::abort() {
    mutex.lock();
    if (_working) {
        _abort = true;
    }
    mutex.unlock();
}

void Worker::doSend() {
    while (true) {
        CC->SendPending();
        QThread::msleep(10);
    }
}

void Worker::doWork() {
    char *localCERT = CC->secured ? CC->LOCAL_PRIVATE_KEY.c_str() : 0;
    int  res;
    //QWaitCondition done;

    TParameters *PARAM      = new TParameters;
    Worker      *emitWorker = (Worker *)GetWorker();

    while (true) {
        PARAM->Owner    = CC;
        PARAM->UserData = 0;
        PARAM->Flags    = 0;
        if (!have_messages(CC, CC->CLIENT_SOCKET)) {
            if (CC->LastObject) {
                PARAM->ID = MSG_FLUSH_UI;
                mutex2.lock();
                queue.enqueue(PARAM);
                mutex2.unlock();

                emit valueChanged(&queue, &mutex2);
                PARAM = new TParameters;
            }
            CC->SendPending();
            continue;
        }

        res = get_message(CC, PARAM, CC->CLIENT_SOCKET, localCERT, (PROGRESS_API)CC->notify, false);
        if (res == -2)
            continue;

        if ((res <= 0) && (CC->MainForm)) {
            int reconnect_res = 0;
            // 60 retries
            for (int i = 0; i < 20; i++) {
                reconnect_res = CC->CheckReconnect();

                if ((reconnect_res < 0) || (reconnect_res == 1))
                    break;

                if (CC->reconnectInfo)
                    CC->reconnectInfo(i + 1);
#ifdef _WIN32
                Sleep(1000);
#else
                sleep(1);
#endif
            }
            // reconnected !
            if (reconnect_res == 1) {
                localCERT = CC->secured ? CC->LOCAL_PRIVATE_KEY.c_str() : 0;
                if (CC->reconnectInfo)
                    CC->reconnectInfo(0);
                continue;
            }
            if (CC->reconnectInfo)
                CC->reconnectInfo(-1);
        }
        if (res <= 0) {
            PARAM->ID    = 0x500;
            PARAM->Flags = 1;

            mutex2.lock();
            queue.enqueue(PARAM);
            mutex2.unlock();
            emit valueChanged(&queue, &mutex2);
            break;
        }
        CC->msg_count++;
        mutex2.lock();
        queue.enqueue(PARAM);
        mutex2.unlock();

        emit valueChanged(&queue, &mutex2);
        PARAM = new TParameters;
        //done.wait(&mutex2, 10000);
        CC->SendPending();
    }
    mutex.lock();
    _working = false;
    mutex.unlock();

    emit finished();
}

void Worker::emitTransferNotify(int percent, int status, bool idle_call) {
    emit transferNotify(percent, status, idle_call);
}

void Worker::emitMessage(int message) {
    emit Message(message);
}

void Worker::doTransferNotify(int percent, int status, bool idle_call) {
    BigMessageNotify(CC, percent, status, idle_call);
}

void Worker::doMessage(int message) {
    ReconnectMessage(message);
}

void Worker::callback(QQueue<TParameters *> *queue, QMutex *mutex2) {
    mutex2->lock();
    TParameters *msg = queue->dequeue();
    mutex2->unlock();
    int res = MESSAGE_CALLBACK(msg);

    if (res == 1) {
        if (msg->Flags) {
            AnsiString    err("Error opening applicaiton <b>");
            ConsoleWindow *cw = (ConsoleWindow *)msg->Owner->UserData;
            err += msg->Owner->lastApp;
            err += "</b>.<br/><br/>This could indicate a missing or broken application.";
            cw->Started();
            QMessageBox msg(QMessageBox::Warning, "Error opening application", err.c_str(), QMessageBox::Ok);
            msg.setWindowIcon(QIcon(QString::fromUtf8(":/resources/conceptclienticon.png")));
            msg.exec();
        }
        delete msg;
        //exit(0);
        Done(msg->Owner, 0);
    }
    delete msg;
    // at least one call
    _instances = 1;
}
