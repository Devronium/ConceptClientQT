#ifndef MACOSXAPPLICATION_H
#define MACOSXAPPLICATION_H
 
#include <QApplication>
#include <QFileOpenEvent>
#include <QEvent>
#include "AnsiString.h"

typedef bool (*OSX_OPEN_CALLBACK)(const char *fname, void *user_data);

typedef struct {
    int step;
    void *userdata;
    AnsiString data;
} MacOSXStep;

class MacOSXApplication : public QApplication{
    Q_OBJECT

    OSX_OPEN_CALLBACK open_callback;
public:
    void *userdata;
    
    MacOSXApplication(OSX_OPEN_CALLBACK open_callback, void *user_data, int &argc, char **argv) : QApplication(argc, argv){
        this->open_callback = open_callback;
        this->userdata = user_data;
    }
 
protected:
    bool event(QEvent *ev){ // the open file event handler
        if (ev->type() == QEvent::FileOpen) {
            const QString file_name = static_cast<QFileOpenEvent*>(ev)->file();
            if (open_callback(file_name.toUtf8().data(), userdata))
                return true;
        }
        return QApplication::event(ev);
    }
};
 
#endif // MACOSXAPPLICATION_H
