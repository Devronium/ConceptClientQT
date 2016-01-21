#ifndef __SCINTILLAEVENTHANDLER_H
#define __SCINTILLAEVENTHANDLER_H

#include <QObject>
#include <ScintillaEditBase.h>

typedef void (*NOTIFY_FUNCTION)(ScintillaEditBase *widget, SCNotification *scn);

class ScintillaEventHandler : public QObject {
    Q_OBJECT
protected:
    ScintillaEditBase * widget;
    NOTIFY_FUNCTION notify;
public:
    ScintillaEventHandler(ScintillaEditBase *owner, NOTIFY_FUNCTION func) : QObject() {
        widget = owner;
        notify = func;
    }

public slots:
    void on_notify(SCNotification *scn) {
        notify(widget, scn);
    }
};
#endif
