#include "ConceptClient.h"
#include "consolewindow.h"
#include "BasicMessages.h"
#include <QClipboard>
#include <QColorDialog>
#include <QFontDialog>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QVideoWidget>
#include <QCamera>
#include <QCameraInfo>
#include <QCameraImageCapture>
#include <QCameraViewfinder>
#include <QXmlStreamReader>
#include <QMovie>
#include <QSystemTrayIcon>
#include <QSettings>
#include <QResource>
#include <QFontDatabase>

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef _WIN32
 #include <QWindow>
 #include <QtWin>
#endif

#include "EventAwareWidget.h"
#include "BaseWidget.h"
#include "CustomButton.h"
#include "verticallabel.h"
#include "interface.h"
#include "logindialog.h"
#include "TreeDelegate.h"
#include "ListDelegate.h"
#include "unzip.h"
#include "messages.h"
#include "worker.h"
#include "stock_constants.h"
#include "PropertiesBox/qtpropertybrowser.h"
#include "PropertiesBox/qttreepropertybrowser.h"
#include "PropertiesBox/qtvariantproperty.h"
#include "cameraframegrabber.h"
#include "AudioObject.h"
#ifndef NO_WEBKIT
    #include <QWebFrame>
    #include <QWebInspector>
    #include <QJsonDocument>
    #include "ManagedRequest.h"
    #include "ManagedPage.h"
#endif

#include <locale.h>

#ifndef LITTLE_ENDIAN
    #define LITTLE_ENDIAN
#endif

#ifndef _WIN32
 #define _unlink    unlink
#endif

AnsiString      run_target;
AnsiString      run_value;
int             wait_thread    = 0;
int             _debug         = false;
bool            first_save     = true;
bool            first_open     = true;
Worker          *emitWorker    = NULL;
QProgressBar    *progress      = NULL;
QLabel          *reconnectInfo = NULL;
QMediaPlayer    *ring          = NULL;
QSystemTrayIcon *trayIcon      = NULL;
AnsiString      temp_dir;
QMap<QString, QMediaPlayer *> StaticAudioPlayers;
CConceptClient *lpClient = NULL;
static int     dialogs   = 0;

HMODULE controls[MAX_CONTROLS];
int     controls_count = 0;
INVOKER INVOKERS[MAX_CONTROLS];

extern "C" {
#define PROTO_LIST(PROTOTYPE)    PROTOTYPE
#ifndef UINT4
 #define UINT4    unsigned int
#endif
#include "md5.h"
#include "sha1.h"
#include "sha256.h"
}

class DHeaderView : public QHeaderView {
public:
    DHeaderView(QWidget *parent) : QHeaderView(Qt::Horizontal, parent) {
    }

    int sectionWidth(int index) {
        QSize s = sectionSizeFromContents(index);

        return s.width();
    }
};

#define TRAY_NOTIFY(title, text)    WIN32Notify(title, text);

void WIN32Notify(char *title, char *text) {
    if (!trayIcon) {
        trayIcon = new QSystemTrayIcon(QIcon(QString::fromUtf8(":/resources/conceptclienticon.png")));
        trayIcon->setToolTip(QString::fromUtf8("concept:// Client Notification"));
    }
    trayIcon->show();
    trayIcon->showMessage(QString::fromUtf8(title), QString::fromUtf8(text));
}

AnsiString Ext(AnsiString *fname) {
    AnsiString ext;
    int        blen = fname->Length();
    char       *bin = fname->c_str();

    if (blen) {
        for (int i = (int)blen - 1; i >= 0; i--) {
            if (bin[i] == '.')
                break;
            ext = AnsiString((char)tolower(bin[i])) + ext;
        }
    }
    return ext;
}

void LoadFonts() {
    QResource  r(":/resources/fonts/fontawesome-webfont.ttf");
    QByteArray fontData(reinterpret_cast<const char *>(r.data()), r.size());
    QFontDatabase::addApplicationFontFromData(fontData);
}

#ifdef _WIN32
wchar_t *wstr(const char *utf8) {
    int     len    = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, NULL, 0);
    wchar_t *utf16 = (wchar_t *)malloc((len + 1) * sizeof(wchar_t));

    if (utf16) {
        int size = MultiByteToWideChar(CP_UTF8, 0, utf8, -1, utf16, len);
        if (size == (size_t)(-1)) {
            free(utf16);
            utf16 = NULL;
        }
    }
    return utf16;
}
#endif

void LoadControls(void *CC, const char *dir) {
    lpClient = (CConceptClient *)CC;
    DIR           *dr = opendir(dir);
    struct dirent *dit;
    LoadFonts();
    controls_count = 0;
    if (dr) {
        while ((dit = readdir(dr)) != NULL) {
            struct stat buf;
            stat(dit->d_name, &buf);
            AnsiString fname = dit->d_name;
            int        valid = fname.Pos(".cl");
            if ((valid == fname.Length() - 2) && (valid > 0)) {
                fname  = dir;
                fname += dit->d_name;
#ifdef _WIN32
                controls[controls_count] = LoadLibraryA(fname.c_str());
#else
                controls[controls_count] = dlopen(fname.c_str(), RTLD_LAZY);
#endif
                if (!controls[controls_count]) {
#ifndef _WIN32
                    fprintf(stderr, "Error loading library %s: %s\n", fname.c_str(), dlerror());
#else
                    fprintf(stderr, "Error loading library %s: %i\n", fname.c_str(), GetLastError());
#endif
                }
                if (controls[controls_count]) {
#ifdef _WIN32
                    INVOKERS[controls_count] = (INVOKER)GetProcAddress(controls[controls_count], "Invoke");
#else
                    INVOKERS[controls_count] = (INVOKER)dlsym(controls[controls_count], "Invoke");
#endif
                    if (!INVOKERS[controls_count])
#ifdef _WIN32
                        INVOKERS[controls_count] = (INVOKER)GetProcAddress(controls[controls_count], "_Invoke");

#else
                        INVOKERS[controls_count] = (INVOKER)dlsym(controls[controls_count], "_Invoke");
#endif

                    if (INVOKERS[controls_count]) {
                        INVOKERS[controls_count](0, INVOKE_INITLIBRARY, LocalInvoker);
                        controls_count++;
                    } else
#ifdef _WIN32
                        FreeLibrary(controls[controls_count]);

#else
                        dlclose(controls[controls_count]);
#endif
                }
            }
        }
        closedir(dr);
    }
}

INVOKER GetRequiredInvoker(int cls_id) {
    for (int i = 0; i < controls_count; i++)
        if (INVOKERS[i](0, INVOKE_QUERYTEST, cls_id) == E_ACCEPTED)
            return INVOKERS[i];
    return 0;
}

int SetCustomProperty(GTKControl *item, int prop_id, AnsiString *value) {
    INVOKER Invoke = GetRequiredInvoker(item->type);

    if (!Invoke)
        return 0;

    return IS_OK(Invoke(item->ptr ? item->ptr : item->ptr3, INVOKE_SETPROPERTY, (int)item->type, (int)prop_id, value->c_str(), (int)value->Length()));
}

int CustomMessage(GTKControl *item, int message, AnsiString target, AnsiString value) {
    INVOKER Invoke = GetRequiredInvoker(item->type);

    if (!Invoke)
        return 0;

    return IS_OK(Invoke(item->ptr ? item->ptr : item->ptr3, INVOKE_CUSTOMMESSAGE, message, item->type, target.c_str(), (int)target.Length(), value.c_str(), (int)value.Length()));
}

void GetCustomProperty(GTKControl *item, int prop_id, CConceptClient *Client, AnsiString *result) {
    INVOKER Invoke = GetRequiredInvoker(item->type);

    result->LoadBuffer(0, 0);
    if (!Invoke)
        return;
    char *data    = 0;
    int  data_len = 0;

    if (IS_OK(Invoke(item->ptr ? item->ptr : item->ptr3, INVOKE_GETPROPERTY, item->type, prop_id, &data, &data_len)))
        result->LoadBuffer(data, data_len);
}

int SetCustomEvent(GTKControl *item, int event_id, AnsiString *value) {
    INVOKER Invoke = GetRequiredInvoker(item->type);

    if (!Invoke)
        return 0;

    return IS_OK(Invoke(item->ptr ? item->ptr : item->ptr3, INVOKE_SETEVENT, item->type, event_id, value->c_str(), (int)value->Length()));
}

int LocalInvoker(void *context, int INVOKE_TYPE, ...) {
    va_list arg;

    switch (INVOKE_TYPE) {
        case INVOKE_GETCONTROLPTR:
            {
                GTKControl *ctx = lpClient->ControlsReversed[context];
                if (!ctx)
                    return E_NOTIMPLEMENTED;

                va_start(arg, INVOKE_TYPE);
                int *control_id = va_arg(arg, int *);
                int *cls_id     = va_arg(arg, int *);
                va_end(arg);

                *control_id = ctx->ID;
                *cls_id     = ctx->type;
            }
            break;

        case INVOKE_GETCONTROL:
            {
                va_start(arg, INVOKE_TYPE);
                int  control_id = va_arg(arg, int);
                void **handler  = va_arg(arg, void **);
                int  *cls_id    = va_arg(arg, int *);
                va_end(arg);

                GTKControl *ctrl = lpClient->Controls[control_id];
                if (ctrl) {
                    *cls_id  = ctrl->type;
                    *handler = ctrl->ptr;
                } else
                    return E_NOTIMPLEMENTED;
            }
            break;

        case INVOKE_GETHANDLE:
            {
                va_start(arg, INVOKE_TYPE);
                void **ptr = va_arg(arg, void **);
                va_end(arg);
                if (context)
                    *ptr = context;                  //(Gtk::Widget *)Glib::wrap((GtkWidget *)context);
                else
                    *ptr = 0;
                return E_NOERROR;
            }
            break;

        case INVOKE_GETOBJECT:
            {
                va_start(arg, INVOKE_TYPE);
                void **ptr = va_arg(arg, void **);
                va_end(arg);
                if (context)
                    *ptr = context;                  //((Gtk::Widget *)context)->gobj();
                else
                    *ptr = 0;
                return E_NOERROR;
            }
            break;

        case INVOKE_DESTROYHANDLE:
            {
                if (context)
                    delete (QWidget *)context;
                return E_NOERROR;
            }
            break;

        case INVOKE_GETPOSTSTRING:
            {
                va_start(arg, INVOKE_TYPE);
                char **str = va_arg(arg, char **);
                int  *len  = va_arg(arg, int *);
                va_end(arg);
                *str = lpClient->POST_STRING.c_str();
                *len = lpClient->POST_STRING.Length();
                return E_NOERROR;
            }
            break;

        case INVOKE_GETPOSTOBJECT:
            {
                va_start(arg, INVOKE_TYPE);
                void **handler = va_arg(arg, void **);
                int  *cls_id   = va_arg(arg, int *);
                va_end(arg);
                GTKControl *POST_OBJECT = lpClient->Controls[lpClient->POST_OBJECT];
                if (POST_OBJECT) {
                    *handler = POST_OBJECT->ptr;
                    *cls_id  = POST_OBJECT->type;
                } else {
                    *handler = 0;
                    *cls_id  = -1;
                }
                return E_NOERROR;
            }
            break;

        case INVOKE_GETEXTRAPTR:
            {
                GTKControl *ctx = lpClient->ControlsReversed[context];
                if (!ctx)
                    return E_NOTIMPLEMENTED;

                va_start(arg, INVOKE_TYPE);
                void **ptr = va_arg(arg, void **);
                va_end(arg);
                *ptr = ctx->ptr3;
                return E_NOERROR;
            }
            break;

        case INVOKE_SETEXTRAPTR:
            {
                GTKControl *ctx = lpClient->ControlsReversed[context];
                if (!ctx)
                    return E_NOTIMPLEMENTED;

                va_start(arg, INVOKE_TYPE);
                void *ptr = va_arg(arg, void *);
                va_end(arg);
                ctx->ptr3 = ptr;
                return E_NOERROR;
            }
            break;

        case INVOKE_FIREEVENT:
            {
                GTKControl *ctx = lpClient->ControlsReversed[context];
                if (!ctx)
                    return E_NOTIMPLEMENTED;
                va_start(arg, INVOKE_TYPE);
                int  event = va_arg(arg, int);
                char *data = va_arg(arg, char *);
                va_end(arg);

                lpClient->SendMessageNoWait(AnsiString(ctx->ID),
                                            MSG_EVENT_FIRED,
                                            AnsiString((long)event),
                                            data);
                return E_NOERROR;
            }
            break;

        case INVOKE_FIREEVENT2:
            {
                GTKControl *ctx = lpClient->ControlsReversed[context];
                if (!ctx)
                    return E_NOTIMPLEMENTED;
                va_start(arg, INVOKE_TYPE);
                int  event = va_arg(arg, int);
                char *data = va_arg(arg, char *);
                int  len   = va_arg(arg, int);
                va_end(arg);
                AnsiString def;
                def.LoadBuffer(data, len);

                lpClient->SendMessageNoWait(AnsiString(ctx->ID),
                                            MSG_EVENT_FIRED,
                                            AnsiString((long)event),
                                            def);
                return E_NOERROR;
            }
            break;

        case INVOKE_WAITMESSAGE:
            {
                GTKControl *ctx = lpClient->ControlsReversed[context];
                if (!ctx)
                    return E_NOTIMPLEMENTED;
            }
            break;

        case INVOKE_GETTMPCOOKIES:
            {
                va_start(arg, INVOKE_TYPE);
                char **tmpdir = va_arg(arg, char **);
                *tmpdir = temp_dir.c_str();
                return E_NOERROR;
            }
            break;

        case INVOKE_SENDMESSAGE:
            {
                GTKControl *ctx = lpClient->ControlsReversed[context];
                if (!ctx)
                    return E_NOTIMPLEMENTED;

                va_start(arg, INVOKE_TYPE);
                int  msg     = va_arg(arg, int);
                char *target = va_arg(arg, char *);
                int  t_len   = va_arg(arg, int);
                char *data   = va_arg(arg, char *);
                int  d_len   = va_arg(arg, int);
                va_end(arg);

                lpClient->SendMessageNoWait(AnsiString(ctx->ID),
                                            msg,
                                            target,
                                            data);
                return E_NOERROR;
            }
            break;

        case INVOKE_GETPRIVATEKEY:
            {
                va_start(arg, INVOKE_TYPE);
                char **key = va_arg(arg, char **);
                int  *len  = va_arg(arg, int *);
                va_end(arg);
                *key = 0;
                *len = 0;
                if (lpClient) {
                    *key = lpClient->LOCAL_PRIVATE_KEY.c_str();
                    *len = lpClient->LOCAL_PRIVATE_KEY.Length();
                }
            }
            break;

        case INVOKE_GETPUBLICKEY:
            {
                va_start(arg, INVOKE_TYPE);
                char **key = va_arg(arg, char **);
                int  *len  = va_arg(arg, int *);
                va_end(arg);
                *key = 0;
                *len = 0;
                if (lpClient) {
                    *key = lpClient->LOCAL_PUBLIC_KEY.c_str();
                    *len = lpClient->LOCAL_PUBLIC_KEY.Length();
                }
            }
            break;
    }
    return E_NOERROR;
}

void UnloadControls() {
    for (int i = 0; i < controls_count; i++)
        INVOKERS[i](0, INVOKE_DONELIBRARY);
    controls_count = 0;
}

CConceptClient *GetClient() {
    return lpClient;
}

void SetCookiesDir(AnsiString dir) {
    temp_dir = dir;
}

int CheckCredentials() {
    static int secure_app = -1;

    if (secure_app >= 0)
        return secure_app;

    QMessageBox msg(QMessageBox::Question, QString::fromUtf8("Do you trust this application ?"), QString::fromUtf8("<b>Do you trust this application ?</b><br/><br/>This application wants to perform some operations that could pose a security risk like:<br/>\n- access your environment variables<br/>\n- run a command on your machine<br/>\n<br/>\nIf don't trust this application, it's safer to decline."), QMessageBox::Yes | QMessageBox::No);
    msg.setWindowIcon(QIcon(QString::fromUtf8(":/resources/conceptclienticon.png")));
    dialogs++;
    int res = msg.exec();
    dialogs--;
    if (res == QMessageBox::Yes)
        secure_app = 1;
    else
        secure_app = 0;

    return secure_app;
}

AnsiString GetCachedFile(AnsiString *hash) {
#ifdef _WIN32
    AnsiString h_path = getenv("LOCALAPPDATA");
    if (!h_path.Length())
        h_path = getenv("APPDATA");

    h_path += "/ConceptClient-private/";
#else
    AnsiString h_path = getenv("HOME");
    h_path += "/.ConceptClient-private/";
#endif
    h_path += (char *)"account-";
    h_path += *hash;
    h_path += ".dat";

    return h_path;
}

int GetCachedLogin(AnsiString *username, AnsiString *password, AnsiString *hash) {
    AnsiString file = GetCachedFile(hash);
    AnsiString data;

    data.LoadFile(file.c_str());
    if (!data.Length())
        return 0;

    AnsiString sep = "\n";

    AnsiString target = data;
    int        pos    = target.Pos(sep);
    int        index  = 0;
    int        start  = 0;

    int ret = 1;
    if (pos > 1) {
        target.c_str()[pos - 1] = 0;
        *username = target.c_str();
        if (pos < target.Length()) {
            *password = target.c_str() + pos;
            ret       = 2;
        }
    } else
        *username = target;
    return ret;
}

int SetCachedLogin(AnsiString username, AnsiString password, int flag, AnsiString *hash) {
    AnsiString file = GetCachedFile(hash);

    if ((flag == 0) || (!username.Length())) {
        unlink(file.c_str());
        return 0;
    }
    AnsiString data;
    data  = username;
    data += "\n";
    if (flag == 2)
        data += password;
    return data.SaveFile(file.c_str());
}

AnsiString do_md5(AnsiString str) {
    MD5_CTX       CTX;
    unsigned char md5_sum[16];
    AnsiString    res;

    MD5Init(&CTX);
    MD5Update(&CTX, (unsigned char *)str.c_str(), (long)str.Length());
    MD5Final(md5_sum, &CTX);

    unsigned char result[32];
    for (int i = 0; i < 16; i++) {
        unsigned char first = md5_sum[i] / 0x10;
        unsigned char sec   = md5_sum[i] % 0x10;

        if (first < 10)
            result[i * 2] = '0' + first;
        else
            result[i * 2] = 'a' + (first - 10);

        if (sec < 10)
            result[i * 2 + 1] = '0' + sec;
        else
            result[i * 2 + 1] = 'a' + (sec - 10);
    }
    res.LoadBuffer((char *)result, 32);
    return res;
}

AnsiString do_sha1(AnsiString str) {
    AnsiString  res;
    SHA1Context CTX;

    SHA1Reset(&CTX);
    SHA1Input(&CTX, (unsigned char *)str.c_str(), (long)str.Length());

    if (!SHA1Result(&CTX))
        return res;

    unsigned char result[40];
    unsigned char sha1_sum[20];

    unsigned char *sha1_ptr = (unsigned char *)CTX.Message_Digest;
    for (int j = 0; j < 5; j++) {
#ifdef LITTLE_ENDIAN
        for (int k = 3; k >= 0; k--)
            sha1_sum[j * 4 + 3 - k] = sha1_ptr[j * 4 + k];

#else
        for (int k = 0; k < 4; k++)
            sha1_sum[j * 4 + k] = sha1_ptr[j * 4 + k];
#endif
    }

    for (int i = 0; i < 20; i++) {
        unsigned char first = sha1_sum[i] / 0x10;
        unsigned char sec   = sha1_sum[i] % 0x10;

        if (first < 10)
            result[i * 2] = '0' + first;
        else
            result[i * 2] = 'a' + (first - 10);

        if (sec < 10)
            result[i * 2 + 1] = '0' + sec;
        else
            result[i * 2 + 1] = 'a' + (sec - 10);
    }

    res.LoadBuffer((char *)result, 40);
    return res;
}

AnsiString do_sha256(AnsiString str) {
    AnsiString  res;
    sha256_context CTX;

    sha256_starts(&CTX);
    sha256_update(&CTX, (unsigned char *)str.c_str(), (long)str.Length());

    unsigned char sha256_sum[32];

    sha256_finish(&CTX, sha256_sum);

    unsigned char result[64];

    for (int i = 0; i < 32; i++) {
        unsigned char first = sha256_sum[i] / 0x10;
        unsigned char sec   = sha256_sum[i] % 0x10;

        if (first < 10)
            result[i * 2] = '0' + first;
        else
            result[i * 2] = 'a' + (first - 10);

        if (sec < 10)
            result[i * 2 + 1] = '0' + sec;
        else
            result[i * 2 + 1] = 'a' + (sec - 10);
    }

    res.LoadBuffer((char *)result, 64);
    return res;
}

QFont fontDesc(QFont qf, AnsiString& desc) {
    if (desc == (char *)"bold") {
        qf.setBold(true);
    } else
    if (desc == (char *)"italic") {
        qf.setItalic(true);
    } else
    if (desc == (char *)"underline") {
        qf.setUnderline(true);
    } else
    if (desc.Pos("bold ") == 1) {
        qf.setBold(true);
        AnsiString ssize(desc.c_str() + 5);
        int        size = ssize.ToInt();
        if (size > 0)
            qf.setPixelSize(size);
    } else
    if (desc.Pos("italic ") == 1) {
        qf.setBold(true);
        AnsiString ssize(desc.c_str() + 7);
        int        size = ssize.ToInt();
        if (size > 0)
            qf.setPixelSize(size);
    } else
    if (desc.Pos("underline ") == 1) {
        qf.setBold(true);
        AnsiString ssize(desc.c_str() + 10);
        int        size = ssize.ToInt();
        if (size > 0)
            qf.setPixelSize(size);
    } else {
        QString fdesc(desc.c_str());
        qf.fromString(fdesc);
    }
    return qf;
}

QString ConvertStyle(QString ref) {
    QString res = ref.replace("VisibleRemoteObject", "QWidget");

    res = res.replace("RForm", "QWidget");
    res = res.replace("RLabel", "QLabel");
    res = res.replace("RImage", "QImage");
    res = res.replace("RButton", "QPushButton");
    res = res.replace("RCheckButton", "QCheckBox");
    res = res.replace("RRadioButton", "QRadioButton");
    res = res.replace("RTreeView", "QTreeWidget");
    res = res.replace("RIconView", "QListWidget");
    res = res.replace("RComboBox", "QComboBox");
    res = res.replace("REditComboBox", "QComboBox");
    res = res.replace("REdit", "QLineEdit");
    res = res.replace("RTextView", "QTextEdit");
    res = res.replace("RCalendar", "QCalendarWidget");
    res = res.replace("RMenuBar", "QMenuBar");
    res = res.replace("RStatusbar", "QStatusBar");
    res = res.replace("RToolbar", "QToolBar");
    res = res.replace("RTable", "QWidget");
    res = res.replace("RVBox", "QWidget");
    res = res.replace("RHBox", "QWidget");
    res = res.replace("RVPaned", "QWidget");
    res = res.replace("RHPaned", "QWidget");
    res = res.replace("RSpinButton", "QSpinBox");
    res = res.replace("RHScale", "QSlider");
    res = res.replace("RVScale", "QSlider");
    res = res.replace("RHScrollbar", "QScrollBar");
    res = res.replace("RVScrollbar", "QScrollBar");
    res = res.replace("RProgressBar", "QProgressBar");
    res = res.replace("RToolButton", "QToolButton");
    res = res.replace("RRadioToolButton", "QToolButton");
    res = res.replace("RMenuToolButton", "QToolButton");
    res = res.replace("RToggleToolButton", "QToolButton");
    res = res.replace("RScrolledWindow", "QScrollArea");
    res = res.replace("RFixed", "QWidget");
    res = res.replace("RHButtonBox", "QWidget");
    res = res.replace("RVButtonBox", "QWidget");
    res = res.replace("RHandleBox", "QDockWidget");
    res = res.replace("RNotebook", "QTabWidget");
    res = res.replace("RViewPort", "QWidget");
    return res;
}

void Done(CConceptClient *CC, int res, bool forced) {
    UnloadControls();
    // fast exit
    CC->Disconnect();
    int size = CC->CookieList.Count();
    for (int i = 0; i < size; i++) {
        AnsiString *filename = (AnsiString *)CC->CookieList[i];
        if (filename)
            _unlink(filename->c_str());
    }
    if ((CC->MainForm) || (forced)) {
        if (dialogs <= 0)
            exit(res);
    }
}

#ifndef _WIN32
int nw_system(CConceptClient *Client, const char *cmd, int no_wait = 0) {
    int              stat;
    pid_t            pid;
    struct sigaction sa, savintr, savequit;
    sigset_t         saveblock;
    char             *shell;

    shell = getenv("SHELL");

    if (cmd == NULL)
        return 1;
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigemptyset(&savintr.sa_mask);
    sigemptyset(&savequit.sa_mask);
    sigaction(SIGINT, &sa, &savintr);
    sigaction(SIGQUIT, &sa, &savequit);
    sigaddset(&sa.sa_mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &sa.sa_mask, &saveblock);
    if ((pid = fork()) == 0) {
        sigaction(SIGINT, &savintr, (struct sigaction *)0);
        sigaction(SIGQUIT, &savequit, (struct sigaction *)0);
        sigprocmask(SIG_SETMASK, &saveblock, (sigset_t *)0);
        shell = ((shell) && (shell[0])) ? shell : (char *)"/bin/sh";
        execl(shell, "sh", "-c", cmd, (char *)0);
        _exit(127);
    }
    if (pid == -1)
        stat = -1;
    else 
    if (!no_wait) {
        if (Client) {
            Client->executed_process = pid;
            while (waitpid(pid, &stat, 0) == -1)
                if (errno != EINTR) {
                    stat = -1;
                    break;
                }
            Client->executed_process = 0;
        }
    }
    sigaction(SIGINT, &savintr, (struct sigaction *)0);
    sigaction(SIGQUIT, &savequit, (struct sigaction *)0);
    sigprocmask(SIG_SETMASK, &saveblock, (sigset_t *)0);
    return stat;
}
#endif

int open_link(AnsiString param) {
    if (!param.Length())
        return -1;

    int has_spaces = (param.Pos(" ") > 1);
#ifdef _WIN32
    AnsiString app_path;// = OWNER->path;
    app_path += CConceptClient::ConceptClientPath;

    AnsiString command;
    if (has_spaces)
        command += (char *)"\"";
    command += param.c_str();
    if (has_spaces)
        command += (char *)"\"";

    ShellExecuteA(0,
                  "open",
                  app_path.c_str(),
                  command.c_str(),
                  0,
                  SW_SHOW);
#else
    AnsiString command;// = CConceptClient::path;
    command += CConceptClient::ConceptClientPath;
    if (has_spaces)
        command += (char *)" \"";
    else
        command += (char *)" ";
    command += param.c_str();
    if (has_spaces)
        command += (char *)"\"";
    nw_system(lpClient, command.c_str(), 1);
#endif
    return 0;
}

AnsiString SafeFile(AnsiString& filename) {
    AnsiString safe_file;
    int        len = filename.Length();
    char       *s  = filename.c_str();

    for (int i = 0; i < len; i++) {
        char c = s[i];
        if ((c != '/') && (c != '\\'))
            safe_file += c;
    }
    return safe_file;
}

void StockName(char *iconname, AnsiString& result) {
    if (!iconname)
        return;

    char *ref = strstr(iconname, "phone-");
    if (ref)
        return;

    ref = strstr(iconname, "gtk-clear");
    if (ref)
        return;

    ref = strstr(iconname, "gtk-full");
    if (ref)
        return;

    ref = strstr(iconname, "gtk-goto-");
    if (ref)
        iconname = ref + 9;

    ref = strstr(iconname, "gtk-go-");
    if (ref)
        iconname = ref + 7;

    ref = strstr(iconname, "gtk-media-");
    if (ref)
        iconname = ref + 10;

    ref = strstr(iconname, "gtk-");
    if (ref)
        iconname = ref + 4;

    int len = strlen(iconname);
    result += (char)toupper(iconname[0]);

    for (int i = 1; i < len; i++) {
        char c = iconname[i];
        if (c == '-')
            c = ' ';
        result += c;
    }
    return;
}

AnsiString GET_STOCK(int id) {
    switch (id) {
        case __ABOUT:
            return "gtk-about";

        case __ADD:
            return "gtk-add";

        case __APPLY:
            return "gtk-apply";

        case __BOLD:
            return "gtk-bold";

        case __CANCEL:
            return "gtk-cancel";

        case __CDROM:
            return "gtk-cdrom";

        case __CLEAR:
            return "gtk-clear";

        case __CLOSE:
            return "gtk-close";

        case __COLOR_PICKER:
            return "gtk-color-picker";

        case __CONNECT:
            return "gtk-connect";

        case __CONVERT:
            return "gtk-convert";

        case __COPY:
            return "gtk-copy";

        case __CUT:
            return "gtk-cut";

        case __DELETE:
            return "gtk-delete";

        case __DIALOG_AUTHENTICATION:
            return "gtk-dialog-authentication";

        case __DIALOG_ERROR:
            return "gtk-dialog-error";

        case __DIALOG_INFO:
            return "gtk-dialog-info";

        case __DIALOG_QUESTION:
            return "gtk-dialog-question";

        case __DIALOG_WARNING:
            return "gtk-dialog-warning";

        case __DIRECTORY:
            return "gtk-directory";

        case __DISCONNECT:
            return "gtk-disconnect";

        case __DND:
            return "gtk-dnd";

        case __DND_MULTIPLE:
            return "gtk-dnd-multiple";

        case __EDIT:
            return "gtk-edit";

        case __EXECUTE:
            return "gtk-execute";

        case __FILE:
            return "gtk-file";

        case __FIND:
            return "gtk-find";

        case __FIND_AND_REPLACE:
            return "gtk-find-and-replace";

        case __FLOPPY:
            return "gtk-floppy";

        case __FULLSCREEN:
            return "gtk-fullscreen";

        case __GO_BACK:
            return "gtk-go-back";

        case __GO_DOWN:
            return "gtk-go-down";

        case __GO_FORWARD:
            return "gtk-go-forward";

        case __GO_UP:
            return "gto-go-up";

        case __GOTO_BOTTOM:
            return "gtk-goto-bottom";

        case __GOTO_FIRST:
            return "gtk-goto-first";

        case __GOTO_LAST:
            return "gtk-goto-last";

        case __GOTO_TOP:
            return "gtk-goto-top";

        case __HARDDISK:
            return "gtk-harddisk";

        case __HELP:
            return "gtk-help";

        case __HOME:
            return "gtk-home";

        case __INDENT:
            return "gtk-indent";

        case __INDEX:
            return "gtk-index";

        case __INFO:
            return "gtk-info";

        case __ITALIC:
            return "gtk-italic";

        case __JUMP_TO:
            return "gtk-jump-to";

        case __JUSTIFY_CENTER:
            return "gtk-justify-center";

        case __JUSTIFY_FILL:
            return "gtk-justify-fill";

        case __JUSTIFY_LEFT:
            return "gtk-justify-left";

        case __JUSTIFY_RIGHT:
            return "gtk-justify-right";

        case __LEAVE_FULLSCREEN:
            return "gtk-leave-fullscreen";

        case __MEDIA_FORWARD:
            return "gtk-media-forward";

        case __MEDIA_NEXT:
            return "gtk-media-next";

        case __MEDIA_PAUSE:
            return "gtk-media-pause";

        case __MEDIA_PLAY:
            return "gtk-media-play";

        case __MEDIA_PREVIOUS:
            return "gtk-media-previous";

        case __MEDIA_RECORD:
            return "gtk-media-record";

        case __MEDIA_REWIND:
            return "gtk-media-rewind";

        case __MEDIA_STOP:
            return "gtk-media-stop";

        case __MISSING_IMAGE:
            return "gtk-missing-image";

        case __NETWORK:
            return "gtk-network";

        case __NEW:
            return "gtk-new";

        case __NO:
            return "gtk-no";

        case __OK:
            return "gtk-ok";

        case __OPEN:
            return "gtk-open";

        case __PASTE:
            return "gtk-paste";

        case __PREFERENCES:
            return "gtk-preferences";

        case __PRINT:
            return "gtk-print";

        case __PRINT_PREVIEW:
            return "gtk-print-preview";

        case __PROPERTIES:
            return "gtk-properties";

        case __QUIT:
            return "gtk-quit";

        case __REDO:
            return "gtk-redo";

        case __REFRESH:
            return "gtk-refresh";

        case __REMOVE:
            return "gtk-remove";

        case __REVERT_TO_SAVED:
            return "gtk-revert-to-saved";

        case __SAVE:
            return "gtk-save";

        case __SAVE_AS:
            return "gtk-save-as";

        case __SELECT_COLOR:
            return "gtk-select-color";

        case __SELECT_FONT:
            return "gtk-select-font";

        case __SORT_ASCENDING:
            return "gtk-sort-ascending";

        case __SORT_DESCENDING:
            return "gtk-sort-descending";

        case __SPELL_CHECK:
            return "gtk-spell-check";

        case __STOP:
            return "gtk-stop";

        case __STRIKETHROUGH:
            return "gtk-strikethrough";

        case __UNDELETE:
            return "gtk-undelete";

        case __UNDERLINE:
            return "gtk-underline";

        case __UNDO:
            return "gtk-undo";

        case __UNINDENT:
            return "gtk-unident";

        case __YES:
            return "gtk-yes";

        case __ZOOM_100:
            return "gtk-zoom-100";

        case __ZOOM_FIT:
            return "gtk-zoom-fit";

        case __ZOOM_IN:
            return "gtk-zoom-in";

        case __ZOOM_OUT:
            return "gtk-zoom-out";
    }
    return (char *)"";
}

QPixmap StockIcon(char *iconname) {
    AnsiString temp(iconname);

    int stock_id = temp.ToInt();

    if (stock_id) {
        AnsiString temp2 = GET_STOCK(stock_id);
        if (temp2.Length())
            temp = temp2;
    }

    QStyle *l_style = QApplication::style();

    if (temp == (char *)"gtk-ok")
        return l_style->standardPixmap(QStyle::SP_DialogOkButton);
    if (temp == (char *)"gtk-cancel")
        return l_style->standardPixmap(QStyle::SP_DialogCancelButton);
    if (temp == (char *)"gtk-close")
        return l_style->standardPixmap(QStyle::SP_DialogCloseButton);
    if (temp == (char *)"gtk-apply")
        return l_style->standardPixmap(QStyle::SP_DialogApplyButton);
    if (temp == (char *)"gtk-clear")
        return l_style->standardPixmap(QStyle::SP_DialogResetButton);
    if (temp == (char *)"gtk-refresh")
        return l_style->standardPixmap(QStyle::SP_BrowserReload);
    if (temp == (char *)"gtk-stop")
        return l_style->standardPixmap(QStyle::SP_BrowserStop);
    if (temp == (char *)"gtk-yes")
        return l_style->standardPixmap(QStyle::SP_DialogYesButton);
    if (temp == (char *)"gtk-no")
        return l_style->standardPixmap(QStyle::SP_DialogNoButton);
    if (temp == (char *)"gtk-discard")
        return l_style->standardPixmap(QStyle::SP_DialogDiscardButton);
    if (temp == (char *)"gtk-save")
        return l_style->standardPixmap(QStyle::SP_DialogSaveButton);
    if (temp == (char *)"gtk-help")
        return l_style->standardPixmap(QStyle::SP_DialogHelpButton);
    if (temp == (char *)"gtk-go-back")
        return l_style->standardPixmap(QStyle::SP_ArrowBack);
    if (temp == (char *)"gtk-go-forward")
        return l_style->standardPixmap(QStyle::SP_ArrowForward);
    if (temp == (char *)"gtk-go-up")
        return l_style->standardPixmap(QStyle::SP_ArrowUp);
    if (temp == (char *)"gtk-go-down")
        return l_style->standardPixmap(QStyle::SP_ArrowDown);
    if (temp == (char *)"gtk-delete")
        return l_style->standardPixmap(QStyle::SP_TrashIcon);
    if (temp == (char *)"gtk-media-play")
        return l_style->standardPixmap(QStyle::SP_MediaPlay);
    if (temp == (char *)"gtk-media-stop")
        return l_style->standardPixmap(QStyle::SP_MediaStop);
    if (temp == (char *)"gtk-media-pause")
        return l_style->standardPixmap(QStyle::SP_MediaPause);
    if (temp == (char *)"gtk-media-next")
        return l_style->standardPixmap(QStyle::SP_MediaSkipForward);
    if (temp == (char *)"gtk-media-previous")
        return l_style->standardPixmap(QStyle::SP_MediaSkipBackward);
    if (temp == (char *)"gtk-media-forward")
        return l_style->standardPixmap(QStyle::SP_MediaSeekForward);
    if (temp == (char *)"gtk-media-backward")
        return l_style->standardPixmap(QStyle::SP_MediaSeekBackward);
    if (temp == (char *)"gtk-dialog-error")
        return l_style->standardPixmap(QStyle::SP_MessageBoxCritical);
    if (temp == (char *)"gtk-dialog-warning")
        return l_style->standardPixmap(QStyle::SP_MessageBoxWarning);
    if (temp == (char *)"gtk-dialog-info")
        return l_style->standardPixmap(QStyle::SP_MessageBoxInformation);
    if (temp == (char *)"gtk-dialog-question")
        return l_style->standardPixmap(QStyle::SP_MessageBoxQuestion);

    AnsiString source = ":/resources/";
    source += temp;
    source += ".png";
    QPixmap pixmap;
    if (!pixmap.load(QString::fromUtf8(source.c_str()))) {
        if (temp.Pos("gtk-") != 1) {
            AnsiString tmp2("gtk-");
            tmp2 += temp;
            return StockIcon(tmp2.c_str());
        }
        fprintf(stderr, "Unknown stock item: %s\n", iconname);
    }
    return pixmap;
}

void UpdateTabs(QTabWidget *tabwidget, GTKControl *item, CConceptClient *Client) {
    if (!tabwidget->tabsClosable()) {
        ClientEventHandler *events = (ClientEventHandler *)item->events;
        if (!events) {
            events       = new ClientEventHandler(item->ID, Client);
            item->events = events;
        }
        QObject::connect(tabwidget, SIGNAL(tabCloseRequested(int)), events, SLOT(on_tabCloseRequested(int)));
        tabwidget->setTabsClosable(true);
        int index = 0;
        while (tabwidget->widget(index)) {
            QWidget *closebutton = tabwidget->tabBar()->tabButton(index, QTabBar::RightSide);
            if (closebutton)
                closebutton->hide();
            index++;
        }
    }
}

void MarkRecursive(CConceptClient *Client, GTKControl *tab, int index, QWidget *element, NotebookTab *nt, int adjusted) {
    if ((!tab) || (tab->type != CLASS_NOTEBOOK))
        return;
    if (!element)
        return;
    if (!nt)
        return;

    QTabWidget *tabwidget = (QTabWidget *)tab->ptr;
    GTKControl *item      = Client->ControlsReversed[element];
    if (item) {
        item->ref   = tab;
        item->index = index;
        switch (item->type) {
            case CLASS_IMAGE:
                {
                    const QPixmap *pmap = ((QLabel *)item->ptr)->pixmap();
                    if (nt->visible) {
                        if (pmap)
                            tabwidget->setTabIcon(adjusted, QIcon(*pmap));
                    }
                    nt->image = item;
                }
                break;

            case CLASS_LABEL:
                if (nt->visible)
                    tabwidget->setTabText(adjusted, ((QLabel *)item->ptr)->text());
                nt->title = (char *)((QLabel *)item->ptr)->text().toUtf8().data();
                break;

            case CLASS_EVENTBOX:
                {
                    UpdateTabs(tabwidget, tab, Client);
                    if (nt->visible > 0) {
                        QWidget *closebutton = tabwidget->tabBar()->tabButton(adjusted, QTabBar::RightSide);
                        if (closebutton)
                            closebutton->show();
                    }
                    nt->button_id    = item->ID;
                    nt->close_button = true;
                }
                break;

            case CLASS_BUTTON:
                //UpdateTabs(tabwidget, tab, Client);
                //tabwidget->setTabText(adjusted, ((QPushButton *)item->ptr)->text());
                if (nt->visible) {
                    tabwidget->tabBar()->setTabButton(adjusted, QTabBar::RightSide, (QWidget *)item->ptr);

                    if (item->visible > 0)
                        ((QWidget *)item->ptr)->show();
                }
                nt->button = item;
                break;

            // no break here !
            default:
                {
                    QObjectList children            = element->children();
                    QObjectList::const_iterator it  = children.begin();
                    QObjectList::const_iterator eIt = children.end();
                    while (it != eIt) {
                        QWidget *child = (QWidget *)*it;
                        MarkRecursive(Client, tab, index, (QWidget *)child, nt, adjusted);
                        ++it;
                    }
                }
        }
    }
}

AnsiString ConvertCaption(AnsiString& caption) {
    AnsiString result;
    int        len  = caption.Length();
    char       *buf = caption.c_str();

    for (int i = 0; i < len; i++) {
        char c = caption[i];
        // if c is &, add it twice
        if (c == '&')
            result += c;

        if (c == '_')
            c = '&';
        result += c;
    }
    return result;
}

int GetDNDFile(Parameters *PARAM) {
    GTKControl *item = PARAM->Owner->Controls[PARAM->Sender.ToInt()];

    if (!item) {
        PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_GET_DNDFILE, "-5", "Error in custom message: Invalid RID in owner");
        return 0;
    }
    if (!item->events) {
        PARAM->Owner->SendMessageNoWait(PARAM->Sender, MSG_GET_DNDFILE, "-4", "No drop operations on this object");
        return 0;
    }
    if (PARAM->Value.ToInt() == 1) {
        ((ClientEventHandler *)item->events)->ResetDrop();
        return 0;
    }
    if (!((ClientEventHandler *)item->events)->FileIsDropped(PARAM->Target)) {
        PARAM->Owner->SendMessageNoWait(PARAM->Sender, MSG_GET_DNDFILE, "-3", "The requested file was not dropped here");
        return 0;
    } else {
        AnsiString  data;
        std::string filename;
#ifdef _WIN32
        if (PARAM->Target.Pos("file:///") == 1)
            filename = UriDecode(PARAM->Target.c_str() + 8);

#else
        if (PARAM->Target.Pos("file://") == 1)
            filename = UriDecode(PARAM->Target.c_str() + 7);
#endif

        else
            filename = UriDecode(PARAM->Target.c_str());

        if (data.LoadFile((char *)filename.c_str()) == 0)
            PARAM->Owner->SendBigMessage(PARAM->Sender, MSG_GET_DNDFILE, "0", data);
        else
            PARAM->Owner->SendMessageNoWait(PARAM->Sender, MSG_GET_DNDFILE, "-1", "Error reading file (insufficient rights ?)");
        return 0;
    }
    return 1;
}

QColor doColor(unsigned int color) {
    int alpha = -1;

    if (color >= 0x1000000) {
        alpha  = color / 0x1000000;
        color %= 0x1000000;
    }
    int red = color / 0x10000;
    color %= 0x10000;
    int green = color / 0x100;
    color %= 0x100;
    int blue = color;

    QColor result(red, green, blue);
    if (alpha >= 0)
        result.setAlpha(alpha);

    return result;
}

QString doColorString(unsigned int color) {
    QString result;
    int     alpha = -1;

    if (color >= 0x1000000) {
        alpha  = color / 0x1000000;
        color %= 0x1000000;
    }
    int red = color / 0x10000;
    color %= 0x10000;
    int green = color / 0x100;
    color %= 0x100;
    int blue = color;
    //alpha=255;
    if (alpha >= 0)
        result = QString("rgba(");
    else
        result = QString("rgb(");
    result += QString::number(red);
    result += QString(",");
    result += QString::number(green);
    result += QString(",");
    result += QString::number(blue);
    if (alpha >= 0) {
        result += QString(",");
        result += QString::number((double)alpha / 0xFF);
    }
    result += QString(")");
    return result;
}

void SetAdjustment(CConceptClient *Client, GTKControl *item) {
    if (item->type != CLASS_ADJUSTMENT)
        return;

    GTKControl *parent = (GTKControl *)item->parent;
    switch (parent->type) {
        case CLASS_HSCROLLBAR:
        case CLASS_VSCROLLBAR:
            item->ptr = parent->ptr;
            break;

        case CLASS_TREEVIEW:
        case CLASS_SCROLLEDWINDOW:
            {
                QScrollBar *bar     = NULL;
                GTKControl *parent2 = parent;
                if (parent->type == CLASS_SCROLLEDWINDOW) {
                    QWidget *child = ((QScrollArea *)parent->ptr)->widget();
                    parent2 = Client->ControlsReversed[child];
                    if (parent2) {
                        if ((parent2->type != CLASS_TREEVIEW) && (parent2->type != CLASS_ICONVIEW) && (parent2->type != CLASS_TEXTVIEW))
                            parent2 = parent;
                    } else
                        parent2 = parent;
                }
                if (parent->ID_Adjustment == item->ID)
                    bar = ((QAbstractScrollArea *)parent2->ptr)->verticalScrollBar();
                else
                if (parent->ID_HAdjustment == item->ID)
                    bar = ((QAbstractScrollArea *)parent2->ptr)->horizontalScrollBar();

                if (bar)
                    item->ptr = bar;
            }
            break;
    }
}

QTreeWidgetItem *ItemByPath(QTreeWidget *widget, AnsiString& apath) {
    if (!apath.Length())
        return NULL;

    QString     path(apath.c_str());
    QStringList list = path.split(QString(":"), QString::SkipEmptyParts);
    int         len  = list.size();
    //if (!len)
    //    return NULL;

    QTreeWidgetItem *parent = widget->invisibleRootItem();
    if ((len) && (parent)) {
        for (int i = 0; i < len; i++) {
            int sindex = list[i].toInt();
            if (!parent)
                return NULL;

            int ccount = parent->childCount();
            if ((sindex >= ccount) || (sindex < 0)) {
                return NULL;
            }
            parent = parent->child(sindex);
        }
    }
    return parent;
}

AnsiString ItemToPath(QTreeWidget *widget, QTreeWidgetItem *item) {
    AnsiString res;
    AnsiString tmp((char *)":");
    int        index = -1;

    while (item) {
        QTreeWidgetItem *parent = item->parent();
        if (parent)
            index = parent->indexOfChild(item);
        else
            index = widget->indexOfTopLevelItem(item);

        if (res.Length())
            res = tmp + res;
        res  = AnsiString((long)index) + res;
        item = parent;
    }
    return res;
}

QModelIndex computeModelIndex(QTreeWidgetItem *item, int column = 0) {
    QModelIndex index;

    if (item == 0)
        return index;
    QTreeWidget *treeWidget = item->treeWidget();
    if (treeWidget == 0)
        return index;
    QTreeWidgetItem *parent = item->parent();
    if (parent)
        return computeModelIndex(parent, 0).child(parent->indexOfChild(item), column);
    return treeWidget->model()->index(treeWidget->indexOfTopLevelItem(item), 0);
}

void UpdateParents(GTKControl *parent, QWidget *widget, int packing) {
    while (parent) {
        /*if (parent->type==CLASS_SCROLLEDWINDOW) {
            if (packing==MyPACK_SHRINK)
                widget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
            else
                widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
           }*/
        if (parent->type == CLASS_BUTTON)
            widget->resize(widget->sizeHint());
        QWidget *widget = (QWidget *)parent->ptr;
        parent = (GTKControl *)parent->parent;
    }
}

int is_ipv6(char *str, int len, AnsiString *replaced_host) {
    int res = 0;

    for (int i = 0; i < len; i++) {
        if (str[i] == ':') {
            res++;
            if (res > 1)
                return 1;
        } else
        if ((replaced_host) && (!res))
            *replaced_host += str[i];
    }
    return 0;
}

void GetShell(AnsiString *from, char **file, char **param) {
    *file  = from->c_str();
    *param = *file;
    int  len       = from->Length();
    char in_quotes = false;
    for (int i = 0; i < len; i++) {
        if ((*param)[0] == ' ') {
            if (!in_quotes) {
                (*param)[0] = 0;
                (*param)++;
                return;
            }
        } else
        if ((*param)[0] == '"')
            in_quotes = !in_quotes;
        (*param)++;
    }
    *param = 0;
}

THREAD_TYPE2 ThreadSHELL(LPVOID PARAM) {
    CConceptClient *OWNER = (CConceptClient *)PARAM;
    AnsiString     replaced_host;
    int            isipv6 = is_ipv6(OWNER->lastHOST.c_str(), OWNER->lastHOST.Length(), &replaced_host);

    AnsiString app_path;// = OWNER->path;
    app_path += OWNER->ConceptClientPath;

    AnsiString params;
    if (isipv6)
        params += (char *)"[";
    if (isipv6)
        params += OWNER->lastHOST;
    else
        params += replaced_host;
    if (isipv6)
        params += (char *)"]";
    params += (char *)":";
    params += AnsiString(OWNER->PORT);
    params += (char *)" \"";
    params += run_target;
    params += (char *)"\"";
    if (run_value.ToInt() != 0) {
        if (OWNER->is_http)
            params += " http ";
        else
        if (_debug)
            params += " debug ";
        else
            params += " pipe ";
        params += run_value;
    }

    wait_thread = 0;
#ifdef _WIN32
    ShellExecuteA(0,
                  "open",
                  app_path.c_str(),
                  params.c_str(),
                  0,
                  SW_SHOW);
#else
    AnsiString path_param = app_path;
    path_param += " ";
    path_param += params;
    system(path_param.c_str());
#endif
}

int NewApplication(CConceptClient *owner, char *target, char *value, bool debug = false) {
    run_target  = target;
    run_value   = value;
    wait_thread = 1;
    _debug      = debug;
#ifdef _WIN32
    DWORD TID;
    CreateThread(0, 0, ThreadSHELL, owner, 0, &TID);
#else
    //pthread *threadID;
    pthread_t threadID;
    pthread_create(&threadID, NULL, ThreadSHELL, owner);
#endif
    Sleep(500);
    return 0;
}

THREAD_TYPE2 ExecuteShellThread(LPVOID PARAM) {
    CConceptClient *Client = (CConceptClient *)PARAM;

    while (true) {
        AnsiString *to_execute = 0;
        semp(Client->shell_lock);
        if (Client->ShellList.Count())
            to_execute = (AnsiString *)Client->ShellList.Item(0);
        semv(Client->shell_lock);
        if (to_execute) {
            semp(Client->shell_lock);
            Client->ShellList.Remove(0);
            semv(Client->shell_lock);
#ifdef _WIN32
            SHELLEXECUTEINFOA ShExecInfo;
            ZeroMemory(&ShExecInfo, sizeof(SHELLEXECUTEINFO));
            ShExecInfo.cbSize = sizeof(SHELLEXECUTEINFO);
            ShExecInfo.lpVerb = "open";
            ShExecInfo.fMask  = 0x00000040;

            char *file  = NULL;
            char *param = NULL;
            GetShell(to_execute, &file, &param);


            ShExecInfo.lpFile       = file;
            ShExecInfo.lpParameters = param;
            ShellExecuteExA(&ShExecInfo);
            if (ShExecInfo.hProcess) {
                semp(Client->execute_lock);
                Client->executed_process = ShExecInfo.hProcess;
                semv(Client->execute_lock);
                WaitForSingleObject(ShExecInfo.hProcess, INFINITE);
                semp(Client->execute_lock);
                Client->executed_process = 0;
                semv(Client->execute_lock);
                //if (run_app < 0)
                //    break;
            }
#else
            //system(path_param.c_str());
            nw_system(Client, to_execute->c_str());
#endif
            delete to_execute;
        } else
            Sleep(500);
    }
    return 0;
}

#ifndef _WIN32
THREAD_TYPE2 CommandThread(LPVOID PARAM) {
    AnsiString sparam = *(AnsiString *)PARAM;

    if (lpClient)
        semv(lpClient->execute_lock);
    system(sparam.c_str());
    return 0;
}
#endif

int Execute(CConceptClient *Client, AnsiString command, int add_to_queue) {
    int res = 0;

    if (add_to_queue) {
        semp(Client->shell_lock);
        Client->ShellList.Add(new AnsiString(command), DATA_STRING);
        res = Client->ShellList.Count();
        semv(Client->shell_lock);
    } else {
#ifdef _WIN32
        char *file  = NULL;
        char *param = NULL;
        GetShell(&command, &file, &param);

        ShellExecuteA(NULL, "open", file, param, NULL, SW_SHOWNORMAL);
#else
 #ifdef __APPLE__
        command = AnsiString("open ") + command;
 #else
        char *prefix_env = (char *)getenv("CONCEPTOPEN");
        if ((prefix_env) && (prefix_env[0]))
            command = AnsiString(prefix_env) + AnsiString(" ") + command;
 #endif
        pthread_t threadID;
        semp(Client->execute_lock);
        pthread_create(&threadID, NULL, CommandThread, &command);
        semp(Client->execute_lock);
        // wait to unlock
        semv(Client->execute_lock);
#endif
    }
    return res;
}

void SafeDelete(CConceptClient *Client, GTKControl *item, bool delete_item = true) {
    if (!item)
        return;
    for (std::map<int, GTKControl *>::iterator it = Client->Controls.begin(); it != Client->Controls.end(); ++it) {
        GTKControl *ref = it->second;
        if (ref) {
            if (ref->ref == item)
                ref->ref = NULL;
            if (ref->parent == item)
                ref->parent = NULL;
        }
    }
    item->Clear();
    if (delete_item)
        delete item;
}

void Dispose(CConceptClient *Client, GTKControl *item, int delete_children = 1) {
    if (!item)
        return;
    if (item->ptr) {
        Client->Controls.erase(item->ID);
        if (item->ptr)
            Client->ControlsReversed.erase(item->ptr);
        if (item->ptr2)
            Client->ControlsReversed.erase(item->ptr2);
        if (item->ptr3)
            Client->ControlsReversed.erase(item->ptr3);
        if (delete_children == -1) {
            SafeDelete(Client, item);
            return;
        }

        if (item->ptr3) {
            switch (item->type) {
                case CLASS_TREEVIEW:
                case CLASS_ICONVIEW:
                case CLASS_COMBOBOX:
                case CLASS_COMBOBOXTEXT:
                case 1019:
                    delete (TreeData *)item->ptr3;
                    if (item->ptr2)
                        delete (QStringList *)item->ptr2;
                    break;
            }
        }

        QWidget *widget = (QWidget *)item->ptr;
        item->ptr  = NULL;
        item->ptr2 = NULL;
        item->ptr3 = NULL;
        widget->hide();
        return;
        // buggy code 
        QObjectList children            = widget->children();
        QObjectList::const_iterator it  = children.begin();
        QObjectList::const_iterator eIt = children.end();
        int child_count = 0;
        while (it != eIt) {
            QWidget *child = (QWidget *)*it;
            if (child) {
                GTKControl *c = Client->ControlsReversed[child];
                if ((c) && (c != item)) {
                    Dispose(Client, c, delete_children + 1);
                    child_count++;
                }
            }
            ++it;
        }

        QList<QAction *> actions = widget->actions();
        int len = actions.size();
        for (int i = 0; i < len; i++) {
            QAction *action = actions[i];
            if (action) {
                GTKControl *ref2 = Client->ControlsReversed[action];
                if (ref2) {
                    Client->ControlsReversed.erase(action);
                    Client->Controls.erase(ref2->ID);
                    if (delete_children)
                        delete ref2;
                    child_count++;
                }
            }
        }

        NotebookTab *nt2 = (NotebookTab *)widget->property("notebookheader").toULongLong();
        if ((nt2) && (nt2->header)) {
            GTKControl *header = (GTKControl *)nt2->header;
            nt2->header = NULL;
            widget->setProperty("notebookheader", 0);
            Dispose(Client, header, -1);
            delete_children = false;
        }
        GTKControl *parent = (GTKControl *)item->parent;
        if ((parent) && (parent->type == CLASS_NOTEBOOK) && (parent->ptr)) {
            NotebookTab *nt = (NotebookTab *)parent->pages->Item(item->index);
            if (nt) {
                nt->image  = NULL;
                nt->button = NULL;
                nt->object = NULL;
                if (nt->header) {
                    GTKControl *header = (GTKControl *)nt->header;
                    nt->header = NULL;
                    //((QWidget *)header->ptr)->setProperty("notebookheader", 0);
                    Dispose(Client, header, -1);
                }
                if (nt->visible) {
                    int index = ((QTabWidget *)parent->ptr)->indexOf((QWidget *)item->ptr);
                    if (index < 0)
                        index = parent->AdjustIndex(nt);
                    ((QTabWidget *)parent->ptr)->removeTab(index);
                    nt->visible = false;
                }
            }
        } else
        if ((parent) && (parent->type == CLASS_BUTTON))
            ((CustomButton *)parent->ptr)->child = NULL;

        // if nt2 is not null, it will be deleted by the QTabbar (when another control will be set).
        widget->disconnect();
        if ((delete_children) && (!nt2)) {
            if (!child_count)
                delete widget;
        }
        SafeDelete(Client, item, !child_count);
    } else {
        switch (item->type) {
            case CLASS_TOOLBUTTON:
            case CLASS_TOGGLETOOLBUTTON:
            case CLASS_RADIOTOOLBUTTON:
            case CLASS_IMAGEMENUITEM:
            case CLASS_STOCKMENUITEM:
            case CLASS_CHECKMENUITEM:
            case CLASS_MENUITEM:
            case CLASS_TEAROFFMENUITEM:
            case CLASS_RADIOMENUITEM:
            case CLASS_SEPARATORMENUITEM:
                Client->Controls.erase(item->ID);
                Client->ControlsReversed.erase(item->ptr2);
                if (item->ptr3)
                    Client->ControlsReversed.erase(item->ptr3);

                ((QAction *)item->ptr2)->disconnect();
                if (delete_children)
                    ((QAction *)item->ptr2)->deleteLater();
                delete item;
                break;
        }
    }
}

void Packing(QWidget *widget, GTKControl *parent, int pack, GTKControl *item, bool create_call = false) {
    if ((parent) && (item->type != CLASS_FORM)) {
        if ((create_call) && (parent->packtype >= 0))
            pack = parent->packtype;

        char stretch = 1;
        if ((item->type == CLASS_NOTEBOOK) || (item->type == CLASS_TREEVIEW))
            stretch = 4;

        QSizePolicy qs = widget->sizePolicy();
        switch (parent->type) {
            case CLASS_TABLE:
                //if (create_call)
                //    return;
                return;

            case CLASS_EXPANDER:
                {
                    BaseWidget *image = (BaseWidget *)((QWidget *)parent->ptr)->findChild<QLabel *>("image");
                    if (image) {
                        if (pack == MyPACK_SHRINK)
                            image->vpolicy = QSizePolicy::Fixed;
                        else
                            image->vpolicy = QSizePolicy::Expanding;
                    }
                }
                return;

            case CLASS_VPANED:
            case CLASS_HPANED:
                return;

            case CLASS_IMAGEMENUITEM:
            case CLASS_STOCKMENUITEM:
            case CLASS_CHECKMENUITEM:
            case CLASS_MENUITEM:
            case CLASS_TEAROFFMENUITEM:
            case CLASS_RADIOMENUITEM:
            case CLASS_SEPARATORMENUITEM:
            case CLASS_TOOLSEPARATOR:
                return;

            case CLASS_VBUTTONBOX:
            case CLASS_HBUTTONBOX:
                pack = MyPACK_SHRINK;

            case CLASS_VBOX:
            case CLASS_HBOX:
            case CLASS_TOOLBAR:
            case CLASS_MENUBAR:
            case CLASS_STATUSBAR:
                {
                    QSizePolicy::Policy shrinkv = QSizePolicy::Fixed;
                    QSizePolicy::Policy shrinkh = QSizePolicy::Fixed;
                    switch (pack) {
                        case MyPACK_SHRINK:
                            if ((parent) && (item->type != CLASS_IMAGE)) {
                                if ((parent->type == CLASS_VBOX) || (parent->type == CLASS_VBUTTONBOX)) {
                                    if (parent->type != CLASS_VBUTTONBOX)
                                        shrinkh = QSizePolicy::Expanding;
                                    if ((item->type == CLASS_ICONVIEW) || (item->type == CLASS_TEXTVIEW)) {
                                        QSize s = widget->size();
                                        widget->setMaximumSize(QWIDGETSIZE_MAX, s.height());
                                    }
                                } else
                                if ((parent->type == CLASS_HBOX) || (parent->type == CLASS_HBUTTONBOX)) {
                                    if (/*(item->type!=CLASS_SCROLLEDWINDOW) && */ (parent->type != CLASS_HBUTTONBOX))
                                        shrinkv = QSizePolicy::Expanding;
                                    if ((item->type == CLASS_ICONVIEW) || (item->type == CLASS_TEXTVIEW)) {
                                        QSize s = widget->size();
                                        widget->setMaximumSize(s.width(), QWIDGETSIZE_MAX);
                                    }
                                }
                            }
                            if ((item->type == CLASS_COMBOBOX) | (item->type == CLASS_COMBOBOXTEXT) || (item->type == CLASS_EDIT))
                                shrinkh = QSizePolicy::Preferred;

                            qs.setHorizontalPolicy(shrinkh);
                            qs.setVerticalPolicy(shrinkv);
                            if (parent->flags) {
                                qs.setVerticalStretch(1);
                                qs.setHorizontalStretch(1);
                            } else {
                                qs.setVerticalStretch(0);
                                qs.setHorizontalStretch(0);
                            }
                            widget->setSizePolicy(qs);
                            break;

                        case MyPACK_EXPAND_PADDING:
                            if ((item->type == CLASS_COMBOBOX) || (item->type == CLASS_COMBOBOXTEXT) || (item->type == CLASS_BUTTON) || (item->type == CLASS_TOOLBAR))
                                qs.setHorizontalPolicy(QSizePolicy::Expanding);
                            else
                                qs.setHorizontalPolicy(QSizePolicy::Preferred);
                            if (parent->type == CLASS_HBOX)
                                qs.setVerticalPolicy(QSizePolicy::Expanding);
                            else
                                qs.setVerticalPolicy(QSizePolicy::Preferred);
                            if (parent->flags) {
                                qs.setVerticalStretch(1);
                                qs.setHorizontalStretch(1);
                            } else {
                                qs.setVerticalStretch(stretch);
                                qs.setHorizontalStretch(stretch);
                            }
                            widget->setSizePolicy(qs);
                            break;

                        case MyPACK_EXPAND_WIDGET:
                            qs.setHorizontalPolicy(QSizePolicy::Expanding);
                            qs.setVerticalPolicy(QSizePolicy::Expanding);
                            if (parent->flags) {
                                qs.setVerticalStretch(1);
                                qs.setHorizontalStretch(1);
                            } else {
                                qs.setVerticalStretch(stretch);
                                qs.setHorizontalStretch(stretch);
                            }
                            widget->setSizePolicy(qs);

                            //widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
                            //UpdateScrollParents(parent, widget, pack);
                            break;
                    }
                }
                break;

            //case CLASS_EXPANDER:
            //    widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
            //    break;
            case CLASS_NOTEBOOK:
                widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
                break;

            case CLASS_ASPECTFRAME:
                widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
                break;

            case CLASS_BUTTON:
                widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
                break;

            /*case CLASS_SCROLLEDWINDOW:
               case CLASS_VIEWPORT:
                if ((item->type!=CLASS_TREEVIEW) && (item->type!=CLASS_ICONVIEW) && (item->type!=CLASS_NOTEBOOK) && (item->type!=CLASS_TEXTVIEW) && (item->type<1000)) {
                    widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
                    break;
                }*/
            default:
                widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        }
    }
    if (item->type == CLASS_TREEVIEW)
        ((EventAwareWidget<QTreeWidget> *)widget)->UpdateHeight();
}

GTKControl *CreateCustomControl(GTKControl *parent, Parameters *PARAM, int *ref_pack_type) {
    int     cls_id = PARAM->Target.ToInt();
    INVOKER Invoke = GetRequiredInvoker(cls_id);

    if (!Invoke)
        return 0;

    void *box      = 0;
    int  pack_type = 0;
    int  ct_id     = PARAM->Sender.ToInt();
    int  result    = Invoke(parent ? parent->ptr : NULL, INVOKE_CREATE, cls_id, &box, &pack_type);

    if ((result != E_ACCEPTED) && (result != E_ACCEPTED_NOADD))
        return 0;

    *ref_pack_type = pack_type;

    // add the control to the control list
    GTKControl *refNEW = new GTKControl();
    refNEW->ID   = PARAM->Value.ToInt();
    refNEW->type = cls_id;

    if (result == E_ACCEPTED_NOADD)
        refNEW->ptr3 = box;
    else
        refNEW->ptr = box;

    return refNEW;
}

void CreateControl(Parameters *PARAM, CConceptClient *Client) {
    GTKControl *control = NULL;
    int        type     = PARAM->Target.ToInt();
    GTKControl *parent  = Client->Controls[PARAM->Sender.ToInt()];
    QWidget    *pwidget = 0;
    QLayout    *playout = 0;
    int        ID       = PARAM->Value.ToInt();

    if (parent) {
        pwidget = (QWidget *)parent->ptr;
        if (pwidget) {
            if ((parent->type != CLASS_TREEVIEW) && (parent->type != CLASS_TOOLBUTTON) && (parent->type != CLASS_TOGGLETOOLBUTTON) && (parent->type != CLASS_RADIOTOOLBUTTON) && (parent->type != CLASS_MENU))
                playout = (QLayout *)parent->ptr2;
        }
    }

    int packing = MyPACK_EXPAND_WIDGET;
    switch (type) {
        case CLASS_FORM:
            {
                control = new GTKControl();

                QWidget *window = NULL;
                if ((parent) && (parent->type != CLASS_FORM)) {
                    GTKControl *ref_parent = (GTKControl *)parent->parent;
                    while ((ref_parent) && (ref_parent->type != CLASS_FORM))
                        ref_parent = (GTKControl *)ref_parent->parent;
                    QWidget *pwidget2 = NULL;

                    if ((ref_parent) && (ref_parent->type == CLASS_FORM))
                        pwidget2 = (QWidget *)ref_parent->ptr;

                    window = new EventAwareWidget<QWidget>(control, pwidget2);
                } else
                    window = new EventAwareWidget<QWidget>(control, pwidget);
                window->setFocusPolicy(Qt::ClickFocus);
                window->setWindowFlags(window->windowFlags() | Qt::Window);
                window->setWindowModality(Qt::NonModal);
                window->resize(1, 1);
                window->setWindowIcon(QIcon(QString::fromUtf8(":/resources/conceptclienticon.png")));
                control->ptr = window;

                control->ptr2 = new QVBoxLayout(window);
                ((QLayout *)control->ptr2)->setContentsMargins(0, 0, 0, 0);
                ((QLayout *)control->ptr2)->setSpacing(0);
                ((QLayout *)control->ptr2)->setMargin(0);
                window->setWindowTitle(" ");
            }
            break;

        case CLASS_BUTTON:
            {
                control = new GTKControl();

                QPushButton *widget = new EventAwareWidget<CustomButton>(control, pwidget);
                control->ptr = widget;

                control->ptr2 = new QVBoxLayout(widget);
                ((QLayout *)control->ptr2)->setContentsMargins(0, 0, 0, 0);
                ((QLayout *)control->ptr2)->setSpacing(0);
                ((QLayout *)control->ptr2)->setMargin(0);
                packing = MyPACK_SHRINK;
            }
            break;

        case CLASS_CHECKBUTTON:
            {
                control = new GTKControl();

                QCheckBox *widget = new EventAwareWidget<QCheckBox>(control, pwidget);
                control->ptr  = widget;
                control->ptr2 = NULL;
            }
            break;

        case CLASS_RADIOBUTTON:
            {
                control = new GTKControl();

                QRadioButton *widget = new EventAwareWidget<QRadioButton>(control, pwidget);
                control->ptr  = widget;
                control->ptr2 = NULL;
            }
            break;

        case CLASS_LABEL:
            {
                control = new GTKControl();

                QLabel *widget = new EventAwareWidget<VerticalLabel>(control, pwidget);
                widget->setTextFormat(Qt::PlainText);
                widget->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
                control->ptr  = widget;
                control->ptr2 = NULL;
                packing       = MyPACK_SHRINK;
            }
            break;

        case CLASS_VIEWPORT:

        /*{
            control=new GTKControl();

            QScrollArea *widget = new EventAwareWidget<QScrollArea>(control, pwidget);
            widget->setAutoFillBackground(true);
            QPalette p = widget->palette();
            p.setColor(widget->backgroundRole(), p.color(QPalette::Light));
            widget->setPalette(p);
            widget->setMinimumSize(QSize(1,1));
            widget->setFrameStyle(QFrame::NoFrame);
            widget->setFrameShape(QFrame::NoFrame);
            widget->setWidgetResizable(true);
            widget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            widget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            control->ptr=widget;
            control->ptr2=0;
            type=CLASS_SCROLLEDWINDOW;
           }
           break;*/
        case CLASS_EVENTBOX:
        case CLASS_FIXED:
        case CLASS_ASPECTFRAME:
        case CLASS_ALIGNMENT:
        case CLASS_MIRRORBIN:
            {
                control = new GTKControl();

                QWidget *widget = new EventAwareWidget<QWidget>(control, pwidget);
                control->ptr = widget;

                if (type != CLASS_FIXED) {
                    control->ptr2 = new QVBoxLayout(widget);
                    ((QVBoxLayout *)control->ptr2)->setContentsMargins(0, 0, 0, 0);
                    ((QVBoxLayout *)control->ptr2)->setAlignment(Qt::AlignTop | Qt::AlignLeft);
                    ((QVBoxLayout *)control->ptr2)->setSpacing(0);
                    ((QVBoxLayout *)control->ptr2)->setMargin(0);
                }
            }
            break;

        case CLASS_HSEPARATOR:
            {
                control = new GTKControl();

                QFrame *widget = new EventAwareWidget<QFrame>(control, pwidget);
                control->ptr = widget;

                control->ptr2 = NULL;

                widget->setFrameShape(QFrame::HLine);
                widget->setFrameShadow(QFrame::Sunken);
            }
            break;

        case CLASS_VSEPARATOR:
            {
                control = new GTKControl();

                QFrame *widget = new EventAwareWidget<QFrame>(control, pwidget);
                control->ptr = widget;

                control->ptr2 = NULL;

                widget->setFrameShape(QFrame::VLine);
                widget->setFrameShadow(QFrame::Sunken);
            }
            break;

        case CLASS_FRAME:
            {
                control = new GTKControl();

                QGroupBox *widget = new EventAwareWidget<QGroupBox>(control, pwidget);
                control->ptr = widget;

                control->ptr2 = new QVBoxLayout(widget);
                ((QLayout *)control->ptr2)->setContentsMargins(0, 0, 0, 0);
                ((QLayout *)control->ptr2)->setSpacing(0);
                ((QLayout *)control->ptr2)->setMargin(0);
            }
            break;

        case CLASS_PROGRESSBAR:
            {
                control = new GTKControl();

                QProgressBar *widget = new EventAwareWidget<QProgressBar>(control, pwidget);
                widget->setStyleSheet("QProgressBar {text-align: center;}");
                widget->setMinimum(0);
                widget->setMaximum(10000);
                control->ptr = widget;

                control->ptr2 = NULL;
                packing       = MyPACK_SHRINK;
            }
            break;

        case CLASS_VPANED:
            {
                control = new GTKControl();

                QSplitter *widget = new EventAwareWidget<QSplitter>(control, pwidget);
                control->ptr = widget;
                widget->setOrientation(Qt::Vertical);

                control->ptr2 = NULL;
            }
            break;

        case CLASS_HPANED:
            {
                control = new GTKControl();

                QSplitter *widget = new EventAwareWidget<QSplitter>(control, pwidget);
                control->ptr = widget;
                widget->setOrientation(Qt::Horizontal);

                control->ptr2 = NULL;
            }
            break;

        case CLASS_SCROLLEDWINDOW:
            {
                control = new GTKControl();

                QScrollArea *widget = new EventAwareWidget<QScrollArea>(control, pwidget);
                widget->setAutoFillBackground(true);//setStyleSheet(QString("QScrollArea { background: blue;}"));
                QPalette p = widget->palette();
                p.setColor(widget->backgroundRole(), p.color(QPalette::Light));
                widget->setPalette(p);
                widget->setMinimumSize(QSize(1, 1));
                widget->setFrameStyle(QFrame::NoFrame);
                widget->setFrameShape(QFrame::NoFrame);
                widget->setWidgetResizable(true);
                control->ptr = widget;

                control->ptr2 = 0;
            }
            break;

        case CLASS_IMAGE:
            {
                control = new GTKControl();

                QLabel *widget = new EventAwareWidget<QLabel>(control, pwidget);

                //widget->setAlignment(Qt::AlignTop | Qt::AlignLeft);
                control->ptr = widget;

                control->ptr2 = NULL;
                widget->setAlignment(Qt::AlignVCenter | Qt::AlignHCenter);
                //packing = MyPACK_SHRINK;
            }
            break;

        case CLASS_ADJUSTMENT:
            // nothing
            control = new GTKControl();
            if (parent)
                parent->ID_Adjustment = ID;
            break;

        case CLASS_VSCALE:
            {
                control = new GTKControl();

                QSlider *widget = new EventAwareWidget<QSlider>(control, pwidget);
                control->ptr  = widget;
                control->ptr2 = NULL;

                widget->setOrientation(Qt::Vertical);

                GTKControl *adjustment = PARAM->Owner->Controls[PARAM->Owner->POST_OBJECT];
                if ((adjustment) && (adjustment->type == CLASS_ADJUSTMENT)) {
                    control->ID_Adjustment = PARAM->Owner->POST_OBJECT;
                    adjustment->parent     = control;
                    SetAdjustment(PARAM->Owner, adjustment);
                }
            }
            break;

        case CLASS_HSCALE:
            {
                control = new GTKControl();

                QSlider *widget = new EventAwareWidget<QSlider>(control, pwidget);
                control->ptr  = widget;
                control->ptr2 = NULL;

                widget->setOrientation(Qt::Horizontal);

                GTKControl *adjustment = PARAM->Owner->Controls[PARAM->Owner->POST_OBJECT];
                if ((adjustment) && (adjustment->type == CLASS_ADJUSTMENT)) {
                    control->ID_Adjustment = PARAM->Owner->POST_OBJECT;
                    adjustment->parent     = control;
                    SetAdjustment(PARAM->Owner, adjustment);
                }
            }
            break;

        case CLASS_VSCROLLBAR:
            {
                control = new GTKControl();

                QScrollBar *widget = new EventAwareWidget<QScrollBar>(control, pwidget);
                control->ptr  = widget;
                control->ptr2 = NULL;

                widget->setOrientation(Qt::Vertical);

                GTKControl *adjustment = PARAM->Owner->Controls[PARAM->Owner->POST_OBJECT];
                if ((adjustment) && (adjustment->type == CLASS_ADJUSTMENT)) {
                    control->ID_Adjustment = PARAM->Owner->POST_OBJECT;
                    adjustment->parent     = control;
                    SetAdjustment(PARAM->Owner, adjustment);
                }
            }
            break;

        case CLASS_HSCROLLBAR:
            {
                control = new GTKControl();

                QScrollBar *widget = new EventAwareWidget<QScrollBar>(control, pwidget);
                control->ptr  = widget;
                control->ptr2 = NULL;

                widget->setOrientation(Qt::Horizontal);

                GTKControl *adjustment = PARAM->Owner->Controls[PARAM->Owner->POST_OBJECT];
                if ((adjustment) && (adjustment->type == CLASS_ADJUSTMENT)) {
                    control->ID_HAdjustment = PARAM->Owner->POST_OBJECT;
                    adjustment->parent      = control;
                    SetAdjustment(PARAM->Owner, adjustment);
                }
            }
            break;

        case CLASS_SPINBUTTON:
            {
                control = new GTKControl();

                QSpinBox *widget = new EventAwareWidget<QSpinBox>(control, pwidget);
                control->ptr  = widget;
                control->ptr2 = NULL;

                GTKControl *adjustment = PARAM->Owner->Controls[PARAM->Owner->POST_OBJECT];
                if ((adjustment) && (adjustment->type == CLASS_ADJUSTMENT)) {
                    control->ID_Adjustment = PARAM->Owner->POST_OBJECT;
                    adjustment->parent     = control;
                }
            }
            break;

        case CLASS_MENUBAR:
            {
                control = new GTKControl();

                QMenuBar *widget = new EventAwareWidget<QMenuBar>(control, pwidget);
#ifdef __APPLE__
                if (pwidget)
                    widget->setNativeMenuBar(false);
                else
                    widget->setNativeMenuBar(true);
#endif
                widget->setNativeMenuBar(false);
                control->ptr  = widget;
                control->ptr2 = NULL;

                packing = MyPACK_SHRINK;
            }
            break;

        case CLASS_MENU:
            {
                control = new GTKControl();

                QMenu *widget = new EventAwareWidget<QMenu>(control, pwidget);
                if ((parent) && (parent->ptr2)) {
                    switch (parent->type) {
                        case CLASS_TOOLBUTTON:
                        case CLASS_TOGGLETOOLBUTTON:
                        case CLASS_RADIOTOOLBUTTON:
                        case CLASS_IMAGEMENUITEM:
                        case CLASS_STOCKMENUITEM:
                        case CLASS_CHECKMENUITEM:
                        case CLASS_MENUITEM:
                        case CLASS_TEAROFFMENUITEM:
                        case CLASS_RADIOMENUITEM:
                        case CLASS_SEPARATORMENUITEM:
                            ((QAction *)parent->ptr2)->setMenu(widget);
                            ((QAction *)parent->ptr2)->setText(((QAction *)parent->ptr2)->text());
                            ((QAction *)parent->ptr2)->setIconText(((QAction *)parent->ptr2)->iconText());
                            break;
                    }
                }


                control->ptr  = widget;
                control->ptr2 = NULL;

                packing = MyPACK_SHRINK;
            }
            break;

        case CLASS_IMAGEMENUITEM:
        case CLASS_STOCKMENUITEM:
        case CLASS_CHECKMENUITEM:
        case CLASS_MENUITEM:
        case CLASS_TEAROFFMENUITEM:
        //type=CLASS_MENUITEM;
        case CLASS_RADIOMENUITEM:
        case CLASS_SEPARATORMENUITEM:
            {
                QAction *action = NULL;
                if (parent) {
                    if (parent->type == CLASS_MENUBAR) {
                        if (type == CLASS_SEPARATORMENUITEM)
                            action = ((QMenuBar *)(parent->ptr))->addSeparator();
                        else
                        if (type == CLASS_STOCKMENUITEM) {
                            AnsiString temp;
                            StockName(PARAM->Owner->POST_STRING.c_str(), temp);
                            action = ((QMenuBar *)(parent->ptr))->addAction(QObject::tr(temp.c_str()));
                            action->setIcon(QIcon(StockIcon(PARAM->Owner->POST_STRING)));
                            //action->setIconText(QString::fromUtf8(temp.c_str()));
                        } else
                            action = ((QMenuBar *)(parent->ptr))->addAction(QString::fromUtf8(ConvertCaption(PARAM->Owner->POST_STRING).c_str()));
                    } else
                    if (parent->type == CLASS_MENU) {
                        if (type == CLASS_SEPARATORMENUITEM)
                            action = ((QMenu *)(parent->ptr))->addSeparator();
                        else
                        if (type == CLASS_STOCKMENUITEM) {
                            AnsiString temp;
                            StockName(PARAM->Owner->POST_STRING.c_str(), temp);
                            action = ((QMenu *)(parent->ptr))->addAction(QObject::tr(temp.c_str()));
                            action->setIcon(QIcon(StockIcon(PARAM->Owner->POST_STRING)));
                            //action->setIconText(QString::fromUtf8(temp.c_str()));
                        } else
                            action = ((QMenu *)(parent->ptr))->addAction(QString::fromUtf8(ConvertCaption(PARAM->Owner->POST_STRING).c_str()));
                    }
                }
                if (action) {
                    control       = new GTKControl();
                    control->ptr  = NULL;
                    control->ptr2 = action;
                    action->setVisible(false);
                    if ((type == CLASS_CHECKMENUITEM) || (type == CLASS_CHECKMENUITEM))
                        action->setCheckable(true);
                    Client->ControlsReversed[action] = control;
                }
            }
            break;

        case CLASS_TOOLBAR:
            {
                control = new GTKControl();

                QToolBar *widget = new EventAwareWidget<QToolBar>(control, pwidget);
                widget->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
                widget->setMovable(true);
                control->ptr  = widget;
                control->ptr2 = NULL;

                packing = MyPACK_SHRINK;
            }
            break;

        case CLASS_MENUTOOLBUTTON:
            type = CLASS_TOOLBUTTON;

        case CLASS_TOGGLETOOLBUTTON:
        case CLASS_TOOLBUTTON:
        case CLASS_RADIOTOOLBUTTON:
        case CLASS_TOOLSEPARATOR:
            {
                control = new GTKControl();

                QAction     *action = NULL;
                QToolButton *widget = NULL;
                if ((parent) && (parent->type == CLASS_TOOLBAR)) {
                    if (type == CLASS_TOOLSEPARATOR) {
                        action = ((QToolBar *)(parent->ptr))->addSeparator();
                    } else {
                        widget = new EventAwareWidget<QToolButton>(control, pwidget);

                        action = new QAction("", widget);
                        action->setVisible(false);
                        ((QToolBar *)(parent->ptr))->addAction(action);
                        control->ptr = widget;
                    }
                    control->ptr2 = action;
                    Client->ControlsReversed[action] = control;
                } else {
                    widget        = new EventAwareWidget<QToolButton>(control, pwidget);
                    control->ptr  = widget;
                    control->ptr2 = NULL;
                }

                if ((type == CLASS_TOGGLETOOLBUTTON) || (type == CLASS_RADIOTOOLBUTTON)) {
                    if (action)
                        action->setCheckable(true);
                    if (widget)
                        widget->setCheckable(true);
                }
                packing = MyPACK_SHRINK;
            }
            break;

        case CLASS_EXPANDER:
            {
                control = new GTKControl();

                //QToolBox *widget = new QToolBox(pwidget);
                QWidget     *widget  = new EventAwareWidget<QWidget>(control, pwidget);
                QVBoxLayout *mainbox = new QVBoxLayout(widget);
                mainbox->setContentsMargins(0, 0, 0, 0);
                mainbox->setSpacing(0);
                mainbox->setMargin(0);

                QHBoxLayout *box = new QHBoxLayout();
                box->setContentsMargins(0, 0, 0, 0);
                box->setSpacing(5);
                box->setMargin(0);

                mainbox->addLayout(box);

                QLabel *image = new BaseWidget(widget);

                QStyle *l_style = QApplication::style();

                //image->setPixmap(l_style->standardPixmap(QStyle::SP_ArrowUp));
                image->setText(QChar(0x25BA));
                image->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
                image->setObjectName(QString::fromUtf8("image"));
                image->show();

                box->addWidget(image);

                QLabel *label = new BaseWidget(widget);
                label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
                label->setObjectName(QString::fromUtf8("label"));
                label->show();

                box->addWidget(label);

                ClientEventHandler *events = new ClientEventHandler(ID, Client);
                control->events = events;

                QWidget *container = new QWidget(widget);
                container->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
                container->setObjectName(QString::fromUtf8("container"));
                container->hide();
                mainbox->addWidget(container);

                control->ptr  = widget;
                control->ptr2 = new QVBoxLayout(container);
                ((QLayout *)control->ptr2)->setContentsMargins(0, 0, 0, 0);
                ((QLayout *)control->ptr2)->setSpacing(0);
                ((QLayout *)control->ptr2)->setMargin(0);
                packing = MyPACK_SHRINK;
                //QObject::connect(image, SIGNAL(mousePressEvent(QMouseEvent *)), events, SLOT(on_expander_toggled(QMouseEvent *)));
                //QObject::connect(label, SIGNAL(mousePressEvent(QMouseEvent *)), events, SLOT(on_expander_toggled(QMouseEvent *)));
            }
            break;

        case CLASS_HANDLEBOX:
            {
                control = new GTKControl();

                QDockWidget *widget = new EventAwareWidget<QDockWidget>(control, pwidget);
                control->ptr = widget;

                control->ptr2 = NULL;
            }
            break;

        case CLASS_STATUSBAR:
            {
                control = new GTKControl();

                QStatusBar *widget = new EventAwareWidget<QStatusBar>(control, pwidget);
                control->ptr = widget;

                control->ptr2 = widget->layout();
                packing       = MyPACK_SHRINK;
            }
            break;

        case CLASS_CALENDAR:
            {
                control = new GTKControl();

                QCalendarWidget *widget = new EventAwareWidget<QCalendarWidget>(control, pwidget);
                control->ptr = widget;

                control->ptr2 = NULL;
                packing       = MyPACK_SHRINK;
            }
            break;

        case CLASS_ICONVIEW:
            {
                control = new GTKControl();

                QListWidget *widget = new EventAwareWidget<QListWidget>(control, pwidget);
                if ((parent) && (parent->type == CLASS_SCROLLEDWINDOW)) {
                    if (((QScrollArea *)parent->ptr)->horizontalScrollBarPolicy() != Qt::ScrollBarAlwaysOn)
                        widget->setHorizontalScrollBarPolicy(((QScrollArea *)parent->ptr)->horizontalScrollBarPolicy());

                    if (((QScrollArea *)parent->ptr)->verticalScrollBarPolicy() != Qt::ScrollBarAlwaysOn)
                        widget->setVerticalScrollBarPolicy(((QScrollArea *)parent->ptr)->verticalScrollBarPolicy());
                }
                //widget->setFlow(QListView::LeftToRight);
                widget->setMouseTracking(true);
                widget->setMinimumSize(QSize(1, 1));
                widget->setViewMode(QListView::IconMode);
                widget->setMovement(QListView::Static);
                widget->setItemDelegate(new ListDelegate(control));
                widget->setWrapping(true);
                widget->setResizeMode(QListView::Adjust);
                widget->setLayoutMode(QListView::Batched);
                widget->setFrameShape(QFrame::NoFrame);
                widget->setLineWidth(0);
                control->ptr  = widget;
                control->ptr3 = new TreeData();
                control->ptr2 = new QStringList();
                //packing=MyPACK_SHRINK;
            }
            break;

        case CLASS_TREEVIEW:
            {
                control = new GTKControl();

                QTreeWidget *widget = new EventAwareWidget<QTreeWidget>(control, pwidget);
                //widget->setHeader(new DHeaderView(widget));
                //widget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                //widget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                if ((parent) && (parent->type == CLASS_SCROLLEDWINDOW)) {
                    if (((QScrollArea *)parent->ptr)->horizontalScrollBarPolicy() != Qt::ScrollBarAlwaysOn)
                        widget->setHorizontalScrollBarPolicy(((QScrollArea *)parent->ptr)->horizontalScrollBarPolicy());

                    /*else {
                        widget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
                        ((QScrollArea *)parent->ptr)->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                       }*/

                    if (((QScrollArea *)parent->ptr)->verticalScrollBarPolicy() != Qt::ScrollBarAlwaysOn)
                        widget->setVerticalScrollBarPolicy(((QScrollArea *)parent->ptr)->verticalScrollBarPolicy());

                    /*else {
                        widget->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
                        ((QScrollArea *)parent->ptr)->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                       }*/
                }
                widget->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
                widget->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
                widget->setEditTriggers(QAbstractItemView::EditKeyPressed | QAbstractItemView::SelectedClicked | QAbstractItemView::AnyKeyPressed);
                widget->setMinimumSize(QSize(1, 1));
                widget->setAnimated(true);
                widget->setFrameShape(QFrame::NoFrame);
                widget->setLineWidth(0);

                control->ptr  = widget;
                control->ptr2 = new QStringList();
                widget->setColumnCount(0);
                widget->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
                control->ptr3 = new TreeData();
            }
            break;

        case CLASS_COMBOBOX:
            {
                control = new GTKControl();

                QComboBox *widget = new EventAwareWidget<QComboBox>(control, pwidget);
                control->ptr  = widget;
                control->ptr3 = new TreeData();
                control->ptr2 = new QStringList();
                packing       = MyPACK_SHRINK;
            }
            break;

        case CLASS_COMBOBOXTEXT:
            {
                control = new GTKControl();

                QComboBox *widget = new EventAwareWidget<QComboBox>(control, pwidget);
                widget->setEditable(true);
                control->ptr  = widget;
                control->ptr3 = new TreeData();
                control->ptr2 = new QStringList();
                packing       = MyPACK_SHRINK;
            }
            break;

        case CLASS_EDIT:
            {
                control = new GTKControl();

                QLineEdit *widget = new EventAwareWidget<QLineEdit>(control, pwidget);
                control->ptr = widget;

                control->ptr2 = NULL;
            }
            break;

        case CLASS_TEXTVIEW:
            {
                control = new GTKControl();

                QTextEdit *widget = new EventAwareWidget<QTextEdit>(control, pwidget);
                if ((parent) && (parent->type == CLASS_SCROLLEDWINDOW)) {
                    if (((QScrollArea *)parent->ptr)->horizontalScrollBarPolicy() != Qt::ScrollBarAlwaysOn)
                        widget->setHorizontalScrollBarPolicy(((QScrollArea *)parent->ptr)->horizontalScrollBarPolicy());

                    if (((QScrollArea *)parent->ptr)->verticalScrollBarPolicy() != Qt::ScrollBarAlwaysOn)
                        widget->setVerticalScrollBarPolicy(((QScrollArea *)parent->ptr)->verticalScrollBarPolicy());
                }
                widget->setFrameShape(QFrame::NoFrame);
                widget->setLineWidth(0);
                widget->setMinimumSize(QSize(1, 1));
                control->ptr = widget;

                control->ptr2 = NULL;
            }
            break;

        case CLASS_TEXTTAG:
            {
                control = new GTKControl();

                QTextCharFormat *fmt = new QTextCharFormat();
                control->ptr2 = fmt;
            }
            break;

        case CLASS_NOTEBOOK:
            {
                control = new GTKControl();
                QTabWidget *widget = new EventAwareWidget<QTabWidget>(control, pwidget);
                control->ptr   = widget;
                control->ptr2  = NULL;
                control->pages = new AnsiList(0);
            }
            break;

        case CLASS_VBOX:
        case CLASS_VBUTTONBOX:
            {
                control = new GTKControl();
                QWidget *widget = new EventAwareWidget<QWidget>(control, pwidget);

                control->ptr  = widget;
                control->ptr2 = new QVBoxLayout(widget);
                ((QLayout *)control->ptr2)->setContentsMargins(0, 0, 0, 0);
                ((QLayout *)control->ptr2)->setSpacing(0);
                ((QLayout *)control->ptr2)->setMargin(0);
                ((QLayout *)control->ptr2)->setAlignment(Qt::AlignTop | Qt::AlignJustify);

                if (type == CLASS_VBUTTONBOX) {
                    ((QLayout *)control->ptr2)->setSpacing(5);
                    QSpacerItem *item = new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Expanding);
                    ((QBoxLayout *)control->ptr2)->addSpacerItem(item);
                }
            }
            break;

        case CLASS_HBOX:
        case CLASS_HBUTTONBOX:
            {
                control = new GTKControl();

                QWidget *widget = new EventAwareWidget<QWidget>(control, pwidget);
                control->ptr  = widget;
                control->ptr2 = new QHBoxLayout(widget);
                ((QLayout *)control->ptr2)->setContentsMargins(0, 0, 0, 0);
                ((QLayout *)control->ptr2)->setSpacing(0);
                ((QLayout *)control->ptr2)->setMargin(0);
                ((QLayout *)control->ptr2)->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

                if (type == CLASS_HBUTTONBOX) {
                    ((QLayout *)control->ptr2)->setSpacing(5);
                    QSpacerItem *item = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Fixed);
                    ((QBoxLayout *)control->ptr2)->addSpacerItem(item);
                }
            }
            break;

        case CLASS_TABLE:
            {
                control = new GTKControl();

                QWidget *widget = new EventAwareWidget<QWidget>(control, pwidget);
                control->extraptr = new int[8];
                for (int i = 0; i < 8; i++)
                    control->extraptr[i] = 0;
                control->ptr  = widget;
                control->ptr2 = new QGridLayout(widget);
                ((QLayout *)control->ptr2)->setContentsMargins(0, 0, 0, 0);
                ((QLayout *)control->ptr2)->setSpacing(0);
                ((QLayout *)control->ptr2)->setMargin(0);
                ((QLayout *)control->ptr2)->setAlignment(Qt::AlignTop | ((QLayout *)control->ptr2)->alignment());

                //((QGridLayout *)control->ptr2)->setHorizontalSpacing(5);
                //((QGridLayout *)control->ptr2)->setVerticalSpacing(5);
                packing = MyPACK_SHRINK;
            }
            break;

#ifndef NO_WEBKIT
        case CLASS_HTMLSNAP:
        case CLASS_CLIENTCHART:
        case 1001:
        case 1012:
            {
                //type=1012;
                control = new GTKControl();
                QWebSettings *defaultSettings = QWebSettings::globalSettings();
                if (defaultSettings) {
                    QSettings settings;
                    settings.beginGroup(QLatin1String("websettings"));

                    defaultSettings->setAttribute(QWebSettings::JavascriptEnabled, settings.value(QLatin1String("enableJavascript"), true).toBool());
                    defaultSettings->setAttribute(QWebSettings::PluginsEnabled, settings.value(QLatin1String("enablePlugins"), true).toBool());
                    defaultSettings->setAttribute(QWebSettings::DeveloperExtrasEnabled, settings.value(QLatin1String("enableDeveloperExtras"), true).toBool());
                }

                QWebView       *widget  = new EventAwareWidget<QWebView>(control, pwidget);
                ManagedRequest *manager = new ManagedRequest(QString::fromUtf8(temp_dir));
                widget->setPage(new ManagedPage(control, widget));
                widget->page()->setNetworkAccessManager(manager);

                control->ptr  = widget;
                control->ptr2 = NULL;
                if (type == CLASS_HTMLSNAP) {
                    if (PARAM->Owner->POST_STRING.Length())
                        control->ptr3 = new AnsiString(PARAM->Owner->POST_STRING);
                    std::string full = PARAM->Owner->snapclasses_full[PARAM->Owner->POST_STRING.c_str()];
                    if (full.size()) {
                        widget->setHtml(QString::fromUtf8(full.c_str()), QUrl(QString("file:///static/")));
                    } else {
                        std::string header = PARAM->Owner->snapclasses_header[PARAM->Owner->POST_STRING.c_str()];
                        std::string body = PARAM->Owner->snapclasses_body[PARAM->Owner->POST_STRING.c_str()];
                        std::string html = "<html><head>";
                        html += header;
                        html += "</head><body>";
                        html += body;
                        html += "</body></html>";
                        widget->setHtml(QString::fromUtf8(html.c_str()), QUrl(QString("file:///static/")));
                    }
                }
            }
            break;
#endif
        case 1002:
            {
                //type=1012;
                control = new GTKControl();
                QVideoWidget *widget = new EventAwareWidget<QVideoWidget>(control, pwidget);
                QPalette     Pal     = widget->palette();
                Pal.setColor(QPalette::Background, Qt::black);
                widget->setAutoFillBackground(true);
                widget->setPalette(Pal);

                QMediaPlayer *mplayer = new QMediaPlayer(widget, QMediaPlayer::StreamPlayback);
                mplayer->setVideoOutput(widget);

                control->ptr  = widget;
                control->ptr2 = mplayer;
            }
            break;

        // sheet
        case 1019:
            {
                control = new GTKControl();
                QString     s(QString::fromUtf8(PARAM->Owner->POST_STRING.c_str()));
                QStringList list = s.split(QString(";"), QString::KeepEmptyParts);

                QTableWidget *widget = new EventAwareWidget<QTableWidget>(control, pwidget);
                control->ptr  = widget;
                control->ptr3 = new TreeData();
                control->ptr2 = new QStringList();

                int rows    = 0;
                int columns = 0;

                if (list.size() > 0) {
                    if (!list[0].toInt())
                        widget->setEditTriggers(QAbstractItemView::NoEditTriggers);
                }
                if (list.size() > 1)
                    rows = list[1].toInt();

                if (list.size() > 2)
                    columns = list[2].toInt();

                char str[2];
                str[1] = 0;
                for (int i = 0; i < columns; i++) {
                    if (i < 26) {
                        str[0] = 'A' + i;
                        *(QStringList *)control->ptr2 << QString(str);
                    } else
                        *(QStringList *)control->ptr2 << QString::number(i);
                }
                widget->setColumnCount(columns);
                widget->setHorizontalHeaderLabels(*(QStringList *)control->ptr2);

                QStringList vertical;
                for (int i = 0; i < rows; i++)
                    vertical << QString::number(i + 1);

                widget->setRowCount(rows);
                widget->setVerticalHeaderLabels(vertical);
            }
            break;

        case 1015:
            {
                QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
                int capture_device_id      = PARAM->Owner->POST_STRING.ToInt();
                if (!cameras.size())
                    return;
                QCameraInfo info;
                if ((capture_device_id < 0) || (capture_device_id >= cameras.size())) {
                    info = QCameraInfo::defaultCamera();
                } else
                    info = cameras[capture_device_id];

                QCamera *camera = new QCamera(info);
                QCameraImageCapture *imageCapture = new QCameraImageCapture(camera);
                imageCapture->setCaptureDestination(QCameraImageCapture::CaptureToBuffer);

                camera->setCaptureMode(QCamera::CaptureStillImage);
                control = new GTKControl();

                ClientEventHandler *events = new ClientEventHandler(ID, Client);
                control->events = events;

                QObject::connect(imageCapture, SIGNAL(imageCaptured(int, QImage)), events, SLOT(on_imageCaptured(int, QImage)));
                if (pwidget) {
                    QCameraViewfinder *widget = new EventAwareWidget<QCameraViewfinder>(control, pwidget);
                    camera->setViewfinder(widget);
                    control->ptr = widget;
                } else {
                    QCameraViewfinder *widget = new QCameraViewfinder();
                    camera->setViewfinder(widget);
                    control->ptr = NULL;
                }
                control->ptr3 = imageCapture;
                camera->start();
            }
            break;
            
        case 1016:
            {
                control = new GTKControl();
                control->ptr3 = new AudioObject(Client, ID);
            }
            break;

        case 1017:
            {
                QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
                int capture_device_id      = 0;//PARAM->Owner->POST_STRING.ToInt();
                if (cameras.size()) {
                    control = new GTKControl();
                    ClientEventHandler *events = new ClientEventHandler(ID, Client);
                    control->events = events;
                }
            }
            break;

        case CLASS_PROPERTIESBOX:
            {
                control = new GTKControl();

                QtTreePropertyBrowser *widget = new EventAwareWidget<QtTreePropertyBrowser>(control, pwidget);
                widget->setHeaderVisible(false);
                control->ptr = widget;
                widget->setPropertiesWithoutValueMarked(true);
                widget->setIndentation(5);
                //widget->setRootIsDecorated(false);

                QtVariantPropertyManager *variantManager = new QtVariantPropertyManager();
                QtVariantEditorFactory   *variantFactory = new QtVariantEditorFactory();

                control->ptr3 = variantManager;

                widget->setFactoryForManager(variantManager, variantFactory);

                //control->ptr2=NULL;
                //control->ptr3=variantManager;
            }
            break;

        default:
            {
                control = CreateCustomControl(parent, PARAM, &packing);
                if (packing < 0) {
                    parent  = NULL;
                    pwidget = NULL;
                }
                if (!control)
                    fprintf(stderr, "Unknown control type %i\n", type);
            }
    }
    if (control) {
        control->type   = type;
        control->ID     = ID;
        control->parent = parent;

        Client->Controls[control->ID] = control;

        if (control->ptr) {
            Client->ControlsReversed[control->ptr] = control;
            AnsiString name("rid");
            name += AnsiString((long)ID);
            ((QWidget *)control->ptr)->setObjectName(name.c_str());
        } else
        if (control->ptr3)
            Client->ControlsReversed[control->ptr3] = control;

        if (control->ptr2)
            Client->ControlsReversed[control->ptr2] = control;

        if ((parent) && (control->ptr) && (control->type != CLASS_FORM) && (packing >= 0)) {
            if (parent->type != CLASS_EVENTBOX)
                control->ref = parent->ref;
            control->index = parent->index;
            if ((control->ref) /*&& ((control->type==CLASS_EVENTBOX) || (control->type==CLASS_BUTTON))*/) {
                NotebookTab *nt = (NotebookTab *)((GTKControl *)control->ref)->pages->Item(parent->index);
                if (nt)
                    MarkRecursive(Client, (GTKControl *)control->ref, parent->index, (QWidget *)control->ptr, nt, ((GTKControl *)control->ref)->AdjustIndex(nt));
            }

            if (parent->type == CLASS_NOTEBOOK) {
                AnsiString page = "Page ";
                control->index = parent->pages->Count();
                page          += AnsiString(control->index + 1);
                NotebookTab *nt = new NotebookTab();
                nt->title = page;

                parent->pages->Add(nt, 0);
            } else
            if (parent->type == CLASS_TABLE) {
                // left 0
                // right 1
                // top 2
                // bottom 3
                // hopt 4
                // vopt 5
                // hspacing 6
                // vspacing 7

                ((QGridLayout *)playout)->addWidget((QWidget *)control->ptr, parent->extraptr[2], parent->extraptr[0], parent->extraptr[3] - parent->extraptr[2], parent->extraptr[1] - parent->extraptr[0]);

                QSizePolicy::Policy shrinkh = QSizePolicy::Fixed;
                QSizePolicy::Policy shrinkv = QSizePolicy::Fixed;

                char vstretch = -1;//parent->extraptr[3]-parent->extraptr[2];
                char hstretch = parent->extraptr[1] - parent->extraptr[0];
                //if ((control->type==CLASS_NOTEBOOK) || (control->type==CLASS_TREEVIEW) || (control->type==CLASS_VBOX) || (control->type==CLASS_SCROLLEDWINDOW))
                //    hstretch=100;
                //if ((control->type==CLASS_NOTEBOOK) || (control->type==CLASS_TREEVIEW))
                //    stretch=4;
                //if (parent->extraptr[4]!=2)
                switch (parent->extraptr[4]) {
                    case 1:
                    case 5:
                        if ((control->type == CLASS_COMBOBOXTEXT) || (control->type == CLASS_EDIT))
                            shrinkh = QSizePolicy::MinimumExpanding;
                        else
                            shrinkh = QSizePolicy::Expanding;
                        break;

                    case 3:
                    case 7:
                        shrinkh = QSizePolicy::MinimumExpanding;
                        break;

                    case 6: // SHRINK | FILL
                        //if ((control->type!=CLASS_BUTTON) && (control->type!=CLASS_EVENTBOX))
                        //    hstretch = -1;
                        if (type != CLASS_LABEL)
                            shrinkh = QSizePolicy::Preferred;
                        //else
                        //  hstretch=-1;
                        //else
                        //  hstretch=-1;
                        //else
                        //  shrinkh=QSizePolicy::Minimum;
                        break;

                    case 2:
                        shrinkh = QSizePolicy::Fixed;
                        break;
                }

                //if (parent->extraptr[5]!=2)
                //    shrinkv=QSizePolicy::Expanding;
                if (control->type != CLASS_EXPANDER) {
                    switch (parent->extraptr[5]) {
                        case 1:
                        case 5:
                            ((QLayout *)parent->ptr2)->setAlignment(0);
                            shrinkv = QSizePolicy::Expanding;
                            break;

                        case 3:
                        case 7:
                            shrinkv = QSizePolicy::MinimumExpanding;
                            break;

                        case 6: // SHRINK | FILL
                            if (type == CLASS_LABEL)
                                shrinkv = QSizePolicy::Preferred;
                            //if (type==CLASS_LABEL)
                            //  vstretch=-1;
                            //shrinkv=QSizePolicy::Preferred;
                            break;

                        case 2:
                            shrinkv = QSizePolicy::Fixed;
                            break;
                    }
                } else {
                    BaseWidget *image = (BaseWidget *)((QWidget *)control->ptr)->findChild<QLabel *>("image");
                    if (image) {
                        switch (parent->extraptr[5]) {
                            case 1:
                            case 5:
                                image->vpolicy = QSizePolicy::Expanding;
                                break;

                            case 6: // SHRINK | FILL
                                image->vpolicy = QSizePolicy::Fixed;
                                break;
                        }
                    }
                }

                /*if ((control->type==CLASS_HBOX) && (shrinkv==QSizePolicy::Fixed)) {
                    vstretch=0;
                    ((QWidget *)control->ptr)->setMaximumHeight(22);
                   }*/


                QSizePolicy qs = ((QWidget *)control->ptr)->sizePolicy();
                qs.setHorizontalPolicy(shrinkh);
                qs.setVerticalPolicy(shrinkv);
                if (parent->flags) {
                    qs.setVerticalStretch(1);
                    qs.setHorizontalStretch(1);
                } else {
                    if (vstretch >= 0)
                        qs.setVerticalStretch(vstretch);
                    if (hstretch >= 0)
                        qs.setHorizontalStretch(hstretch);
                }
                ((QWidget *)control->ptr)->setSizePolicy(qs);
                //((QWidget *)control->ptr)->setSizePolicy(shrinkh, shrinkv);
                if (control->type == CLASS_TREEVIEW)
                    ((EventAwareWidget<QTreeWidget> *)control->ptr)->UpdateHeight();

                ((QWidget *)control->ptr)->setContentsMargins(parent->extraptr[6], parent->extraptr[7], parent->extraptr[6], parent->extraptr[7]);
            } else
            if (parent->type == CLASS_SCROLLEDWINDOW) {
                if (!parent->flags)
                    ((QScrollArea *)(parent->ptr))->setWidget((QWidget *)control->ptr);
                //if (playout)
                //    playout->addWidget((QWidget *)control->ptr);
            } else
            if ((parent->type == CLASS_HPANED) || (parent->type == CLASS_VPANED)) {
                ((QSplitter *)(parent->ptr))->addWidget((QWidget *)control->ptr);
                if (playout)
                    playout->addWidget((QWidget *)control->ptr);
            } else
            if ((parent->type == CLASS_TOOLBUTTON) || (parent->type == CLASS_TOGGLETOOLBUTTON) || (parent->type == CLASS_RADIOTOOLBUTTON)) {
                return;
            } else
            if (parent->type == CLASS_STATUSBAR) {
                ((QStatusBar *)(parent->ptr))->addWidget((QWidget *)control->ptr);
            } else
            if (parent->type == CLASS_HANDLEBOX) {
                if (!parent->flags)
                    ((QDockWidget *)(parent->ptr))->setWidget((QWidget *)control->ptr);
            } else
            if ((parent->type == CLASS_HPANED) || (parent->type == CLASS_VPANED)) {
                ((QSplitter *)(parent->ptr))->addWidget((QWidget *)control->ptr);
            } else
            if ((parent->type == CLASS_VBOX) || (parent->type == CLASS_VBUTTONBOX) ||
                (parent->type == CLASS_ALIGNMENT) || (parent->type == CLASS_EVENTBOX) ||
                (parent->type == CLASS_ASPECTFRAME) || (parent->type == CLASS_VIEWPORT)) {
                ((QVBoxLayout *)playout)->addWidget((QWidget *)control->ptr);
            } else
            if ((parent->type == CLASS_HBOX) || (parent->type == CLASS_HBUTTONBOX)) {
                ((QHBoxLayout *)playout)->addWidget((QWidget *)control->ptr);
            } else
            if (control->type == CLASS_MENU) {
                if (parent->type == CLASS_MENUBAR)
                    control->ptr2 = ((QMenuBar *)(parent->ptr))->addMenu((QMenu *)control->ptr);
                else
                if (parent->type == CLASS_MENU)
                    control->ptr2 = ((QMenu *)(parent->ptr))->addMenu((QMenu *)control->ptr);
            } else
            if (playout) {
                if (parent->type == CLASS_BUTTON)
                    ((CustomButton *)parent->ptr)->child = control;
                playout->addWidget((QWidget *)control->ptr);
            }
        }

        if (control->ptr) {
            ((QWidget *)control->ptr)->hide();
            if ((parent) && (control->type != CLASS_FORM))
                Packing(((QWidget *)control->ptr), parent, packing, control, true);
        }
    }
}

int compensated_send_message(AnsiString& Sender, int msg_id, AnsiString& Target, AnsiString& Value, void *UserData) {
    Parameters PARAM;

    PARAM.Owner  = (CConceptClient *)UserData;
    PARAM.Sender = Sender;
    PARAM.Target = Target;
    PARAM.Value  = Value;
    PARAM.ID     = msg_id;

    ((CConceptClient *)UserData)->OUT_PARAM.ID = 0;
    MESSAGE_CALLBACK(&PARAM, &((CConceptClient *)UserData)->OUT_PARAM);
    return 1;
}

int compensated_wait_message(AnsiString& Sender, int msg_id, AnsiString& Target, AnsiString& Value, void *UserData) {
    if (((CConceptClient *)UserData)->OUT_PARAM.ID == msg_id) {
        Sender = ((CConceptClient *)UserData)->OUT_PARAM.Sender;
        Target = ((CConceptClient *)UserData)->OUT_PARAM.Target;
        Value  = ((CConceptClient *)UserData)->OUT_PARAM.Value;
        ((CConceptClient *)UserData)->OUT_PARAM.ID = 0;
        return 1;
    }
    ((CConceptClient *)UserData)->OUT_PARAM.ID = 0;
    Sender = "";
    Value  = "";
    Target = "";
    return -1;
}

void SetProperty(Parameters *PARAM, CConceptClient *Client) {
    GTKControl *item = Client->Controls[PARAM->Sender.ToInt()];

    if (!item)
        return;

    int  val;
    bool property_set = false;
    int  prop_id      = PARAM->Target.ToInt();
    switch (prop_id) {
        case P_FOCUS:
            if (item->ptr) {
                if (PARAM->Value.ToInt()) {
                    ((QWidget *)item->ptr)->setFocus(Qt::MouseFocusReason);
                    if (item->type == CLASS_EDIT)
                        ((QLineEdit *)item->ptr)->selectAll();
                    else
                    if (item->type == CLASS_COMBOBOXTEXT) {
                        QLineEdit *edit = ((QComboBox *)item->ptr)->lineEdit();
                        if (edit)
                            edit->selectAll();
                    }
                }
            }
            property_set = true;
            break;

        case P_SHOWTOOLTIP:
            if (item->ptr) {
                if (PARAM->Value.ToInt()) {
                    ((QWidget *)item->ptr)->setFocus(Qt::MouseFocusReason);
                    QToolTip::showText(((QWidget *)item->ptr)->mapToGlobal(QPoint(0, 0)), ((QWidget *)item->ptr)->toolTip());
                }
            }
            property_set = true;
            break;

        case P_FORMMASK:
            if (item->ptr) {
                GTKControl *item2 = Client->Controls[PARAM->Value.ToInt()];
                if ((item2) && (item2->type == CLASS_IMAGE)) {
                    const QPixmap *pmap = ((QLabel *)item2->ptr)->pixmap();
                    ((QWidget *)item->ptr)->setMask(pmap->mask());
                } else
                    ((QWidget *)item->ptr)->clearMask();
            }
            property_set = true;
            break;

        case P_ADDTEXT:
            if ((item->ptr) && (item->type == CLASS_TEXTVIEW)) {
                //((QTextEdit *)item->ptr)->append(QString::fromUtf8(PARAM->Value.c_str()));
                ((QTextEdit *)item->ptr)->moveCursor(QTextCursor::End);
                ((QTextEdit *)item->ptr)->insertPlainText(QString::fromUtf8(PARAM->Value.c_str()));
                ((QTextEdit *)item->ptr)->moveCursor(QTextCursor::End);
                property_set = true;
            }
            break;

        case P_MENUOBJECT:
            {
                switch (item->type) {
                    case CLASS_IMAGEMENUITEM:
                    case CLASS_RADIOMENUITEM:
                    case CLASS_CHECKMENUITEM:
                    case CLASS_MENUITEM:
                        {
                            GTKControl *item2 = Client->Controls[PARAM->Value.ToInt()];
                            if ((item2) && (item2->type == CLASS_MENU))
                                ((QAction *)item->ptr2)->setMenu((QMenu *)item2->ptr);
                            else
                            if ((item2) && (!item2->ptr) && (item->ptr2))
                                ((QAction *)item->ptr2)->setMenu(((QAction *)item2->ptr2)->menu());
                            property_set = true;
                        }
                        break;

                    case CLASS_BUTTON:
                        {
                            GTKControl *item2 = Client->Controls[PARAM->Value.ToInt()];
                            if ((item2) && (item2->type == CLASS_MENU))
                                ((QPushButton *)item->ptr)->setMenu((QMenu *)item2->ptr);
                            else
                            if ((item2) && (!item2->ptr) && (item->ptr2))
                                ((QPushButton *)item->ptr)->setMenu(((QAction *)item2->ptr2)->menu());
                            property_set = true;
                        }
                        break;
                }
            }
            break;

        case P_CAPTION:
            switch (item->type) {
                case CLASS_LABEL:
                    ((QLabel *)item->ptr)->setText(QString::fromUtf8(PARAM->Value.c_str()));
                    ((QLabel *)item->ptr)->adjustSize();
                    if ((item->ref) && (((GTKControl *)item->ref)->type == CLASS_NOTEBOOK)) {
                        GTKControl  *parent = (GTKControl *)item->ref;
                        NotebookTab *nt     = (NotebookTab *)parent->pages->Item(item->index);
                        if (nt) {
                            int adjusted = parent->AdjustIndex(nt);
                            if (nt->visible)
                                ((QTabWidget *)((GTKControl *)item->ref)->ptr)->setTabText(adjusted, ((QLabel *)item->ptr)->text());
                            nt->title = PARAM->Value;
                        }
                    }
                    property_set = true;
                    break;

                case CLASS_TOOLBUTTON:
                case CLASS_TOGGLETOOLBUTTON:
                case CLASS_RADIOTOOLBUTTON:
                    ((QAction *)item->ptr2)->setText((QString::fromUtf8(ConvertCaption(PARAM->Value).c_str())));
                    if (((QToolButton *)item->ptr)->toolButtonStyle() != Qt::ToolButtonIconOnly)
                        ((QAction *)item->ptr2)->setIconText(QString::fromUtf8(ConvertCaption(PARAM->Value).c_str()));
                    else
                        ((QAction *)item->ptr2)->setIconText(QString::fromUtf8(" "));

                case CLASS_RADIOBUTTON:
                case CLASS_CHECKBUTTON:
                case CLASS_BUTTON:
                    if (item->flags) {
                        ((QAbstractButton *)item->ptr)->setIcon(QIcon(StockIcon(PARAM->Value.c_str())));
                        AnsiString temp;
                        StockName(PARAM->Value.c_str(), temp);
                        ((QAbstractButton *)item->ptr)->setText(QObject::tr(temp.c_str()));
                    } else
                        ((QAbstractButton *)item->ptr)->setText(QString::fromUtf8(ConvertCaption(PARAM->Value).c_str()));
                    property_set = true;
                    break;

                case CLASS_EDIT:
                    ((QLineEdit *)item->ptr)->setText(QString::fromUtf8(PARAM->Value.c_str()));
                    property_set = true;
                    break;

                case CLASS_TEXTVIEW:
                    ((QTextEdit *)item->ptr)->setPlainText(QString::fromUtf8(PARAM->Value.c_str()));
                    property_set = true;
                    break;

                case CLASS_FORM:
                    ((QWidget *)item->ptr)->setWindowTitle(QString::fromUtf8(PARAM->Value.c_str()));
                    property_set = true;
                    break;

                case CLASS_COMBOBOXTEXT:
                    ((QComboBox *)item->ptr)->setCurrentText(QString::fromUtf8(PARAM->Value.c_str()));
                    property_set = true;
                    break;

                case CLASS_EXPANDER:
                    {
                        QLabel *label = ((QWidget *)item->ptr)->findChild<QLabel *>("label");
                        if (label)
                            label->setText(QString::fromUtf8(PARAM->Value.c_str()));
                    }
                    property_set = true;
                    break;

                case CLASS_FRAME:
                    {
                        QString s = QString::fromUtf8(PARAM->Value.c_str());
                        if (((QGroupBox *)item->ptr)->property("markup").toBool())
                            s = s.remove(QRegExp("<[^>]*>"));
                        ((QGroupBox *)item->ptr)->setTitle(s);
                    }
                    property_set = true;
                    break;

                case CLASS_PROGRESSBAR:
                    ((QProgressBar *)item->ptr)->setTextVisible((bool)PARAM->Value.Length());
                    ((QProgressBar *)item->ptr)->setFormat(QString::fromUtf8(PARAM->Value.c_str()));
                    property_set = true;
                    break;

                case CLASS_STATUSBAR:
                    {
                        QLabel *label = ((QWidget *)item->ptr)->findChild<QLabel *>("label");
                        if (!label) {
                            label = new QLabel((QStatusBar *)item->ptr);
                            label->setObjectName(QString::fromUtf8("label"));
                            ((QStatusBar *)item->ptr)->addWidget(label);
                            label->show();
                        }
                        label->setText(QString::fromUtf8(PARAM->Value.c_str()));
                    }
                    property_set = true;
                    break;

                case CLASS_MENU:
                    ((QMenu *)item->ptr)->setTitle(QString::fromUtf8(ConvertCaption(PARAM->Value).c_str()));
                    property_set = true;
                    break;

                case 1001:
                case 1012:
#ifndef NO_WEBKIT
                    ((ManagedPage *)((QWebView *)item->ptr)->page())->ignoreNext = true;
                    ((QWebView *)item->ptr)->setHtml(QString::fromUtf8(PARAM->Value.c_str()), QUrl(QString("file:///static/")));
#endif
                    property_set = true;
                    break;
                case CLASS_HTMLSNAP:
#ifndef NO_WEBKIT
                    {
                        QString str = QString::fromUtf8("document.body.innerHTML='");
                        QString data =  QString::fromUtf8(PARAM->Value.c_str());
                        data = data.replace("\\", "\\\\");
                        data = data.replace("'", "\\'");
                        str += data;
                        str += QString::fromUtf8("';");
                        ((QWebView *)item->ptr)->page()->mainFrame()->evaluateJavaScript(str);
                    }
#endif
                    property_set = true;
                    break;

                case CLASS_IMAGEMENUITEM:
                case CLASS_STOCKMENUITEM:
                case CLASS_CHECKMENUITEM:
                case CLASS_MENUITEM:
                case CLASS_TEAROFFMENUITEM:
                case CLASS_RADIOMENUITEM:
                case CLASS_SEPARATORMENUITEM:
                    if (item->type == CLASS_STOCKMENUITEM) {
                        AnsiString temp;

                        StockName(PARAM->Value.c_str(), temp);
                        QString txt = QObject::tr(temp.c_str());
                        ((QAction *)item->ptr2)->setText(txt);
                        ((QAction *)item->ptr2)->setIcon(QIcon(StockIcon(PARAM->Value.c_str())));
                        //((QAction*)item->ptr2)->setIconText(txt);
                    } else
                        ((QAction *)item->ptr2)->setText(QString::fromUtf8(ConvertCaption(PARAM->Value).c_str()));
                    property_set = true;
                    break;
            }
            break;

        case P_DECORATIONS:
            {
                switch (item->type) {
                    case CLASS_FORM:
                        {
                            int             vflags     = PARAM->Value.ToInt();
                            Qt::WindowFlags flags_mask = Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint;
                            flags_mask &= ~(Qt::FramelessWindowHint | Qt::MSWindowsFixedSizeDialogHint);
                            Qt::WindowFlags flags = ((QWidget *)item->ptr)->windowFlags() & (~flags_mask);
                            if (vflags & (1 << 0)) {
                                flags |= Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint;
                                flags &= ~(Qt::FramelessWindowHint | Qt::MSWindowsFixedSizeDialogHint);
                            }
                            if (vflags & (1 << 1)) {
#ifdef _WIN32
                                flags |= Qt::MSWindowsFixedSizeDialogHint;
#else
                                flags |= Qt::FramelessWindowHint;
#endif
                            }
                            if (vflags & (1 << 2))
                                flags |= Qt::MSWindowsFixedSizeDialogHint;
                            if (vflags & (1 << 3))
                                flags |= Qt::WindowTitleHint;
                            if (vflags & (1 << 4))
                                flags |= Qt::WindowSystemMenuHint;
                            if (vflags & (1 << 5))
                                flags |= Qt::WindowMinimizeButtonHint;
                            if (vflags & (1 << 6))
                                flags |= Qt::WindowMaximizeButtonHint;
                            if (vflags & (1 << 7))
                                flags &= ~(Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint);
                            if (vflags & (1 << 8))
                                flags |= (Qt::CustomizeWindowHint | Qt::WindowCloseButtonHint);
                            ((QWidget *)item->ptr)->setWindowFlags(flags);
                        }
                        property_set = true;
                        break;
                }
            }
            break;

        case P_SKIPTASKBAR:
            if (item->type == CLASS_FORM) {
                Qt::WindowFlags flags = ((QWidget *)item->ptr)->windowFlags() & (~Qt::Dialog);

                if (PARAM->Value.ToInt())
                    flags |= Qt::Dialog;
                ((QWidget *)item->ptr)->setWindowFlags(flags);
                property_set = true;
            }
            break;

        case P_KEEPABOVE:
            if (item->type == CLASS_FORM) {
                Qt::WindowFlags flags = ((QWidget *)item->ptr)->windowFlags() & (~Qt::WindowStaysOnTopHint);

                if (PARAM->Value.ToInt())
                    flags |= Qt::WindowStaysOnTopHint;
                ((QWidget *)item->ptr)->setWindowFlags(flags);
                property_set = true;
            }
            break;

        case P_KEEPBELOW:
            if (item->type == CLASS_FORM) {
                Qt::WindowFlags flags = ((QWidget *)item->ptr)->windowFlags() & (~Qt::WindowStaysOnBottomHint);

                if (PARAM->Value.ToInt())
                    flags |= Qt::WindowStaysOnBottomHint;
                ((QWidget *)item->ptr)->setWindowFlags(flags);
                property_set = true;
            }
            break;

        case P_RESIZABLE:
            if (item->type == CLASS_FORM) {
                Qt::WindowFlags flags = ((QWidget *)item->ptr)->windowFlags() & (~Qt::MSWindowsFixedSizeDialogHint);

                if (PARAM->Value.ToInt())
                    flags |= Qt::MSWindowsFixedSizeDialogHint;
                ((QWidget *)item->ptr)->setWindowFlags(flags);
                property_set = true;
            }
            break;

        case P_WIDTH:
            if (item->ptr) {
                if (item->type == CLASS_FORM) {
                    ((QWidget *)item->ptr)->resize(PARAM->Value.ToInt(), ((QWidget *)item->ptr)->height());
                    item->use_stock = -1;
                } else
                if (item->type != CLASS_EVENTBOX)
                    ((QWidget *)item->ptr)->setFixedWidth(PARAM->Value.ToInt());
                property_set = true;
            }
            break;

        case P_HEIGHT:
            if (item->ptr) {
                if (item->type == CLASS_FORM) {
                    ((QWidget *)item->ptr)->resize(((QWidget *)item->ptr)->width(), PARAM->Value.ToInt());
                    item->use_stock = -1;
                } else
                if (item->type != CLASS_EVENTBOX)
                    ((QWidget *)item->ptr)->setFixedHeight(PARAM->Value.ToInt());
                property_set = true;
            }
            break;

        case P_LEFT:
            if (item->ptr) {
                if ((item->type != CLASS_FORM) && (item->parent)) {
                    GTKControl *parent = (GTKControl *)item->parent;
                    if ((parent->ptr) && (parent->ptr2)) {
                        ((QLayout *)parent->ptr2)->removeWidget((QWidget *)item->ptr);
                    }
                    if ((parent->type == CLASS_SCROLLEDWINDOW) || (parent->type == CLASS_HANDLEBOX))
                        parent->flags = 1;
                }
                ((QWidget *)item->ptr)->move(PARAM->Value.ToInt(), ((QWidget *)item->ptr)->y());
            }
            property_set = true;
            break;

        case P_TOP:
            if (item->ptr) {
                if ((item->type != CLASS_FORM) && (item->parent)) {
                    GTKControl *parent = (GTKControl *)item->parent;
                    if ((parent->ptr) && (parent->ptr2))
                        ((QLayout *)parent->ptr2)->removeWidget((QWidget *)item->ptr);
                    if ((parent->type == CLASS_SCROLLEDWINDOW) || (parent->type == CLASS_HANDLEBOX))
                        parent->flags = 1;
                }
                ((QWidget *)item->ptr)->move(((QWidget *)item->ptr)->x(), PARAM->Value.ToInt());
            }
            property_set = true;
            break;

        case P_MODAL:
            if (item->ptr) {
                if (PARAM->Value.ToInt())
                    ((QWidget *)item->ptr)->setWindowModality(Qt::ApplicationModal);
                else
                    ((QWidget *)item->ptr)->setWindowModality(Qt::NonModal);
            }
            property_set = true;
            break;

        case P_TITLEBAR:
            if (item->type == CLASS_FORM) {
                if (PARAM->Value.ToInt())
                    ((QWidget *)item->ptr)->setWindowFlags(Qt::Window);
                else
                    ((QWidget *)item->ptr)->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
                property_set = true;
            }
            break;

        case P_BACKGROUNDIMAGE:
            if (item->ptr) {
                GTKControl *item2 = Client->Controls[PARAM->Value.ToInt()];
                if ((!item2) || (item2->type != CLASS_IMAGE)) {
                    QPalette palette;
                    ((QWidget *)item->ptr)->setPalette(palette);
                    break;
                }

                const QPixmap *pmap = ((QLabel *)item2->ptr)->pixmap();
                if (pmap) {
                    QPalette palette;
                    palette.setBrush(((QWidget *)item->ptr)->backgroundRole(), QBrush(*pmap));
                    ((QWidget *)item->ptr)->setPalette(palette);
                }
                property_set = true;
            }
            break;

        case P_DEFAULTCONTROL:
            if (item->type == CLASS_FORM) {
                GTKControl *item2 = Client->Controls[PARAM->Value.ToInt()];
                if (item2->type == CLASS_BUTTON) {
                    ((QPushButton *)item2->ptr)->setDefault(true);
                    ((QPushButton *)item2->ptr)->setAutoDefault(true);
                }
                property_set = true;
            }
            break;

        case P_IMAGEPOSITION:
            {
                switch (item->type) {
                    case CLASS_BUTTON:
                    case CLASS_CHECKBUTTON:
                    case CLASS_RADIOBUTTON:
                        // not supported yet
                        property_set = true;
                        break;
                }
            }
            break;

        case P_PACKTYPE:
            item->packtype = (char)PARAM->Value.ToInt();
            property_set   = true;
            break;

        case P_PACKING:
            if (item->ptr) {
                if ((item->type != CLASS_TOOLBUTTON) && (item->type != CLASS_TOGGLETOOLBUTTON) && (item->type != CLASS_RADIOTOOLBUTTON) && (item->type != CLASS_TOOLSEPARATOR))
                    Packing((QWidget *)item->ptr, (GTKControl *)item->parent, PARAM->Value.ToInt(), item);
                property_set = true;
            }
            break;

        case P_VISIBLE:
            {
                if (item->type == CLASS_MENU)
                    break;
                val = PARAM->Value.ToInt();
                int old_visible = item->visible;
                item->visible = (val > 0);

                if ((!item->ptr) && (item->ptr2)) {
                    ((QAction *)item->ptr2)->setVisible(item->visible);
                    break;
                } else
                if ((item->type == CLASS_TOOLBUTTON) || (item->type == CLASS_TOGGLETOOLBUTTON) || (item->type == CLASS_RADIOTOOLBUTTON)) {
                    ((QAction *)item->ptr2)->setVisible(item->visible);
                    break;
                } else
                if (((item->parent) && (((GTKControl *)item->parent)->ptr)) || (item->type == CLASS_FORM)) {
                    //((QWidget *)item->ptr)->setUpdatesEnabled(false);
                    GTKControl *parent = (GTKControl *)item->parent;
                    if (item->visible) {
                        if ((item->type == CLASS_FORM) && (old_visible < -1)) {
                            if (old_visible == -2)
                                ((QWidget *)item->ptr)->showMinimized();
                            else
                            if (old_visible == -3)
                                ((QWidget *)item->ptr)->showMaximized();
                            else {
                                ((QWidget *)item->ptr)->show();
                                if (item->use_stock >= 0) {
                                    ((QWidget *)item->ptr)->adjustSize();
                                    item->use_stock = -3;
                                }
                            }
                        } else
                            ((QWidget *)item->ptr)->show();
                        if (item->type == CLASS_FORM) {
                            ((QWidget *)item->ptr)->raise();
                            ((QWidget *)item->ptr)->activateWindow();
                        } else
                        if (parent->type == CLASS_NOTEBOOK) {
                            NotebookTab *nt = (NotebookTab *)parent->pages->Item(item->index);
                            if ((nt) && (!nt->visible)) {
                                nt->visible = true;
                                int index = parent->AdjustIndex(nt);
                                ((QTabWidget *)parent->ptr)->insertTab(index, (QWidget *)item->ptr, QString::fromUtf8(nt->title.c_str()));
                                if (nt->image) {
                                    const QPixmap *pixmap = ((QLabel *)((GTKControl *)nt->image)->ptr)->pixmap();
                                    ((QTabWidget *)parent->ptr)->setTabIcon(index, *pixmap);
                                }
                                if (nt->button)
                                    ((QTabWidget *)parent->ptr)->tabBar()->setTabButton(index, QTabBar::RightSide, (QWidget *)((GTKControl *)nt->button)->ptr);
                                else
                                if (!nt->close_button) {
                                    QWidget *closebutton = ((QTabWidget *)parent->ptr)->tabBar()->tabButton(index, QTabBar::RightSide);
                                    if (closebutton)
                                        closebutton->hide();
                                }

                                if (nt->close_button) {
                                    QWidget *closebutton = ((QTabWidget *)parent->ptr)->tabBar()->tabButton(index, QTabBar::RightSide);
                                    if (closebutton)
                                        closebutton->show();
                                }
                            }
                        }
                        if (item->ref) {
                            if (item->type == CLASS_EVENTBOX) {
                                // close button
                                NotebookTab *nt = (NotebookTab *)((GTKControl *)item->ref)->pages->Item(item->index);
                                if (nt) {
                                    int     index        = ((GTKControl *)item->ref)->AdjustIndex(nt);
                                    QWidget *closebutton = ((QTabWidget *)((GTKControl *)item->ref)->ptr)->tabBar()->tabButton(index, QTabBar::RightSide);
                                    if (closebutton)
                                        closebutton->show();
                                }
                            } else {
                                NotebookTab *nt = (NotebookTab *)((GTKControl *)item->ref)->pages->Item(item->index);
                                if (nt) {
                                    int index = ((GTKControl *)item->ref)->AdjustIndex(nt);
                                    if (nt->button) {
                                        ((QTabWidget *)((GTKControl *)item->ref)->ptr)->tabBar()->setTabButton(index, QTabBar::RightSide, (QWidget *)((GTKControl *)nt->button)->ptr);
                                        ((QWidget *)((GTKControl *)nt->button)->ptr)->show();
                                    }
                                }
                            }
                        }
                    } else {
                        ((QWidget *)item->ptr)->hide();
                        if (PARAM->Owner->MainForm == item)
                            Done(PARAM->Owner, 0, true);
                        if ((parent) && (parent->type == CLASS_NOTEBOOK)) {
                            NotebookTab *nt = (NotebookTab *)parent->pages->Item(item->index);
                            if (nt) {
                                nt->visible = true;
                                int index = ((QTabWidget *)parent->ptr)->indexOf((QWidget *)item->ptr);//parent->AdjustIndex(item->ptr);//parent->AdjustIndex(nt);
                                nt->visible = false;
                                if (index >= 0)
                                    ((QTabWidget *)parent->ptr)->removeTab(index);
                            }
                        } else
                        if ((item->type == CLASS_EVENTBOX) && (item->ref)) {
                            // close button
                            NotebookTab *nt = (NotebookTab *)((GTKControl *)item->ref)->pages->Item(item->index);
                            if (nt) {
                                int     index        = ((GTKControl *)item->ref)->AdjustIndex(nt);
                                QWidget *closebutton = ((QTabWidget *)((GTKControl *)item->ref)->ptr)->tabBar()->tabButton(index, QTabBar::RightSide);
                                if (closebutton)
                                    closebutton->hide();
                            }
                        }
                    }
                }
            }
            property_set = true;
            break;

        case P_ATTLEFT:
            if (item->type == CLASS_TABLE) {
                val = PARAM->Value.ToInt();
                item->extraptr[0] = val;
                property_set      = true;
            }
            break;

        case P_ATTRIGHT:
            if (item->type == CLASS_TABLE) {
                val = PARAM->Value.ToInt();
                item->extraptr[1] = val;
                property_set      = true;
            }
            break;

        case P_ATTTOP:
            if (item->type == CLASS_TABLE) {
                val = PARAM->Value.ToInt();
                item->extraptr[2] = val;
                property_set      = true;
            }
            break;

        case P_ATTBOTTOM:
            if (item->type == CLASS_TABLE) {
                val = PARAM->Value.ToInt();
                item->extraptr[3] = val;
                property_set      = true;
            }
            break;

        case P_ATTHOPT:
            if (item->type == CLASS_TABLE) {
                val = PARAM->Value.ToInt();
                item->extraptr[4] = val;
                property_set      = true;
            }
            break;

        case P_ATTVOPT:
            if (item->type == CLASS_TABLE) {
                val = PARAM->Value.ToInt();
                item->extraptr[5] = val;
                property_set      = true;
            }
            break;

        case P_ATTHSPACING:
            if (item->type == CLASS_TABLE) {
                val = PARAM->Value.ToInt();
                item->extraptr[6] = val;
                property_set      = true;
            }
            break;

        case P_ATTVSPACING:
            if (item->type == CLASS_TABLE) {
                val = PARAM->Value.ToInt();
                item->extraptr[7] = val;
                property_set      = true;
            }
            break;

        case P_MARKUP:
            switch (item->type) {
                case CLASS_LABEL:
                    if (PARAM->Value.ToInt())
                        ((QLabel *)item->ptr)->setTextFormat(Qt::RichText);
                    else
                        ((QLabel *)item->ptr)->setTextFormat(Qt::PlainText);
                    property_set = true;
                    break;

                case CLASS_EXPANDER:
                    {
                        QLabel *label = ((QWidget *)item->ptr)->findChild<QLabel *>("label");
                        if (label) {
                            if (PARAM->Value.ToInt())
                                label->setTextFormat(Qt::RichText);
                            else
                                label->setTextFormat(Qt::PlainText);
                        }
                    }
                    property_set = true;
                    break;

                case CLASS_FRAME:
                    {
                        bool markup = (bool)PARAM->Value.ToInt();
                        if (markup) {
                            QString s = ((QGroupBox *)item->ptr)->title().remove(QRegExp("<[^>]*>"));
                            ((QGroupBox *)item->ptr)->setTitle(s);
                        }
                        ((QGroupBox *)item->ptr)->setProperty("markup", markup);
                    }
                    break;
            }
            break;

        case P_ACTIONWIDGETSTART:
            if (item->type == CLASS_NOTEBOOK) {
                int     id      = PARAM->Value.ToInt();
                QWidget *widget = NULL;
                int     show    = 0;
                if (id > 0) {
                    GTKControl *item2 = Client->Controls[id];
                    if ((item2) && (item2->ptr)) {
                        widget = (QWidget *)item2->ptr;
                        show   = item2->visible;
                        if (!item2->parent)
                            item2->parent = item;
                    }
                }
                QWidget *old = ((QTabWidget *)item->ptr)->cornerWidget(Qt::TopLeftCorner);
                if (old)
                    old->hide();
                ((QTabWidget *)item->ptr)->setCornerWidget(widget, Qt::TopLeftCorner);
                if ((widget) && (show > 0))
                    widget->show();
                property_set = true;
            }
            break;

        case P_ACTIONWIDGETEND:
            if (item->type == CLASS_NOTEBOOK) {
                int     id      = PARAM->Value.ToInt();
                QWidget *widget = NULL;
                int     show    = 0;
                if (id > 0) {
                    GTKControl *item2 = Client->Controls[id];
                    if ((item2) && (item2->ptr)) {
                        widget = (QWidget *)item2->ptr;
                        show   = item2->visible;
                        if (!item2->parent)
                            item2->parent = item;
                    }
                }
                QWidget *old = ((QTabWidget *)item->ptr)->cornerWidget(Qt::TopRightCorner);
                if (old)
                    old->hide();
                ((QTabWidget *)item->ptr)->setCornerWidget(widget, Qt::TopRightCorner);
                if ((widget) && (show > 0))
                    widget->show();
                property_set = true;
            }
            break;

        case P_ICON:
        case P_IMAGE:
            {
                GTKControl *item2 = Client->Controls[PARAM->Value.ToInt()];
                if ((item2) && (item2->type == CLASS_IMAGE)) {
                    const QPixmap *pmap = ((QLabel *)item2->ptr)->pixmap();
                    if (pmap) {
                        switch (item->type) {
                            case CLASS_TOOLBUTTON:
                            case CLASS_TOGGLETOOLBUTTON:
                            case CLASS_RADIOTOOLBUTTON:
                                ((QAction *)item->ptr2)->setIcon(QIcon(*pmap));
                                property_set = true;
                                break;

                            case CLASS_BUTTON:
                                ((QAbstractButton *)item->ptr)->setIcon(QIcon(*pmap));
                                ((QAbstractButton *)item->ptr)->setIconSize(pmap->rect().size());
                                //if ((!((QAbstractButton *)item->ptr)->minimumHeight()) && (!((QAbstractButton *)item->ptr)->minimumWidth()))
                                //    ((QAbstractButton *)item->ptr)->setMinimumSize(pmap->rect().size());
                                break;

                            case CLASS_FORM:
                                ((QWidget *)item->ptr)->setWindowIcon(QIcon(*pmap));
                                property_set = true;
                                break;

                            case CLASS_IMAGE:
                                ((QLabel *)item->ptr)->setPixmap(*pmap);
                                property_set = true;
                                break;

                            default:
                                // is action
                                if ((!item->ptr) && (item->ptr2)) {
                                    ((QAction *)item->ptr2)->setIcon(QIcon(*pmap));
                                    ((QAction *)item->ptr2)->setIconVisibleInMenu(true);
                                    property_set = true;
                                }
                        }
                    }
                }
            }
            break;

        case P_POPUPMENU:
            if (item->ptr) {
                GTKControl *item2 = Client->Controls[PARAM->Value.ToInt()];
                QMenu      *menu  = NULL;
                if ((item2) && (item2->type == CLASS_MENU))
                    menu = (QMenu *)item2->ptr;
                else
                if ((item2) && (!item2->ptr) && (item->ptr2))
                    menu = ((QAction *)item2->ptr2)->menu();

                if (menu) {
                    ClientEventHandler *events = (ClientEventHandler *)item->events;
                    if (!events) {
                        events       = new ClientEventHandler(item->ID, Client);
                        item->events = events;
                    }
                    events->popup = menu;
                    ((QWidget *)item->ptr)->setContextMenuPolicy(Qt::CustomContextMenu);
                    QObject::connect((QWidget *)item->ptr, SIGNAL(customContextMenuRequested(const QPoint &)), events, SLOT(on_customContextMenuRequested(const QPoint &)));
                }
                property_set = true;
            }
            break;

        case P_MENU:
            {
                switch (item->type) {
                    case CLASS_TOOLBUTTON:
                    case CLASS_TOGGLETOOLBUTTON:
                    case CLASS_RADIOTOOLBUTTON:
                        {
                            GTKControl *item2 = Client->Controls[PARAM->Value.ToInt()];
                            if ((item2) && (item2->type == CLASS_MENU))
                                ((QAction *)item->ptr2)->setMenu((QMenu *)item2->ptr);
                            else
                            if ((item2) && (!item2->ptr) && (item->ptr2))
                                ((QAction *)item->ptr2)->setMenu(((QAction *)item2->ptr2)->menu());
                            else
                                ((QAction *)item->ptr2)->setMenu(NULL);
                        }
                        property_set = true;
                        break;

                    case CLASS_BUTTON:
                        GTKControl *item2 = Client->Controls[PARAM->Value.ToInt()];
                        if ((item2) && (item2->type == CLASS_MENU))
                            ((QPushButton *)item->ptr)->setMenu((QMenu *)item2->ptr);
                        else
                        if ((item2) && (!item2->ptr) && (item->ptr2))
                            ((QPushButton *)item->ptr)->setMenu(((QAction *)item2->ptr2)->menu());
                        property_set = true;
                        break;
                }
            }
            break;

        case P_ISIMPORTANT:
            switch (item->type) {
                case CLASS_TOOLBUTTON:
                case CLASS_TOGGLETOOLBUTTON:
                case CLASS_RADIOTOOLBUTTON:
                case CLASS_MENUTOOLBUTTON:
                    if (PARAM->Value.ToInt()) {
                        ((QToolButton *)item->ptr)->setToolButtonStyle(Qt::ToolButtonFollowStyle);
                        ((QAction *)item->ptr2)->setIconText(((QToolButton *)item->ptr)->text());
                    } else {
                        ((QToolButton *)item->ptr)->setToolButtonStyle(Qt::ToolButtonIconOnly);
                        //((QAction *)item->ptr2)->setIconText(QString::fromUtf8(""));
                    }
                    property_set = true;
                    break;
            }
            break;

        case P_RELIEFSTYLE:
            if (item->type == CLASS_BUTTON) {
                ((QPushButton *)item->ptr)->setFlat((bool)PARAM->Value.ToInt());
                property_set = true;
            }
            break;

        case P_ORIENTATION:
            switch (item->type) {
                case CLASS_TOOLBAR:
                    if (PARAM->Value.ToInt())
                        ((QToolBar *)item->ptr)->setOrientation(Qt::Vertical);
                    else
                        ((QToolBar *)item->ptr)->setOrientation(Qt::Horizontal);
                    property_set = true;
                    break;

                case CLASS_PROGRESSBAR:
                    if (PARAM->Value.ToInt())
                        ((QProgressBar *)item->ptr)->setOrientation(Qt::Vertical);
                    else
                        ((QProgressBar *)item->ptr)->setOrientation(Qt::Horizontal);
                    property_set = true;
                    break;

                case CLASS_ICONVIEW:
                    ((TreeData *)item->ptr3)->orientation = PARAM->Value.ToInt();
                    property_set = true;
                    break;

                case CLASS_PRINTER:
                    property_set = true;
                    break;
            }
            break;

        case P_SHOWARROW:
            if (item->type == CLASS_TOOLBAR) {
                ((QToolBar *)item->ptr)->setMovable(PARAM->Value.ToInt());
                property_set = true;
            }
            break;

        case P_TABPOS:
            if (item->type == CLASS_NOTEBOOK) {
                switch (PARAM->Value.ToInt()) {
                    case 0:
                        ((QTabWidget *)item->ptr)->setTabPosition(QTabWidget::West);
                        break;

                    case 1:
                        ((QTabWidget *)item->ptr)->setTabPosition(QTabWidget::East);
                        break;

                    case 3:
                        ((QTabWidget *)item->ptr)->setTabPosition(QTabWidget::South);
                        break;

                    case 2:
                    default:
                        ((QTabWidget *)item->ptr)->setTabPosition(QTabWidget::North);
                }
                property_set = true;
            }
            break;

        case P_SHOWTABS:
            if (item->type == CLASS_NOTEBOOK) {
                if (PARAM->Value.ToInt()) {
                    ((QTabWidget *)item->ptr)->tabBar()->show();
                } else {
                    ((QTabWidget *)item->ptr)->tabBar()->hide();
                }
                property_set = true;
            }
            break;

        case P_TABBORDER:
            if (item->type == CLASS_NOTEBOOK) {
                if (PARAM->Value.ToInt())
                    ((QTabWidget *)item->ptr)->setDocumentMode(true);
                else
                    ((QTabWidget *)item->ptr)->setDocumentMode(false);
                property_set = true;
            }
            break;

        case P_SHOWBORDER:
            if (item->type == CLASS_NOTEBOOK) {
                if (PARAM->Value.ToInt())
                    ((QTabWidget *)item->ptr)->setStyleSheet("QTabWidget { border: 1px; }");
                else
                    ((QTabWidget *)item->ptr)->setStyleSheet("QTabWidget { border: none; }");
                property_set = true;
            }
            break;

        case P_SCROLLABLE:
            if (item->type == CLASS_NOTEBOOK) {
                ((QTabWidget *)item->ptr)->tabBar()->setUsesScrollButtons(PARAM->Value.ToInt());
                property_set = true;
            }
            break;

        case P_MAXIMIZED:
            if (item->type == CLASS_FORM) {
                if ((PARAM->Value.ToInt()) && (item->visible > 0))
                    ((QWidget *)item->ptr)->showMaximized();
                else
                    item->visible = -3;
                property_set = true;
            } else
            if (item->type == CLASS_SCROLLEDWINDOW) {
                ((QScrollArea *)item->ptr)->adjustSize();
                QSize size;
                switch (PARAM->Value.ToInt()) {
                    case 0:
                        size.setHeight(((QWidget *)item->ptr)->height());
                        size.setWidth(((QScrollArea *)item->ptr)->maximumViewportSize().width());
                        ((QWidget *)item->ptr)->resize(size);
                        break;

                    case 1:
                        size.setWidth(((QWidget *)item->ptr)->width());
                        size.setHeight(((QScrollArea *)item->ptr)->maximumViewportSize().height());
                        ((QWidget *)item->ptr)->resize(size);
                        break;

                    case 3:
                        ((QWidget *)item->ptr)->resize(((QScrollArea *)item->ptr)->maximumViewportSize());
                        break;
                }
            } else
            if (item->ptr) {
                if (PARAM->Value.ToInt()) {
                    ((QWidget *)item->ptr)->adjustSize();
                    ((QWidget *)item->ptr)->resize(((QWidget *)item->ptr)->minimumSizeHint());
                }
                property_set = true;
            }
            break;

        case P_MINIMIZED:
            if (item->type == CLASS_FORM) {
                if ((PARAM->Value.ToInt()) && (item->visible > 0))
                    ((QWidget *)item->ptr)->showMinimized();
                else
                    item->visible = -2;
                property_set = true;
            }
            break;

        case P_EXPANDED:
            if (item->type == CLASS_EXPANDER) {
                //QLabel* label = ((QWidget *)item->ptr)->findChild<QLabel*>("label");
                BaseWidget *image = (BaseWidget *)((QWidget *)item->ptr)->findChild<QLabel *>("image");
                if (image) {
                    image->Toggle((bool)PARAM->Value.ToInt());
                }
                property_set = true;
            }
            break;

        case P_PAGE:
            if (item->type == CLASS_NOTEBOOK) {
                int         index = PARAM->Value.ToInt();
                NotebookTab *nt   = (NotebookTab *)item->pages->Item(index);
                if ((nt) && (nt->visible))
                    ((QTabWidget *)item->ptr)->setCurrentIndex(item->AdjustIndex(nt));
                property_set = true;
            }
            break;

        case P_ANGLE:
            {
                switch (item->type) {
                    case CLASS_LABEL:
                        ((VerticalLabel *)item->ptr)->angle = PARAM->Value.ToInt();
                        ((VerticalLabel *)item->ptr)->update();
                        property_set = true;
                        break;
                }
            }
            break;

        case P_TOOLBARSTYLE:
            if (item->type == CLASS_TOOLBAR) {
                switch (PARAM->Value.ToInt()) {
                    case 0:
                        ((QToolBar *)item->ptr)->setToolButtonStyle(Qt::ToolButtonIconOnly);
                        break;

                    case 1:
                        ((QToolBar *)item->ptr)->setToolButtonStyle(Qt::ToolButtonTextOnly);
                        break;

                    case 2:
                        ((QToolBar *)item->ptr)->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
                        break;

                    case 3:
                        ((QToolBar *)item->ptr)->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
                        break;

                    default:
                        ((QToolBar *)item->ptr)->setToolButtonStyle(Qt::ToolButtonFollowStyle);
                        break;
                }
                property_set = true;
            }
            break;

        case P_XALIGN:
            switch (item->type) {
                case CLASS_LABEL:
                case CLASS_IMAGE:
                    {
                        double        d     = PARAM->Value.ToFloat();
                        Qt::Alignment align = ((QLabel *)item->ptr)->alignment();
                        align &= 0xFFF0;
                        if (d == -1)
                            ((QLabel *)item->ptr)->setAlignment(align | Qt::AlignJustify);
                        else
                        if (d <= 0.1)
                            ((QLabel *)item->ptr)->setAlignment(align | Qt::AlignLeft);
                        else
                        if (d >= 0.9)
                            ((QLabel *)item->ptr)->setAlignment(align | Qt::AlignRight);
                        else
                            ((QLabel *)item->ptr)->setAlignment(align | Qt::AlignHCenter);

                        if (d > 0.1) {
                            GTKControl *parent = (GTKControl *)item->parent;
                            if ((d >= 0) && (parent) && ((parent->type == CLASS_TABLE) || (parent->type == CLASS_VBOX) || (parent->type == CLASS_HBOX))) {
                                QLayoutItem *litem = ((QLayout *)parent->ptr2)->itemAt(((QLayout *)parent->ptr2)->indexOf((QWidget *)item->ptr));
                                if (litem) {
                                    align  = litem->alignment();
                                    align &= 0xFFF0;
                                    if (d == -1)
                                        litem->setAlignment(align | Qt::AlignJustify);
                                    else
                                    if (d <= 0.1)
                                        litem->setAlignment(align | Qt::AlignLeft);
                                    else
                                    if (d >= 0.9)
                                        litem->setAlignment(align | Qt::AlignRight);
                                    else
                                        litem->setAlignment(align | Qt::AlignHCenter);
                                }
                            }
                        }
                    }
                    property_set = true;
                    break;

                case CLASS_EDIT:
                    {
                        double        d     = PARAM->Value.ToFloat();
                        Qt::Alignment align = ((QLineEdit *)item->ptr)->alignment();
                        align &= 0xFFF0;
                        if (d == -1)
                            ((QLineEdit *)item->ptr)->setAlignment(align | Qt::AlignJustify);
                        else
                        if (d <= 0.1)
                            ((QLineEdit *)item->ptr)->setAlignment(align | Qt::AlignLeft);
                        else
                        if (d >= 0.9)
                            ((QLineEdit *)item->ptr)->setAlignment(align | Qt::AlignRight);
                        else
                            ((QLineEdit *)item->ptr)->setAlignment(align | Qt::AlignHCenter);
                    }
                    property_set = true;
                    break;

                case CLASS_COMBOBOXTEXT:
                    {
                        double    d     = PARAM->Value.ToFloat();
                        QLineEdit *edit = ((QComboBox *)item->ptr)->lineEdit();
                        if (edit) {
                            Qt::Alignment align = edit->alignment();
                            align &= 0xFFF0;
                            if (d == -1)
                                edit->setAlignment(align | Qt::AlignJustify);
                            else
                            if (d <= 0.1)
                                edit->setAlignment(align | Qt::AlignLeft);
                            else
                            if (d >= 0.9)
                                edit->setAlignment(align | Qt::AlignRight);
                            else
                                edit->setAlignment(align | Qt::AlignHCenter);
                        }
                    }
                    property_set = true;
                    break;

                case CLASS_ASPECTFRAME:
                case CLASS_ALIGNMENT:
                    if ((item->ptr) && (item->ptr2)) {
                        double        d     = PARAM->Value.ToFloat();
                        Qt::Alignment align = ((QLayout *)item->ptr2)->alignment();
                        align &= 0xFFF0;
                        if (d == -1)
                            ((QLayout *)item->ptr2)->setAlignment(align | Qt::AlignJustify);
                        else
                        if (d <= 0.1)
                            ((QLayout *)item->ptr2)->setAlignment(align | Qt::AlignLeft);
                        else
                        if (d >= 0.9)
                            ((QLayout *)item->ptr2)->setAlignment(align | Qt::AlignRight);
                        else
                            ((QLayout *)item->ptr2)->setAlignment(align | Qt::AlignHCenter);
                    }
                    property_set = true;
                    break;
            }
            break;

        case P_YALIGN:
            switch (item->type) {
                case CLASS_LABEL:
                case CLASS_IMAGE:
                    {
                        double        d     = PARAM->Value.ToFloat();
                        Qt::Alignment align = ((QLabel *)item->ptr)->alignment();
                        align &= 0xFF0F;
                        if (d <= 0.1)
                            ((QLabel *)item->ptr)->setAlignment(align | Qt::AlignTop);
                        else
                        if (d >= 0.9)
                            ((QLabel *)item->ptr)->setAlignment(align | Qt::AlignBottom);
                        else
                            ((QLabel *)item->ptr)->setAlignment(align | Qt::AlignVCenter);

                        /*GTKControl *parent = (GTKControl *)item->parent;
                           if ((parent) && ((parent->type==CLASS_TABLE) || (parent->type==CLASS_VBOX) || (parent->type==CLASS_HBOX))) {
                            if (d<=0.1)
                                ((QLayout *)parent->ptr2)->setAlignment((QWidget *)item->ptr, align | Qt::AlignTop);
                            else
                            if (d>=0.9)
                                ((QLayout *)parent->ptr2)->setAlignment((QWidget *)item->ptr, align | Qt::AlignBottom);
                            else
                                ((QLayout *)parent->ptr2)->setAlignment((QWidget *)item->ptr, align | Qt::AlignVCenter);
                           }*/
                    }
                    property_set = true;
                    break;

                case CLASS_EDIT:
                    {
                        double        d     = PARAM->Value.ToFloat();
                        Qt::Alignment align = ((QLineEdit *)item->ptr)->alignment();
                        align &= 0xFF0F;
                        if (d <= 0.1)
                            ((QLineEdit *)item->ptr)->setAlignment(align | Qt::AlignTop);
                        else
                        if (d >= 0.9)
                            ((QLineEdit *)item->ptr)->setAlignment(align | Qt::AlignBottom);
                        else
                            ((QLineEdit *)item->ptr)->setAlignment(align | Qt::AlignVCenter);
                    }
                    property_set = true;
                    break;

                case CLASS_COMBOBOXTEXT:
                    {
                        double    d     = PARAM->Value.ToFloat();
                        QLineEdit *edit = ((QComboBox *)item->ptr)->lineEdit();
                        if (edit) {
                            Qt::Alignment align = edit->alignment();
                            align &= 0xFF0F;
                            if (d <= 0.1)
                                edit->setAlignment(align | Qt::AlignTop);
                            else
                            if (d >= 0.9)
                                edit->setAlignment(align | Qt::AlignBottom);
                            else
                                edit->setAlignment(align | Qt::AlignVCenter);
                        }
                    }
                    property_set = true;
                    break;

                case CLASS_ASPECTFRAME:
                case CLASS_ALIGNMENT:
                    if ((item->ptr) && (item->ptr2)) {
                        double        d     = PARAM->Value.ToFloat();
                        Qt::Alignment align = ((QLayout *)item->ptr2)->alignment();
                        align &= 0xFF0F;
                        if (d <= 0.1)
                            ((QLayout *)item->ptr2)->setAlignment(align | Qt::AlignTop);
                        else
                        if (d >= 0.9)
                            ((QLayout *)item->ptr2)->setAlignment(align | Qt::AlignBottom);
                        else
                            ((QLayout *)item->ptr2)->setAlignment(align | Qt::AlignVCenter);
                    }
                    property_set = true;
                    break;
            }
            break;

        case P_USESTOCK:
            switch (item->type) {
                case CLASS_BUTTON:
                    {
                        item->flags = PARAM->Value.ToInt();
                        if (item->flags) {
                            char *iconname = ((QAbstractButton *)item->ptr)->text().toUtf8().data();
                            if ((iconname) && (iconname[0])) {
                                AnsiString temp(iconname);
                                ((QAbstractButton *)item->ptr)->setIcon(QIcon(StockIcon(iconname)));

                                AnsiString temp2;
                                StockName(temp, temp2);
                                ((QAbstractButton *)item->ptr)->setText(QObject::tr(temp2.c_str()));
                                //((QAbstractButton *)item->ptr)->setText(QString());
                            }
                        } else
                            ((QAbstractButton *)item->ptr)->setIcon(QIcon());
                    }
                    property_set = true;
                    break;

                case CLASS_TOOLBUTTON:
                case CLASS_TOGGLETOOLBUTTON:
                case CLASS_RADIOTOOLBUTTON:
                    {
                        QIcon icon(StockIcon(PARAM->Value.c_str()));
                        ((QAbstractButton *)item->ptr)->setIcon(icon);
                        if (item->ptr2)
                            ((QAction *)item->ptr2)->setIcon(icon);
                    }
                    property_set = true;
                    break;
            }
            break;

        case P_ENABLED:
            switch (item->type) {
                case CLASS_TOOLBUTTON:
                case CLASS_TOGGLETOOLBUTTON:
                case CLASS_RADIOTOOLBUTTON:
                case CLASS_IMAGEMENUITEM:
                case CLASS_STOCKMENUITEM:
                case CLASS_CHECKMENUITEM:
                case CLASS_MENUITEM:
                case CLASS_TEAROFFMENUITEM:
                case CLASS_RADIOMENUITEM:
                case CLASS_SEPARATORMENUITEM:
                    if (item->ptr2)
                        ((QAction *)item->ptr2)->setEnabled((bool)PARAM->Value.ToInt());

                default:
                    if (item->ptr) {
                        if (item->type == CLASS_FORM) {
                            bool        enabled             = (bool)PARAM->Value.ToInt();
                            QObjectList children            = ((QWidget *)item->ptr)->children();
                            QObjectList::const_iterator it  = children.begin();
                            QObjectList::const_iterator eIt = children.end();
                            while (it != eIt) {
                                QWidget *child = (QWidget *)*it;
                                if (child) {
                                    GTKControl *c = Client->ControlsReversed[child];
                                    if ((c) && (c->type != CLASS_FORM) && (c->ptr)) {
                                        ((QWidget *)c->ptr)->setEnabled(enabled);
                                    }
                                }
                                ++it;
                            }
                        } else
                            ((QWidget *)item->ptr)->setEnabled((bool)PARAM->Value.ToInt());
                    }
            }
            property_set = true;
            break;

        case P_ICONSIZE:
            if (item->type == CLASS_TOOLBAR) {
                int isize = PARAM->Value.ToInt();
                switch (isize) {
                    case 0:
                    case 1:
                        isize = 16;
                        break;

                    case 2:
                        isize = 24;
                        break;

                    case 3:
                        isize = 32;
                        break;

                    default:
                        if (isize <= 5)
                            isize = 32;
                }
                ((QToolBar *)item->ptr)->setIconSize(QSize(isize, isize));
                property_set = true;
            }
            break;

        case P_FIXEDHEIGHTMODE:
            {
                switch (item->type) {
                    case CLASS_TREEVIEW:
                        ((QTreeWidget *)item->ptr)->setUniformRowHeights((bool)PARAM->Value.ToInt());
                        property_set = true;
                        break;
                }
            }
            break;

        case P_HEADERSCLICKABLE:
            {
                switch (item->type) {
                    case CLASS_TREEVIEW:
                        ((QTreeWidget *)item->ptr)->header()->setSectionsClickable((bool)PARAM->Value.ToInt());
                        property_set = true;
                        break;
                }
            }
            break;

        case P_HOVEREXPAND:
            {
                switch (item->type) {
                    case CLASS_TREEVIEW:
                        // not supported
                        property_set = true;
                        break;
                }
            }
            break;

        case P_HOVERSELECTION:
            {
                switch (item->type) {
                    case CLASS_TREEVIEW:
                        {
                            ClientEventHandler *events = (ClientEventHandler *)item->events;
                            if (events) {
                                QObject::disconnect((QTreeWidget *)item->ptr, SIGNAL(itemEntered(QTreeWidgetItem *, int)), events, SLOT(on_itemEntered(QTreeWidgetItem *, int)));
                                ((QTreeWidget *)item->ptr)->setMouseTracking(false);
                            }
                            if (PARAM->Value.ToInt()) {
                                if (!events) {
                                    events       = new ClientEventHandler(item->ID, Client);
                                    item->events = events;
                                }
                                ((QTreeWidget *)item->ptr)->setMouseTracking(true);
                                QObject::connect((QTreeWidget *)item->ptr, SIGNAL(itemEntered(QTreeWidgetItem *, int)), events, SLOT(on_itemEntered(QTreeWidgetItem *, int)));
                            }
                        }
                        property_set = true;
                        break;
                }
            }
            break;

        case P_MODEL:
            switch (item->type) {
                case CLASS_TREEVIEW:
                    ((QTreeWidget *)item->ptr)->setRootIsDecorated((bool)PARAM->Value.ToInt());
                    property_set = true;
                    break;

                case CLASS_PROPERTIESBOX:
                    ((QtTreePropertyBrowser *)item->ptr)->setProperty("returnindex", (bool)PARAM->Value.ToInt());
                    property_set = true;
                    break;
            }
            break;

        case P_RULESHINT:
            switch (item->type) {
                case CLASS_TREEVIEW:
                    ((QTreeWidget *)item->ptr)->setAlternatingRowColors((bool)PARAM->Value.ToInt());
                    property_set = true;
                    break;
            }
            break;

        case P_HEADERSVISIBLE:
            switch (item->type) {
                case CLASS_TREEVIEW:
                    ((QTreeWidget *)item->ptr)->setHeaderHidden(!PARAM->Value.ToInt());
                    ((EventAwareWidget<QTreeWidget> *)item->ptr)->UpdateHeight();
                    property_set = true;
                    break;
            }
            break;

        case P_GRIDLINES:
            switch (item->type) {
                case CLASS_TREEVIEW:
                    {
                        int lines = PARAM->Value.ToInt();
                        switch (lines) {
                            case 1:
                                ((QTreeWidget *)item->ptr)->setStyleSheet("QTreeWidget::item {border-bottom: 1px solid black;}");
                                break;

                            case 2:
                                ((QTreeWidget *)item->ptr)->setStyleSheet("QTreeWidget::item {border-right: 1px solid black;}");
                                break;

                            case 3:
                                ((QTreeWidget *)item->ptr)->setStyleSheet("QTreeWidget::item {border-right: 1px solid black; border-bottom: 1px solid black;}");
                                break;

                            case 0:
                            default:
                                ((QTreeWidget *)item->ptr)->setStyleSheet("border: none;");
                        }
                    }
                    property_set = true;
                    break;
            }
            break;

        case P_POSITION:
            {
                switch (item->type) {
                    case CLASS_VPANED:
                    case CLASS_HPANED:
                        ((EventAwareWidget<QSplitter> *)item->ptr)->setPosition(PARAM->Value.ToInt());
                        property_set = true;
                        break;

                    case CLASS_FORM:
                        switch (PARAM->Value.ToInt()) {
                            case 2:
                                ((QWidget *)item->ptr)->move(((QWidget *)item->ptr)->mapFromGlobal(QCursor::pos()));
                                break;

                            case 1:
                            case 3:
                            case 4:
                                // center (this is default)

                                /*
                                   if (item->parent) {
                                    QSize size = ((QWidget *)item->ptr)->sizeHint();
                                    //QDesktopWidget* desktop = QApplication::desktop();
                                    //QRect rect = desktop->screenGeometry();
                                    GTKControl *parent = (GTKControl *)item->parent;
                                    QWidget *pwidget = (QWidget *)parent->ptr;

                                    QRect rect = ((QWidget *)parent->ptr)->window()->frameGeometry();

                                    QRect itemrect = ((QWidget *)item->ptr)->rect();
                                    QPoint position = rect.center() - itemrect.center();
                                    ((QWidget *)item->ptr)->move(position);
                                   }
                                   }*/
                                break;
                        }
                        property_set = true;
                        break;
                }
            }
            break;

        case P_IMAGECOLUMN:
            {
                switch (item->type) {
                    case CLASS_TREEVIEW:
                    case CLASS_ICONVIEW:
                    case CLASS_COMBOBOX:
                    case CLASS_COMBOBOXTEXT:
                        ((TreeData *)item->ptr3)->icon = PARAM->Value.ToInt();
                        property_set = true;
                        break;
                }
            }
            break;

        case P_TEXTCOLUMN:
            {
                switch (item->type) {
                    case CLASS_TREEVIEW:
                    case CLASS_ICONVIEW:
                    case CLASS_COMBOBOX:
                    case CLASS_COMBOBOXTEXT:
                        ((TreeData *)item->ptr3)->text = PARAM->Value.ToInt();
                        property_set = true;
                        break;
                }
            }
            break;

        case P_MARKUPCOLUMN:
            {
                switch (item->type) {
                    case CLASS_TREEVIEW:
                    case CLASS_ICONVIEW:
                    case CLASS_COMBOBOX:
                    case CLASS_COMBOBOXTEXT:
                        ((TreeData *)item->ptr3)->markup = PARAM->Value.ToInt();
                        property_set = true;
                        break;
                }
            }
            break;

        case P_TOOLTIPCOLUMN:
            {
                switch (item->type) {
                    case CLASS_TREEVIEW:
                    case CLASS_ICONVIEW:
                    case CLASS_COMBOBOX:
                    case CLASS_COMBOBOXTEXT:
                        ((TreeData *)item->ptr3)->hint = PARAM->Value.ToInt();
                        property_set = true;
                        break;
                }
            }
            break;

        case P_ITEMWIDTH:
            {
                switch (item->type) {
                    case CLASS_ICONVIEW:
                        ((TreeData *)item->ptr3)->extra = PARAM->Value.ToInt();
                        property_set = true;
                        break;
                }
            }
            break;

        case P_LAYOUT:
            // not supported yet
            break;

        case P_STYLE:
            if (item->ptr)
                ((QWidget *)item->ptr)->setStyleSheet(ConvertStyle(QString::fromUtf8(PARAM->Value.c_str())));
            else
            if ((item->ptr2) && (item->parent)) {
                GTKControl *parent = (GTKControl *)item->parent;
                if ((parent) && (parent->type == CLASS_TOOLBAR)) {
                    QWidget *widget = ((QToolBar *)parent->ptr)->widgetForAction((QAction *)item->ptr2);
                    if (widget)
                        widget->setStyleSheet(ConvertStyle(QString::fromUtf8(PARAM->Value.c_str())));
                }
            }
            property_set = true;
            break;

        case P_NORMAL_BG_COLOR:
            if (item->ptr) {
                ((QWidget *)item->ptr)->setStyleSheet(((QWidget *)item->ptr)->styleSheet() + QString("QWidget { background: ") + doColorString((unsigned int)PARAM->Value.ToFloat()) + QString(";}"));
                if (item->type == 1002) {
                    QPalette Pal = ((QWidget *)item->ptr)->palette();
                    Pal.setColor(QPalette::Background, Qt::black);
                    ((QWidget *)item->ptr)->setAutoFillBackground(true);
                    ((QWidget *)item->ptr)->setPalette(Pal);
                }
                property_set = true;
            }
            break;

        case P_INACTIVE_BG_COLOR:
            if (item->ptr) {
                ((QWidget *)item->ptr)->setStyleSheet(((QWidget *)item->ptr)->styleSheet() + QString("QWidget:inactive{ background: ") + doColorString((unsigned int)PARAM->Value.ToFloat()) + QString(";}"));
                property_set = true;
            }
            break;

        case P_SELECTED_BG_COLOR:
            if (item->ptr) {
                ((QWidget *)item->ptr)->setStyleSheet(((QWidget *)item->ptr)->styleSheet() + QString("QWidget:hover { background: ") + doColorString((unsigned int)PARAM->Value.ToFloat()) + QString(";}"));
                property_set = true;
            }
            break;

        case P_NORMAL_FG_COLOR:
            if (item->ptr) {
                ((QWidget *)item->ptr)->setStyleSheet(((QWidget *)item->ptr)->styleSheet() + QString("QWidget { color: ") + doColorString((unsigned int)PARAM->Value.ToFloat()) + QString(";}"));
                property_set = true;
            }
            break;

        case P_INACTIVE_FG_COLOR:
            if (item->ptr) {
                ((QWidget *)item->ptr)->setStyleSheet(((QWidget *)item->ptr)->styleSheet() + QString("QWidget:inactive { color: ") + doColorString((unsigned int)PARAM->Value.ToFloat()) + QString(";}"));
                property_set = true;
            }
            break;

        case P_SELECTED_FG_COLOR:
            if (item->ptr) {
                ((QWidget *)item->ptr)->setStyleSheet(((QWidget *)item->ptr)->styleSheet() + QString("QWidget:hover { color: ") + doColorString((unsigned int)PARAM->Value.ToFloat()) + QString(";}"));
                property_set = true;
            }
            break;

        case P_TOPPADDING:
            if (item->ptr) {
                int left, top, right, bottom;
                ((QLayout *)item->ptr2)->getContentsMargins(&left, &top, &right, &bottom);
                ((QLayout *)item->ptr2)->setContentsMargins(left, PARAM->Value.ToInt(), right, bottom);
                property_set = true;
            }
            break;

        case P_LEFTPADDING:
            if (item->ptr) {
                int left, top, right, bottom;
                ((QLayout *)item->ptr2)->getContentsMargins(&left, &top, &right, &bottom);
                ((QLayout *)item->ptr2)->setContentsMargins(PARAM->Value.ToInt(), top, right, bottom);
                property_set = true;
            }
            break;

        case P_BOTTOMPADDING:
            if (item->ptr) {
                int left, top, right, bottom;
                ((QLayout *)item->ptr2)->getContentsMargins(&left, &top, &right, &bottom);
                ((QLayout *)item->ptr2)->setContentsMargins(left, top, right, PARAM->Value.ToInt());
                property_set = true;
            }
            break;

        case P_RIGHTPADDING:
            if (item->ptr) {
                int left, top, right, bottom;
                ((QLayout *)item->ptr2)->getContentsMargins(&left, &top, &right, &bottom);
                ((QLayout *)item->ptr2)->setContentsMargins(left, top, PARAM->Value.ToInt(), bottom);
                property_set = true;
            }
            break;

        case P_SPACING:
            switch (item->type) {
                case CLASS_VBOX:
                case CLASS_HBOX:
                case CLASS_VBUTTONBOX:
                case CLASS_HBUTTONBOX:
                case CLASS_STATUSBAR:
                    if (item->ptr2)
                        ((QBoxLayout *)item->ptr2)->setSpacing(PARAM->Value.ToInt());
                    property_set = true;
                    break;

                case CLASS_ICONVIEW:
                    ((QListWidget *)item->ptr)->setSpacing(PARAM->Value.ToInt());
                    property_set = true;
                    break;
            }
            break;

        case P_HOMOGENEOUS:
            if ((item->ptr) && (item->ptr2)) {
                item->flags  = PARAM->Value.ToInt();
                property_set = true;
            }
            break;

        case P_XPADDING:
            if ((item->ptr) && (item->ptr2)) {
                switch (item->type) {
                    case CLASS_HBUTTONBOX:
                    case CLASS_VBUTTONBOX:
                    case CLASS_HBOX:
                    case CLASS_VBOX:
                        {
                            int left, top, right, bottom;
                            int padding = PARAM->Value.ToInt();
                            ((QBoxLayout *)item->ptr2)->getContentsMargins(&left, &top, &right, &bottom);
                            ((QBoxLayout *)item->ptr2)->setContentsMargins(padding, top, padding, bottom);
                        }
                        property_set = true;
                        break;

                    case CLASS_LABEL:
                    case CLASS_IMAGE:
                        ((QLabel *)item->ptr)->setMargin(PARAM->Value.ToInt());
                        property_set = true;
                        break;
                }
            }
            break;

        case P_YPADDING:
            if ((item->ptr) && (item->ptr2)) {
                switch (item->type) {
                    case CLASS_HBUTTONBOX:
                    case CLASS_VBUTTONBOX:
                    case CLASS_HBOX:
                    case CLASS_VBOX:
                        {
                            int left, top, right, bottom;
                            int padding = PARAM->Value.ToInt();
                            ((QBoxLayout *)item->ptr2)->getContentsMargins(&left, &top, &right, &bottom);
                            ((QBoxLayout *)item->ptr2)->setContentsMargins(left, padding, right, padding);
                        }
                        property_set = true;
                        break;

                    case CLASS_LABEL:
                    case CLASS_IMAGE:
                        // same as XPADDING
                        // to change at some point
                        ((QLabel *)item->ptr)->setMargin(PARAM->Value.ToInt());
                        property_set = true;
                        break;
                }
            }
            break;

        case P_ROWSPACING:
            switch (item->type) {
                case CLASS_TABLE:
                    ((QGridLayout *)item->ptr2)->setVerticalSpacing(PARAM->Value.ToInt());
                    property_set = true;
                    break;
            }
            break;

        case P_COLUMNSPACING:
            switch (item->type) {
                case CLASS_TABLE:
                    ((QGridLayout *)item->ptr2)->setHorizontalSpacing(PARAM->Value.ToInt());
                    property_set = true;
                    break;
            }
            break;

        case P_BORDERWIDTH:
            if ((item->ptr) && (item->ptr2)) {
                ((QBoxLayout *)item->ptr2)->setMargin(PARAM->Value.ToInt());
                property_set = true;
            }
            break;

        case P_FULLSCREEN:
            if (item->ptr) {
                if (PARAM->Value.ToInt())
                    ((QWidget *)item->ptr)->setWindowState(Qt::WindowFullScreen);
                else
                    ((QWidget *)item->ptr)->setWindowState(Qt::WindowNoState);
                property_set = true;
            }
            break;

        case P_TOOLTIPMARKUP:
        case P_TOOLTIP:
            switch (item->type) {
                case CLASS_TOOLSEPARATOR:
                    ((QAction *)item->ptr2)->setToolTip(QString::fromUtf8(PARAM->Value.c_str()));
                    ((QAction *)item->ptr2)->setWhatsThis(QString::fromUtf8(PARAM->Value.c_str()));
                    property_set = true;
                    break;

                case CLASS_EDIT:
                    ((QLineEdit *)item->ptr)->setPlaceholderText(QString::fromUtf8(PARAM->Value.c_str()));
                    ((QLineEdit *)item->ptr)->setToolTip(QString::fromUtf8(PARAM->Value.c_str()));
                    ((QLineEdit *)item->ptr)->setWhatsThis(QString::fromUtf8(PARAM->Value.c_str()));
                    property_set = true;
                    break;

                case CLASS_COMBOBOXTEXT:
                    {
                        QLineEdit *edit = ((QComboBox *)item->ptr)->lineEdit();
                        if (edit)
                            edit->setPlaceholderText(QString::fromUtf8(PARAM->Value.c_str()));
                        ((QLineEdit *)item->ptr)->setToolTip(QString::fromUtf8(PARAM->Value.c_str()));
                        ((QLineEdit *)item->ptr)->setWhatsThis(QString::fromUtf8(PARAM->Value.c_str()));
                    }
                    property_set = true;
                    break;

                case CLASS_TOOLBUTTON:
                case CLASS_TOGGLETOOLBUTTON:
                case CLASS_RADIOTOOLBUTTON:
                case CLASS_IMAGEMENUITEM:
                case CLASS_STOCKMENUITEM:
                case CLASS_CHECKMENUITEM:
                case CLASS_MENUITEM:
                case CLASS_TEAROFFMENUITEM:
                case CLASS_RADIOMENUITEM:
                case CLASS_SEPARATORMENUITEM:
                    ((QAction *)item->ptr2)->setToolTip(QString::fromUtf8(PARAM->Value.c_str()));
                    ((QAction *)item->ptr2)->setWhatsThis(QString::fromUtf8(PARAM->Value.c_str()));
                    property_set = true;

                // no break;
                default:
                    if (item->ptr) {
                        ((QWidget *)item->ptr)->setToolTip(QString::fromUtf8(PARAM->Value.c_str()));
                        ((QWidget *)item->ptr)->setWhatsThis(QString::fromUtf8(PARAM->Value.c_str()));
                        property_set = true;
                    }
            }
            break;

        case P_MINHEIGHT:
            if (item->ptr) {
                int size = PARAM->Value.ToInt();
                if (size < 0)
                    size = 0;

                ((QWidget *)item->ptr)->setMinimumHeight(size);
                if (size)
                    item->minset = 1;
                else
                    item->minset = 0;
                property_set = true;
            }
            break;

        case P_MINWIDTH:
            if (item->ptr) {
                int size = PARAM->Value.ToInt();
                if (size < 0)
                    size = 0;

                ((QWidget *)item->ptr)->setMinimumWidth(size);
                if (size)
                    item->minset = 1;
                else
                    item->minset = 0;
                property_set = true;
            }
            break;

        case P_MASKEDCHAR:
        case P_CAPSWARNING:
            // not supported
            property_set = true;
            break;

        case P_MASKED:
            {
                switch (item->type) {
                    case CLASS_EDIT:
                        if (PARAM->Value.ToInt())
                            ((QLineEdit *)item->ptr)->setEchoMode(QLineEdit::Password);
                        else
                            ((QLineEdit *)item->ptr)->setEchoMode(QLineEdit::Normal);
                        property_set = true;
                        break;

                    case CLASS_COMBOBOXTEXT:
                        {
                            QLineEdit *edit = ((QComboBox *)item->ptr)->lineEdit();
                            if (edit) {
                                if (PARAM->Value.ToInt())
                                    edit->setEchoMode(QLineEdit::Password);
                                else
                                    edit->setEchoMode(QLineEdit::Normal);
                            }
                        }
                        property_set = true;
                        break;
                    case 1016:
                        ((AudioObject *)item->ptr3)->DRM = PARAM->Value.ToInt();
                        property_set = true;
                        break;
                }
            }
            break;
            
        case P_FORMAT:
            switch (item->type) {
                case CLASS_EDIT:
                    ((QLineEdit *)item->ptr)->setInputMask(QString::fromUtf8(PARAM->Value.c_str()));
                    property_set = true;
                    break;

                case CLASS_COMBOBOXTEXT:
                    {
                        QLineEdit *edit = ((QComboBox *)item->ptr)->lineEdit();
                        if (edit)
                            ((QLineEdit *)item->ptr)->setInputMask(QString::fromUtf8(PARAM->Value.c_str()));
                    }
                    property_set = true;
                    break;
            }
            break;

        case P_PRIMARYICON:
            {
                GTKControl *item2 = Client->Controls[PARAM->Value.ToInt()];
                if ((!item2) || (item2->type != CLASS_IMAGE)) {
                    if (item->ptr3)
                        ((QAction *)item->ptr3)->setVisible(false);
                    break;
                }

                const QPixmap *pmap = ((QLabel *)item2->ptr)->pixmap();

                switch (item->type) {
                    case CLASS_EDIT:
                        if (pmap) {
                            if (!item->ptr2)
                                item->ptr2 = ((QLineEdit *)item->ptr)->addAction(QIcon(*pmap), QLineEdit::LeadingPosition);
                            else
                                ((QAction *)item->ptr2)->setIcon(QIcon(*pmap));
                        } else
                        if (item->ptr2)
                            ((QAction *)item->ptr2)->setVisible(false);
                        property_set = true;
                        break;

                    case CLASS_COMBOBOXTEXT:
                        property_set = true;
                        break;
                }
            }
            break;

        case P_SECONDARYICON:
            {
                GTKControl *item2 = Client->Controls[PARAM->Value.ToInt()];
                if ((!item2) || (item2->type != CLASS_IMAGE)) {
                    if (item->ptr3)
                        ((QAction *)item->ptr3)->setVisible(false);

                    break;
                }

                const QPixmap *pmap = ((QLabel *)item2->ptr)->pixmap();

                switch (item->type) {
                    case CLASS_EDIT:
                        if (pmap) {
                            if (!item->ptr3)
                                item->ptr3 = ((QLineEdit *)item->ptr)->addAction(QIcon(*pmap), QLineEdit::TrailingPosition);
                            else
                                ((QAction *)item->ptr3)->setIcon(QIcon(*pmap));
                        } else
                        if (item->ptr3)
                            ((QAction *)item->ptr3)->setVisible(false);
                        property_set = true;
                        break;

                    case CLASS_COMBOBOXTEXT:
                        property_set = true;
                        break;
                }
            }
            break;

        case P_PRIMARYACTIVABLE:
            switch (item->type) {
                case CLASS_EDIT:
                    if (item->ptr2)
                        ((QAction *)item->ptr2)->setEnabled((bool)PARAM->Value.ToInt());
                    break;

                case CLASS_COMBOBOXTEXT:
                    property_set = true;
                    break;
            }
            break;

        case P_SECONDARYACTIVABLE:
            switch (item->type) {
                case CLASS_EDIT:
                    if (item->ptr3)
                        ((QAction *)item->ptr3)->setEnabled(PARAM->Value.ToInt());
                    property_set = true;
                    break;

                case CLASS_COMBOBOXTEXT:
                    property_set = true;
                    break;
            }
            break;

        case P_PRIMARYTOOLTIP:
            {
                switch (item->type) {
                    case CLASS_EDIT:
                        if (item->ptr2)
                            ((QAction *)item->ptr2)->setToolTip(QString::fromUtf8(PARAM->Value.c_str()));
                        property_set = true;
                        break;

                    case CLASS_COMBOBOXTEXT:
                        property_set = true;
                        break;
                }
            }
            break;

        case P_SECONDARYTOOLTIP:
            {
                switch (item->type) {
                    case CLASS_EDIT:
                        if (item->ptr3)
                            ((QAction *)item->ptr3)->setToolTip(QString::fromUtf8(PARAM->Value.c_str()));
                        property_set = true;
                        break;

                    case CLASS_COMBOBOXTEXT:
                        property_set = true;
                        break;
                }
            }
            break;

        case P_PRIMARYICONNAME:
            {
                switch (item->type) {
                    case CLASS_EDIT:
                        if (!item->ptr2)
                            item->ptr2 = ((QLineEdit *)item->ptr)->addAction(QIcon(StockIcon(PARAM->Value.c_str())), QLineEdit::LeadingPosition);
                        else
                            ((QAction *)item->ptr2)->setIcon(QIcon(StockIcon(PARAM->Value.c_str())));
                        property_set = true;
                        break;

                    case CLASS_COMBOBOXTEXT:
                        property_set = true;
                        break;
                }
            }
            break;

        case P_SECONDARYICONNAME:
            {
                switch (item->type) {
                    case CLASS_EDIT:
                        if (!item->ptr3)
                            item->ptr3 = ((QLineEdit *)item->ptr)->addAction(QIcon(StockIcon(PARAM->Value.c_str())), QLineEdit::TrailingPosition);
                        else
                            ((QAction *)item->ptr3)->setIcon(QIcon(StockIcon(PARAM->Value.c_str())));
                        property_set = true;
                        break;

                    case CLASS_COMBOBOXTEXT:
                        property_set = true;
                        break;
                }
            }
            break;

        case P_SUGESTION:
            {
                switch (item->type) {
                    case CLASS_EDIT:
                        {
                            QStringList list       = QString::fromUtf8(PARAM->Value.c_str()).split(QString(";"), QString::SkipEmptyParts);
                            QCompleter  *completer = new QCompleter(list);
                            completer->setCaseSensitivity(Qt::CaseInsensitive);
                            ((QLineEdit *)item->ptr)->setCompleter(completer);
                        }
                        property_set = true;
                        break;

                    case CLASS_COMBOBOXTEXT:
                        {
                            QLineEdit *edit = ((QComboBox *)item->ptr)->lineEdit();
                            if (edit) {
                                QStringList list       = QString::fromUtf8(PARAM->Value.c_str()).split(QString(";"), QString::SkipEmptyParts);
                                QCompleter  *completer = new QCompleter(list);
                                completer->setCaseSensitivity(Qt::CaseInsensitive);
                                edit->setCompleter(completer);
                            }
                        }
                        property_set = true;
                        break;
                }
            }
            break;

        case P_SUGESTIONMODEL:
            {
                QAbstractItemModel *model = NULL;

                GTKControl *item2 = Client->Controls[PARAM->Value.ToInt()];
                if (item2) {
                    switch (item2->type) {
                        case CLASS_TREEVIEW:
                            model = ((QTreeWidget *)item->ptr)->model();
                            break;

                        case CLASS_ICONVIEW:
                            model = ((QListWidget *)item->ptr)->model();
                            break;

                        case CLASS_COMBOBOX:
                        case CLASS_COMBOBOXTEXT:
                            model = ((QComboBox *)item->ptr)->model();
                            break;
                    }
                }

                switch (item->type) {
                    case CLASS_EDIT:
                        {
                            QCompleter *completer = new QCompleter(model);
                            completer->setCaseSensitivity(Qt::CaseInsensitive);
                            ((QLineEdit *)item->ptr)->setCompleter(completer);
                        }
                        property_set = true;
                        break;

                    case CLASS_COMBOBOXTEXT:
                        {
                            QLineEdit *edit = ((QComboBox *)item->ptr)->lineEdit();
                            if (edit) {
                                QCompleter *completer = new QCompleter(model);
                                completer->setCaseSensitivity(Qt::CaseInsensitive);
                                edit->setCompleter(completer);
                            }
                        }
                        property_set = true;
                        break;
                }
            }
            break;

        case P_READONLY:
            {
                switch (item->type) {
                    case CLASS_EDIT:
                        ((QLineEdit *)item->ptr)->setReadOnly((bool)PARAM->Value.ToInt());
                        property_set = true;
                        break;

                    case CLASS_COMBOBOXTEXT:
                        {
                            QLineEdit *edit = ((QComboBox *)item->ptr)->lineEdit();
                            if (edit)
                                edit->setReadOnly((bool)PARAM->Value.ToInt());
                        }
                        property_set = true;
                        break;

                    case CLASS_TEXTVIEW:
                        ((QTextEdit *)item->ptr)->setReadOnly((bool)PARAM->Value.ToInt());
                        property_set = true;
                        break;

                    case CLASS_TEXTTAG:
                        property_set = true;
                        break;
                }
            }
            break;

        case P_MAXLEN:
            {
                int len = PARAM->Value.ToInt();
                switch (item->type) {
                    case CLASS_EDIT:
                        if (len <= 0)
                            len = 32767;
                        ((QLineEdit *)item->ptr)->setMaxLength(len);
                        property_set = true;
                        break;

                    case CLASS_COMBOBOXTEXT:
                        {
                            if (len <= 0)
                                len = 32767;
                            QLineEdit *edit = ((QComboBox *)item->ptr)->lineEdit();
                            if (edit)
                                edit->setMaxLength(len);
                        }
                        property_set = true;
                        break;
                    case 1016:
                        ((AudioObject *)item->ptr3)->maxbuffers = PARAM->Value.ToInt();
                        property_set = true;
                        break;
                }
            }
            break;

        case P_ACTIVATEDEFAULT:
            // deprecated
            // use REdit.OnActivate
            property_set = true;
            break;

        case P_BORDER:
            {
                switch (item->type) {
                    case CLASS_EDIT:
                        ((QLineEdit *)item->ptr)->setFrame((bool)PARAM->Value.ToInt());
                        property_set = true;
                        break;

                    case CLASS_COMBOBOXTEXT:
                    case CLASS_COMBOBOX:
                        ((QComboBox *)item->ptr)->setFrame((bool)PARAM->Value.ToInt());
                        property_set = true;
                        break;

                    case CLASS_NOTEBOOK:
                    case CLASS_IMAGEMENUITEM:
                    case CLASS_STOCKMENUITEM:
                    case CLASS_CHECKMENUITEM:
                    case CLASS_MENUITEM:
                    case CLASS_TEAROFFMENUITEM:
                    case CLASS_RADIOMENUITEM:
                    case CLASS_SEPARATORMENUITEM:
                    case CLASS_ICONVIEW:
                    case CLASS_TREEVIEW:
                        property_set = true;
                        break;

                    default:
                        if ((item->ptr) && (item->ptr2)) {
                            ((QLayout *)item->ptr2)->setMargin(PARAM->Value.ToInt());
                            property_set = true;
                        }
                        break;
                }
            }
            break;

        case P_CHECKED:
            {
                switch (item->type) {
                    case CLASS_CHECKBUTTON:
                        ((QCheckBox *)item->ptr)->setChecked((bool)PARAM->Value.ToInt());
                        property_set = true;
                        break;

                    case CLASS_RADIOBUTTON:
                        ((QRadioButton *)item->ptr)->setChecked((bool)PARAM->Value.ToInt());
                        property_set = true;
                        break;

                    case CLASS_RADIOMENUITEM:
                    case CLASS_CHECKMENUITEM:
                    case CLASS_RADIOTOOLBUTTON:
                    case CLASS_TOGGLETOOLBUTTON:
                        ((QAction *)item->ptr2)->setChecked((bool)PARAM->Value.ToInt());
                        property_set = true;
                        break;
                }
            }
            break;

        case P_INCONSISTENT:
            {
                switch (item->type) {
                    case CLASS_CHECKBUTTON:
                        if (PARAM->Value.ToInt())
                            ((QCheckBox *)item->ptr)->setCheckState(Qt::PartiallyChecked);
                        else {
                            if (((QCheckBox *)item->ptr)->isChecked())
                                ((QCheckBox *)item->ptr)->setCheckState(Qt::Checked);
                            else
                                ((QCheckBox *)item->ptr)->setCheckState(Qt::Unchecked);
                        }
                        property_set = true;
                        break;
                }
            }
            break;

        case P_GROUP:
            {
                switch (item->type) {
                    case CLASS_RADIOBUTTON:
                        {
                            GTKControl *item2 = Client->Controls[PARAM->Value.ToInt()];
                            if ((item2) && (item2->type == CLASS_RADIOBUTTON) && (item != item2)) {
                                QButtonGroup *group = ((QRadioButton *)item2->ptr)->group();
                                if (!group) {
                                    group = new QButtonGroup((QRadioButton *)item2->ptr);
                                    group->addButton((QRadioButton *)item2->ptr);
                                }
                                group->addButton((QRadioButton *)item->ptr);
                            }
                        }
                        property_set = true;
                        break;

                    case CLASS_RADIOMENUITEM:
                    case CLASS_RADIOTOOLBUTTON:
                        {
                            GTKControl *item2 = Client->Controls[PARAM->Value.ToInt()];
                            if ((item2) && ((item2->type == CLASS_RADIOMENUITEM) || (item2->type == CLASS_RADIOTOOLBUTTON)) && (item != item2)) {
                                QActionGroup *group = ((QAction *)item2->ptr2)->actionGroup();
                                if (!group) {
                                    group = new QActionGroup((QAction *)item2->ptr2);
                                    ((QAction *)item2->ptr2)->setActionGroup(group);
                                }
                                group->addAction((QAction *)item->ptr2);
                            }
                        }
                        property_set = true;
                        break;
                }
            }
            break;

        case P_ISDETACHED:
            switch (item->type) {
                case CLASS_HANDLEBOX:
                    ((QDockWidget *)item->ptr)->setFloating((bool)PARAM->Value.ToInt());
                    property_set = true;
                    break;
            }
            break;

        case P_SELECTABLE:
            if (item->type == CLASS_LABEL) {
                if (PARAM->Value.ToInt())
                    ((QLabel *)item->ptr)->setTextInteractionFlags(Qt::TextSelectableByMouse);
                else
                    ((QLabel *)item->ptr)->setTextInteractionFlags(Qt::NoTextInteraction);
                property_set = true;
            }
            break;

        case P_USEUNDERLINE:
        case P_PATTERN:
            // deprecated
            property_set = true;
            break;

        case P_WRAP:
            {
                switch (item->type) {
                    case CLASS_LABEL:
                        ((QLabel *)item->ptr)->setWordWrap((bool)PARAM->Value.ToInt());
                        property_set = true;
                        break;

                    case CLASS_SPINBUTTON:
                        ((QSpinBox *)item->ptr)->setWrapping((bool)PARAM->Value.ToInt());
                        property_set = true;
                        break;

                    case CLASS_TEXTVIEW:
                        switch (PARAM->Value.ToInt()) {
                            case 1:
                            case 3:
                                ((QTextEdit *)item->ptr)->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
                                break;

                            case 2:
                                ((QTextEdit *)item->ptr)->setWordWrapMode(QTextOption::WordWrap);
                                break;

                            case 0:
                            default:
                                ((QTextEdit *)item->ptr)->setWordWrapMode(QTextOption::NoWrap);
                        }
                        property_set = true;
                        break;

                    case CLASS_TEXTTAG:
                    case CLASS_PRINTER:
                        property_set = true;
                        break;
                }
            }
            break;

        case P_JUSTIFY:
            {
                switch (item->type) {
                    case CLASS_TEXTVIEW:
                        switch (PARAM->Value.ToInt()) {
                            case 1:
                                ((QTextEdit *)item->ptr)->setAlignment(Qt::AlignRight);
                                break;

                            case 2:
                                ((QTextEdit *)item->ptr)->setAlignment(Qt::AlignCenter);
                                break;

                            case 3:
                                ((QTextEdit *)item->ptr)->setAlignment(Qt::AlignJustify);
                                break;

                            case 0:
                            default:
                                ((QTextEdit *)item->ptr)->setAlignment(Qt::AlignLeft);
                        }
                        property_set = true;
                        break;

                    case CLASS_LABEL:
                        switch (PARAM->Value.ToInt()) {
                            case 1:
                                ((QLineEdit *)item->ptr)->setAlignment(Qt::AlignRight);
                                break;

                            case 2:
                                ((QLineEdit *)item->ptr)->setAlignment(Qt::AlignCenter);
                                break;

                            case 3:
                                ((QLineEdit *)item->ptr)->setAlignment(Qt::AlignJustify);
                                break;

                            case 0:
                            default:
                                ((QLineEdit *)item->ptr)->setAlignment(Qt::AlignLeft);
                        }
                        property_set = true;
                        break;

                    case CLASS_TEXTTAG:
                        property_set = true;
                        break;

                    case CLASS_PRINTER:
                        property_set = true;
                        break;
                }
            }
            break;

        case P_URGENT:
            {
                switch (item->type) {
                    case CLASS_FORM:
                        QApplication::alert((QWidget *)item->ptr);
                        //QApplication::setActiveWindow((QWidget *)item->ptr);
                        property_set = true;
                        break;
                }
            }
            break;

        case P_SHADOWTYPE:
            {
                switch (item->type) {
                    case CLASS_FRAME:
                        ((QGroupBox *)item->ptr)->setFlat(!PARAM->Value.ToInt());
                        property_set = true;
                        break;

                    case CLASS_SCROLLEDWINDOW:
                    case CLASS_TREEVIEW:
                    case CLASS_ICONVIEW:
                    case CLASS_TEXTVIEW:
                        switch (PARAM->Value.ToInt()) {
                            case 0:
                                ((QFrame *)item->ptr)->setFrameShape(QFrame::NoFrame);
                                ((QFrame *)item->ptr)->setLineWidth(0);
                                break;

                            case 1:
                                ((QFrame *)item->ptr)->setFrameShape(QFrame::Box);
                                ((QFrame *)item->ptr)->setFrameShadow(QFrame::Plain);
                                break;

                            case 2:
                                ((QFrame *)item->ptr)->setFrameShape(QFrame::Box);
                                ((QFrame *)item->ptr)->setFrameShadow(QFrame::Raised);
                                break;

                            case 3:
                                ((QFrame *)item->ptr)->setFrameShape(QFrame::Panel);
                                ((QFrame *)item->ptr)->setFrameShadow(QFrame::Sunken);
                                break;

                            case 4:
                                ((QFrame *)item->ptr)->setFrameShape(QFrame::Panel);
                                ((QFrame *)item->ptr)->setFrameShadow(QFrame::Raised);
                                break;

                            default:
                                ((QFrame *)item->ptr)->setFrameShape(QFrame::NoFrame);
                        }
                        property_set = true;
                        break;

                    case CLASS_VIEWPORT:
                        property_set = true;
                        break;

                    case CLASS_HANDLEBOX:
                        property_set = true;
                        break;
                }
            }
            break;

        case P_HTEXTALIGN:
            if (item->type == CLASS_FRAME) {
                double        d     = PARAM->Value.ToFloat();
                Qt::Alignment align = ((QGroupBox *)item->ptr)->alignment();
                align &= 0xFFF0;
                if (d == -1)
                    ((QGroupBox *)item->ptr)->setAlignment(align | Qt::AlignJustify);
                else
                if (d <= 0.1)
                    ((QGroupBox *)item->ptr)->setAlignment(align | Qt::AlignLeft);
                else
                if (d >= 0.9)
                    ((QGroupBox *)item->ptr)->setAlignment(align | Qt::AlignRight);
                else
                    ((QGroupBox *)item->ptr)->setAlignment(align | Qt::AlignHCenter);
                property_set = true;
            }
            break;

        case P_VTEXTALIGN:
            if (item->type == CLASS_FRAME) {
                double        d     = PARAM->Value.ToFloat();
                Qt::Alignment align = ((QGroupBox *)item->ptr)->alignment();
                align &= 0xFF0F;
                if (d <= 0.1)
                    ((QGroupBox *)item->ptr)->setAlignment(align | Qt::AlignTop);
                else
                if (d >= 0.9)
                    ((QGroupBox *)item->ptr)->setAlignment(align | Qt::AlignBottom);
                else
                    ((QGroupBox *)item->ptr)->setAlignment(align | Qt::AlignVCenter);
                property_set = true;
            }
            break;

        case P_FRACTION:
            {
                switch (item->type) {
                    case CLASS_PROGRESSBAR:
                        {
                            int max = ((QProgressBar *)item->ptr)->maximum();
                            if (!max)
                                ((QProgressBar *)item->ptr)->setMaximum(10000);
                            ((QProgressBar *)item->ptr)->setValue(PARAM->Value.ToFloat() * max);
                        }
                        property_set = true;
                        break;

                    case CLASS_EDIT:
                        {
                            double step = PARAM->Value.ToFloat();
                            if (step <= 0)
                                ((QLineEdit *)item->ptr)->setStyleSheet(QString());
                            else
                            if (step < 0.995) {
                                AnsiString style("QLineEdit {background: qlineargradient(x1:0, y1:0.5, x2:");
                                style += PARAM->Value;
                                style += " y2:0.5, stop:0 #f0fff0, stop: 0.95 lightblue, stop:1 white);}";
                                ((QLineEdit *)item->ptr)->setStyleSheet(style.c_str());
                            } else
                                ((QLineEdit *)item->ptr)->setStyleSheet("QLineEdit {background: lightblue;}");
                        }
                        property_set = true;
                        break;

                    case CLASS_COMBOBOXTEXT:
                        {
                            QLineEdit *edit = ((QComboBox *)item->ptr)->lineEdit();
                            if (edit) {
                                double step = PARAM->Value.ToFloat();
                                if (step <= 0)
                                    edit->setStyleSheet(QString());
                                else
                                if (step < 0.995) {
                                    AnsiString style("QLineEdit {background: qlineargradient(x1:0, y1:0.5, x2:");
                                    style += PARAM->Value;
                                    style += " y2:0.5, stop:0 #f0fff0, stop: 0.95 lightblue, stop:1 white);}";
                                    edit->setStyleSheet(style.c_str());
                                } else
                                    edit->setStyleSheet("QLineEdit {background: lightblue;}");
                            }
                        }
                        property_set = true;
                        break;
                }
            }
            break;

        case P_STEP:
            {
                // not implemented yet
                switch (item->type) {
                    case CLASS_PROGRESSBAR:
                        property_set = true;
                        break;

                    case CLASS_EDIT:
                        property_set = true;
                        break;

                    case CLASS_COMBOBOXTEXT:
                        property_set = true;
                        break;
                        
                    case 1016:
                        ((AudioObject *)item->ptr3)->framesize = PARAM->Value.ToInt();
                        property_set = true;
                        break;
                }
            }
            break;

        case P_TYPE:
            {
                switch (item->type) {
                    case CLASS_FORM:
                        int             vflags     = PARAM->Value.ToInt();
                        Qt::WindowFlags flags_mask = Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint;
                        flags_mask &= ~(Qt::FramelessWindowHint | Qt::MSWindowsFixedSizeDialogHint);
                        Qt::WindowFlags flags = ((QWidget *)item->ptr)->windowFlags() & (~flags_mask);

                        switch (PARAM->Value.ToInt()) {
                            case 0:
                                ((QWidget *)item->ptr)->setWindowFlags(flags | flags_mask);
                                break;

                            case 1:
                            case 3:
                            case 5:
                            case 7:
                                ((QWidget *)item->ptr)->setWindowFlags(flags | Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
                                break;

                            //case 1:
                            //    ((QWidget *)item->ptr)->setWindowFlags(flags | flags_mask | Qt::Popup);
                            //    break;
                            case 2:
                            default:
                                flags &= ~Qt::WindowCloseButtonHint;
                                flags |= Qt::FramelessWindowHint;
                                ((QWidget *)item->ptr)->setWindowFlags(flags);
                                break;
                        }
                        property_set = true;
                        break;
                }
            }
            break;

        case P_OPACITY:
            if (item->ptr) {
                if (item->type == CLASS_FORM)
                    ((QWidget *)item->ptr)->setParent(NULL);
                ((QWidget *)item->ptr)->setWindowOpacity(PARAM->Value.ToInt());
                property_set = true;
            }
            break;

        case P_CLOSEABLE:
            {
                switch (item->type) {
                    case CLASS_FORM:
                        {
                            item->flags = !PARAM->Value.ToInt();
                            if (item->flags) {
                                ClientEventHandler *events = (ClientEventHandler *)item->events;
                                if (!events) {
                                    events       = new ClientEventHandler(item->ID, Client);
                                    item->events = events;
                                }
                                QObject::connect((QWidget *)item->ptr, SIGNAL(closeEvent(QCloseEvent *)), events, SLOT(on_closeEvent(QCloseEvent *)));
                                if (PARAM->Value.Length() > 3) {
                                    if (!PARAM->Owner->parser)
                                        PARAM->Owner->parser = new CompensationParser(compensated_send_message, compensated_wait_message, PARAM->Owner);

                                    QByteArray ba;
                                    ba.append(PARAM->Value.c_str(), PARAM->Value.Length());
                                    QByteArray ba2  = QByteArray::fromBase64(ba);
                                    AnsiString temp = (char *)ba2.constData();

                                    events->entry_point = PARAM->Owner->parser->Parse(temp.c_str(), temp.Length());
                                }
                            }
                        }
                        property_set = true;
                        break;
                }
            }
            break;

        case P_LOWER:
            {
                GTKControl *parent = (GTKControl *)item->parent;
                if ((parent) && (item->type == CLASS_ADJUSTMENT)) {
                    switch (parent->type) {
                        case CLASS_SPINBUTTON:
                            ((QSpinBox *)parent->ptr)->setMinimum(PARAM->Value.ToInt());
                            break;

                        case CLASS_VSCALE:
                        case CLASS_HSCALE:
                            ((QSlider *)parent->ptr)->setMinimum(PARAM->Value.ToInt());
                            break;

                        case CLASS_VSCROLLBAR:
                        case CLASS_HSCROLLBAR:
                            ((QScrollBar *)parent->ptr)->setMinimum(PARAM->Value.ToInt());
                            break;

                        default:
                            if (item->ptr)
                                ((QScrollBar *)item->ptr)->setMinimum(PARAM->Value.ToInt());
                            break;
                    }
                    property_set = true;
                }
            }
            break;

        case P_UPPER:
            {
                GTKControl *parent = (GTKControl *)item->parent;
                if ((parent) && (item->type == CLASS_ADJUSTMENT)) {
                    switch (parent->type) {
                        case CLASS_SPINBUTTON:
                            ((QSpinBox *)parent->ptr)->setMaximum(PARAM->Value.ToInt());
                            break;

                        case CLASS_VSCALE:
                        case CLASS_HSCALE:
                            ((QSlider *)parent->ptr)->setMaximum(PARAM->Value.ToInt());
                            break;

                        case CLASS_VSCROLLBAR:
                        case CLASS_HSCROLLBAR:
                            ((QScrollBar *)parent->ptr)->setMaximum(PARAM->Value.ToInt());
                            break;

                        default:
                            if (item->ptr)
                                ((QScrollBar *)item->ptr)->setMaximum(PARAM->Value.ToInt());
                            break;
                    }
                    property_set = true;
                }
            }
            break;

        case P_VALUE:
            {
                GTKControl *parent = (GTKControl *)item->parent;
                if ((parent) && (item->type == CLASS_ADJUSTMENT)) {
                    switch (parent->type) {
                        case CLASS_SPINBUTTON:
                            ((QSpinBox *)parent->ptr)->setValue(PARAM->Value.ToInt());
                            break;

                        case CLASS_VSCALE:
                        case CLASS_HSCALE:
                            ((QSlider *)parent->ptr)->setValue(PARAM->Value.ToInt());
                            break;

                        case CLASS_VSCROLLBAR:
                        case CLASS_HSCROLLBAR:
                            ((QScrollBar *)parent->ptr)->setValue(PARAM->Value.ToInt());
                            break;

                        default:
                            if (item->ptr)
                                ((QScrollBar *)item->ptr)->setValue(PARAM->Value.ToInt());
                            break;
                    }
                    property_set = true;
                }
            }
            break;

        case P_INCREMENT:
            {
                GTKControl *parent = (GTKControl *)item->parent;
                if ((parent) && (item->type == CLASS_ADJUSTMENT)) {
                    switch (parent->type) {
                        case CLASS_SPINBUTTON:
                            ((QSpinBox *)parent->ptr)->setSingleStep(PARAM->Value.ToInt());
                            break;

                        case CLASS_VSCALE:
                        case CLASS_HSCALE:
                            ((QSlider *)parent->ptr)->setSingleStep(PARAM->Value.ToInt());
                            break;

                        case CLASS_VSCROLLBAR:
                        case CLASS_HSCROLLBAR:
                            ((QScrollBar *)parent->ptr)->setSingleStep(PARAM->Value.ToInt());
                            break;

                        default:
                            if (item->ptr)
                                ((QScrollBar *)item->ptr)->setSingleStep(PARAM->Value.ToInt());
                            break;
                    }
                    property_set = true;
                }
            }
            break;

        case P_PAGEINCREMENT:
            {
                GTKControl *parent = (GTKControl *)item->parent;
                if ((parent) && (item->type == CLASS_ADJUSTMENT)) {
                    if (parent) {
                        switch (parent->type) {
                            case CLASS_SPINBUTTON:
                                break;

                            case CLASS_VSCALE:
                            case CLASS_HSCALE:
                                ((QSlider *)parent->ptr)->setPageStep(PARAM->Value.ToInt());
                                break;

                            case CLASS_VSCROLLBAR:
                            case CLASS_HSCROLLBAR:
                                ((QScrollBar *)parent->ptr)->setPageStep(PARAM->Value.ToInt());
                                break;

                            default:
                                if (item->ptr)
                                    ((QScrollBar *)item->ptr)->setPageStep(PARAM->Value.ToInt());
                                break;
                        }
                    }
                    property_set = true;
                }
            }
            break;

        case P_PAGESIZE:
            {
                GTKControl *parent = (GTKControl *)item->parent;
                if ((parent) && (item->type == CLASS_ADJUSTMENT)) {
                    switch (parent->type) {
                        case CLASS_VSCALE:
                        case CLASS_HSCALE:
                            ((QSlider *)parent->ptr)->setPageStep(PARAM->Value.ToInt());
                            break;

                        case CLASS_VSCROLLBAR:
                        case CLASS_HSCROLLBAR:
                            ((QScrollBar *)parent->ptr)->setPageStep(PARAM->Value.ToInt());
                            break;

                        default:
                            if (item->ptr)
                                ((QScrollBar *)item->ptr)->setPageStep(PARAM->Value.ToInt());
                    }
                    property_set = true;
                }
            }
            break;

        case P_DIGITS:
            if (item->type == 1015) {
                QCameraImageCapture *imageCapture = (QCameraImageCapture *)item->ptr3;
                if (imageCapture) {
                    imageCapture->capture();
                    if (item->ptr2) {
                        ClientEventHandler *events = (ClientEventHandler *)item->events;
                        if (events)
                            ((QLabel *)item->ptr2)->setPixmap(QPixmap::fromImage(events->img, Qt::AutoColor));
                    }
                }
                property_set = true;
            } else
            if (item->type == 1016) {
                ((AudioObject *)item->ptr3)->NUM_CHANNELS = PARAM->Value.ToInt();
                property_set = true;
            }
            break;

        case P_VALUEPOS:
            {
                if (item->type ==1016) {
                    ((AudioObject *)item->ptr3)->AddBuffer(PARAM->Value.c_str(), PARAM->Value.Length());
                    property_set = true;
                } else {
                    GTKControl *parent = (GTKControl *)item->parent;
                    if (parent) {
                        switch (parent->type) {
                            case CLASS_VSCALE:
                            case CLASS_HSCALE:
                                {
                                    switch (PARAM->Value.ToInt()) {
                                        case 0:
                                            ((QSlider *)parent->ptr)->setTickPosition(QSlider::TicksLeft);
                                            break;

                                        case 1:
                                            ((QSlider *)parent->ptr)->setTickPosition(QSlider::TicksRight);
                                            break;

                                        case 2:
                                            ((QSlider *)parent->ptr)->setTickPosition(QSlider::TicksAbove);
                                            break;

                                        case 3:
                                            ((QSlider *)parent->ptr)->setTickPosition(QSlider::TicksBelow);
                                            break;

                                        case 4:
                                            ((QSlider *)parent->ptr)->setTickPosition(QSlider::TicksBothSides);
                                            break;
                                    }
                                }
                                property_set = true;
                                break;
                        }
                    }
                }
            }
            break;

        case P_INVERTED:
            {
                GTKControl *parent = (GTKControl *)item->parent;
                if (parent) {
                    switch (parent->type) {
                        case CLASS_VSCROLLBAR:
                        case CLASS_HSCROLLBAR:
                            ((QScrollBar *)parent->ptr)->setInvertedAppearance((bool)PARAM->Value.ToInt());
                            property_set = true;
                            break;
                    }
                }
            }
            break;

        case P_SNAPTOTICKS:
        case P_NUMERIC:
            // not needed
            property_set = true;
            break;

        case P_ACCEPTTAB:
            {
                switch (item->type) {
                    case CLASS_TEXTVIEW:
                        ((QTextEdit *)item->ptr)->setTabChangesFocus(!PARAM->Value.ToInt());
                        property_set = true;
                        break;
                }
            }
            break;

        case P_PRINT:
            switch (item->type) {
                case CLASS_TEXTVIEW:
#ifndef NO_PRINT
                    if (PARAM->Value.ToInt()) {
                        QPrinter     printer;
                        QPrintDialog dialog(&printer, (QWidget *)item->ptr);
                        dialogs++;
                        if (dialog.exec() == QDialog::Accepted)
                            ((QTextEdit *)item->ptr)->print(&printer);
                        dialogs--;
                    }
#endif
                    property_set = true;
                    break;
            }
            break;

        case P_SELECTIONSTART:
            switch (item->type) {
                case CLASS_TEXTVIEW:
                    {
                        QTextCursor selection = ((QTextEdit *)item->ptr)->textCursor();
                        selection.setPosition(PARAM->Value.ToInt());
                        ((QTextEdit *)item->ptr)->setTextCursor(selection);
                    }
                    property_set = true;
                    break;
            }
            break;

        case P_SELECTIONLENGTH:
            switch (item->type) {
                case CLASS_TEXTVIEW:
                    {
                        QTextCursor selection = ((QTextEdit *)item->ptr)->textCursor();
                        selection.setPosition(PARAM->Value.ToInt(), QTextCursor::KeepAnchor);
                        ((QTextEdit *)item->ptr)->setTextCursor(selection);
                    }
                    property_set = true;
                    break;
            }
            break;

        case P_CURSORVISIBLE:
            switch (item->type) {
                case CLASS_TEXTVIEW:
                    ((QTextEdit *)item->ptr)->setCursorWidth(PARAM->Value.ToInt());
                    property_set = true;
                    break;
            }
            break;

        case P_INDENT:
            switch (item->type) {
                case CLASS_TEXTVIEW:
                    ((QTextEdit *)item->ptr)->setStyleSheet(((QWidget *)item->ptr)->styleSheet() + QString("text-indent: ") + QString::fromUtf8(PARAM->Value.c_str()) + QString("px;"));
                    property_set = true;
                    break;
            }
            break;

        case P_LEFTMARGIN:
            switch (item->type) {
                case CLASS_TEXTVIEW:
                    ((QTextEdit *)item->ptr)->setStyleSheet(((QWidget *)item->ptr)->styleSheet() + QString("margin-left: ") + QString::fromUtf8(PARAM->Value.c_str()) + QString("px;"));
                    property_set = true;
                    break;
            }
            break;

        case P_RIGHTMARGIN:
            switch (item->type) {
                case CLASS_TEXTVIEW:
                    ((QTextEdit *)item->ptr)->setStyleSheet(((QWidget *)item->ptr)->styleSheet() + QString("margin-right: ") + QString::fromUtf8(PARAM->Value.c_str()) + QString("px;"));
                    property_set = true;
                    break;
            }
            break;

        case P_OVERWRITE:
            {
                switch (item->type) {
                    case CLASS_TEXTVIEW:
                        ((QTextEdit *)item->ptr)->setOverwriteMode((bool)PARAM->Value.ToInt());
                        property_set = true;
                        break;

                    case CLASS_EDIT:
                        // not supported
                        //((QLineEdit *)item->ptr)->setOverwriteMode((bool)PARAM->Value.ToInt());
                        property_set = true;
                        break;

                    case CLASS_COMBOBOXTEXT:
                        {
                            // not supported
                            //QLineEdit *edit = ((QComboBox *)item->ptr)->lineEdit();
                            //if (edit)
                            //    edit->setOverwriteMode((bool)PARAM->Value.ToInt());
                        }
                        property_set = true;
                        break;
                }
            }
            break;

        case P_PIXELSABOVE:
            switch (item->type) {
                case CLASS_TEXTVIEW:
                    ((QTextEdit *)item->ptr)->setStyleSheet(((QWidget *)item->ptr)->styleSheet() + QString("padding-top: ") + QString::fromUtf8(PARAM->Value.c_str()) + QString("px;"));
                    property_set = true;
                    break;
            }
            break;

        case P_PIXELSBELOW:
            switch (item->type) {
                case CLASS_TEXTVIEW:
                    ((QTextEdit *)item->ptr)->setStyleSheet(((QWidget *)item->ptr)->styleSheet() + QString("padding-bottom: ") + QString::fromUtf8(PARAM->Value.c_str()) + QString("px;"));
                    property_set = true;
                    break;
            }
            break;

        case P_PIXELSINSIDE:
            switch (item->type) {
                case CLASS_TEXTVIEW:
                    ((QTextEdit *)item->ptr)->setStyleSheet(((QWidget *)item->ptr)->styleSheet() + QString("padding: ") + QString::fromUtf8(PARAM->Value.c_str()) + QString("px;"));
                    property_set = true;
                    break;
            }
            break;

        case P_XSCALE:
            // not supported/needed
            property_set = true;
            break;

        case P_YSCALE:
            // not supported/needed
            property_set = true;
            break;

        case P_VERTICALBORDER:
            {
                switch (item->type) {
                    case CLASS_NOTEBOOK:
                        ((QTextEdit *)item->ptr)->setStyleSheet(((QWidget *)item->ptr)->styleSheet() + QString("border-left: ") + QString::fromUtf8(PARAM->Value.c_str()) + QString("px solid;"));
                        ((QTextEdit *)item->ptr)->setStyleSheet(((QWidget *)item->ptr)->styleSheet() + QString("border-right: ") + QString::fromUtf8(PARAM->Value.c_str()) + QString("px solid;"));
                        property_set = true;
                        break;
                }
            }
            break;

        case P_HORIZONTALBORDER:
            {
                switch (item->type) {
                    case CLASS_NOTEBOOK:
                        ((QTextEdit *)item->ptr)->setStyleSheet(((QWidget *)item->ptr)->styleSheet() + QString("border-top: ") + QString::fromUtf8(PARAM->Value.c_str()) + QString("px solid;"));
                        ((QTextEdit *)item->ptr)->setStyleSheet(((QWidget *)item->ptr)->styleSheet() + QString("border-bottom: ") + QString::fromUtf8(PARAM->Value.c_str()) + QString("px solid;"));
                        property_set = true;
                        break;
                }
            }
            break;

        case P_DRAGDATA:
            if ((item) && (item->ptr)) {
                DERIVED_TEMPLATE_SET(item, dragData, PARAM->Value.c_str());
                property_set = true;
            }
            break;

        case P_DROPSITE:
            if ((item) && (item->ptr)) {
                int prop = PARAM->Value.ToInt();
                ((QWidget *)item->ptr)->setAcceptDrops((bool)prop);
                DERIVED_TEMPLATE_SET(item, dropsite, prop);
                property_set = true;
            }
            break;

        case P_DRAGICON:
            if ((item) && (item->ptr)) {
                GTKControl *item2 = Client->Controls[PARAM->Value.ToInt()];
                if ((item2) && (item2->type == CLASS_IMAGE)) {
                    const QPixmap *pmap = ((QLabel *)item2->ptr)->pixmap();
                    if (pmap) {
                        DERIVED_TEMPLATE_SET(item, dragicon, *pmap);
                    }
                }
                property_set = true;
            }
            break;

        case P_DRAGOBJECT:
            if ((item) && (item->ptr)) {
                switch (item->type) {
                    case CLASS_TREEVIEW:
                    case CLASS_ICONVIEW:
                        {
                            QAbstractItemView::DragDropMode dragMode = ((QAbstractItemView *)item->ptr)->dragDropMode();
                            if (PARAM->Value.ToInt()) {
                                if (dragMode == QAbstractItemView::DropOnly)
                                    ((QAbstractItemView *)item->ptr)->setDragDropMode(QAbstractItemView::DragOnly);
                                else
                                    ((QAbstractItemView *)item->ptr)->setDragDropMode(QAbstractItemView::DragOnly);
                            } else {
                                if (dragMode == QAbstractItemView::DragOnly)
                                    ((QAbstractItemView *)item->ptr)->setDragDropMode(QAbstractItemView::NoDragDrop);
                                else
                                if (dragMode == QAbstractItemView::DragDrop)
                                    ((QAbstractItemView *)item->ptr)->setDragDropMode(QAbstractItemView::DropOnly);
                            }
                            int dropsite = 0;
                            DERIVED_TEMPLATE_GET(item, dropsite, dropsite);
                            if (dropsite)
                                ((QWidget *)item->ptr)->setAcceptDrops(true);
                        }
                        break;

                    default:
                        DERIVED_TEMPLATE_SET(item, dragable, (bool)PARAM->Value.ToInt());
                }
                property_set = true;
            }
            break;

        case P_HADJUSTMENT:
            {
                GTKControl *item2 = Client->Controls[PARAM->Value.ToInt()];
                if ((item2) && (item2->type == CLASS_ADJUSTMENT)) {
                    item->ID_HAdjustment = item2->ID;
                    item2->parent        = item;
                    SetAdjustment(PARAM->Owner, item2);
                } else
                    item->ID_HAdjustment = -1;
                property_set = true;
            }
            break;

        case P_VADJUSTMENT:
            {
                GTKControl *item2 = Client->Controls[PARAM->Value.ToInt()];
                if ((item2) && (item2->type == CLASS_ADJUSTMENT)) {
                    item->ID_Adjustment = item2->ID;
                    item2->parent       = item;
                    SetAdjustment(PARAM->Owner, item2);
                } else
                    item->ID_Adjustment = -1;
                property_set = true;
            }
            break;

        case P_PLACEMENT:
            switch (item->type) {
                case CLASS_SCROLLEDWINDOW:
                    switch (PARAM->Value.ToInt()) {
                        case 1:
                            ((QScrollArea *)item->ptr)->setAlignment(Qt::AlignBottom | Qt::AlignLeft);
                            break;

                        case 2:
                            ((QScrollArea *)item->ptr)->setAlignment(Qt::AlignTop | Qt::AlignRight);
                            break;

                        case 4:
                            ((QScrollArea *)item->ptr)->setAlignment(Qt::AlignBottom | Qt::AlignRight);
                            break;

                        default:
                            ((QScrollArea *)item->ptr)->setAlignment(Qt::AlignTop | Qt::AlignLeft);
                    }
                    property_set = true;
                    break;
            }
            break;

        case P_HSCROLLBARPOLICY:
            switch (item->type) {
                case CLASS_SCROLLEDWINDOW:
                    switch (PARAM->Value.ToInt()) {
                        case 0:
                            ((QScrollArea *)item->ptr)->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
                            break;

                        case 2:
                            ((QScrollArea *)item->ptr)->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                            break;

                        case 1:
                        default:
                            ((QScrollArea *)item->ptr)->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
                            break;
                    }
                    QWidget *widget = ((QScrollArea *)item->ptr)->widget();
                    if (widget) {
                        GTKControl *item2 = Client->ControlsReversed[widget];
                        if ((item2) && ((item2->type == CLASS_TREEVIEW) || (item2->type == CLASS_ICONVIEW) || (item2->type == CLASS_TEXTVIEW))) {
                            if (((QScrollArea *)item->ptr)->horizontalScrollBarPolicy() != Qt::ScrollBarAlwaysOn)
                                ((QAbstractScrollArea *)widget)->setHorizontalScrollBarPolicy(((QScrollArea *)item->ptr)->horizontalScrollBarPolicy());
                            if (((QScrollArea *)item->ptr)->verticalScrollBarPolicy() != Qt::ScrollBarAlwaysOn)
                                ((QAbstractScrollArea *)widget)->setVerticalScrollBarPolicy(((QScrollArea *)item->ptr)->verticalScrollBarPolicy());
                        }
                    }
                    property_set = true;
                    break;
            }
            break;

        case P_VSCROLLBARPOLICY:
            switch (item->type) {
                case CLASS_SCROLLEDWINDOW:
                    switch (PARAM->Value.ToInt()) {
                        case 0:
                            ((QScrollArea *)item->ptr)->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
                            break;

                        case 2:
                            ((QScrollArea *)item->ptr)->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
                            break;

                        case 1:
                        default:
                            ((QScrollArea *)item->ptr)->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
                            break;
                    }
                    QWidget *widget = ((QScrollArea *)item->ptr)->widget();
                    if (widget) {
                        GTKControl *item2 = Client->ControlsReversed[widget];
                        if ((item2) && ((item2->type == CLASS_TREEVIEW) || (item2->type == CLASS_ICONVIEW) || (item2->type == CLASS_TEXTVIEW))) {
                            if (((QScrollArea *)item->ptr)->horizontalScrollBarPolicy() != Qt::ScrollBarAlwaysOn)
                                ((QAbstractScrollArea *)widget)->setHorizontalScrollBarPolicy(((QScrollArea *)item->ptr)->horizontalScrollBarPolicy());
                            if (((QScrollArea *)item->ptr)->verticalScrollBarPolicy() != Qt::ScrollBarAlwaysOn)
                                ((QAbstractScrollArea *)widget)->setVerticalScrollBarPolicy(((QScrollArea *)item->ptr)->verticalScrollBarPolicy());
                        }
                    }
                    property_set = true;
                    break;
            }
            break;

        case P_OBEYCHILD:
            switch (item->type) {
                case CLASS_ASPECTFRAME:
                    // not supported
                    property_set = true;
                    break;
            }
            break;

        case P_RATIO:
            switch (item->type) {
                case CLASS_ASPECTFRAME:
                    DERIVED_TEMPLATE_SET(item, ratio, PARAM->Value.ToFloat());
                    property_set = true;
                    break;
            }
            break;

        case P_SUBMENU:
            {
                GTKControl *item2 = Client->Controls[PARAM->Value.ToInt()];
                switch (item->type) {
                    case CLASS_IMAGEMENUITEM:
                    case CLASS_SEPARATORMENUITEM:
                    case CLASS_RADIOMENUITEM:
                    case CLASS_CHECKMENUITEM:
                    case CLASS_TEAROFFMENUITEM:
                    case CLASS_MENUITEM:
                        if ((item2) && (item2->type == CLASS_MENU))
                            ((QAction *)item->ptr2)->setMenu((QMenu *)item2->ptr);
                        property_set = true;
                        break;
                }
            }
            break;

        case P_ACCELKEY:
            {
                QString key = QString(PARAM->Value.c_str()).replace(QString(">"), QString("+"));
                key = key.replace(QString("<"), QString()).toUpper().replace(QString("CONTROL"), QString("Ctrl"));
                switch (item->type) {
                    case CLASS_TOOLBUTTON:
                    case CLASS_TOGGLETOOLBUTTON:
                    case CLASS_RADIOTOOLBUTTON:
                    case CLASS_IMAGEMENUITEM:
                    case CLASS_STOCKMENUITEM:
                    case CLASS_CHECKMENUITEM:
                    case CLASS_MENUITEM:
                    case CLASS_TEAROFFMENUITEM:
                    case CLASS_RADIOMENUITEM:
                    case CLASS_SEPARATORMENUITEM:
                        ((QAction *)item->ptr2)->setShortcut(QKeySequence(key));
                        property_set = true;
                        break;

                    case CLASS_BUTTON:
                    case CLASS_CHECKBUTTON:
                        ((QAbstractButton *)item->ptr)->setShortcut(QKeySequence(key));
                        property_set = true;
                        break;

                    case CLASS_RADIOBUTTON:
                        ((QRadioButton *)item->ptr)->setShortcut(QKeySequence(key));
                        property_set = true;
                        break;

                    default:
                        break;
                }
            }
            break;

        case P_RIGHTJUSTIFIED:
            {
                switch (item->type) {
                    case CLASS_IMAGEMENUITEM:
                    case CLASS_SEPARATORMENUITEM:
                    case CLASS_RADIOMENUITEM:
                    case CLASS_CHECKMENUITEM:
                    case CLASS_TEAROFFMENUITEM:
                    case CLASS_MENUITEM:
                        // not supported
                        property_set = true;
                        break;
                }
            }
            break;

        case P_DRAWASRADIO:
            {
                switch (item->type) {
                    case CLASS_RADIOMENUITEM:
                    case CLASS_CHECKMENUITEM:
                        // not supported
                        property_set = true;
                        break;
                }
            }
            break;

        case P_HANDLEPOSITION:
            // not supported
            property_set = true;
            break;

        case P_SNAPEDGE:
            if (item->type == CLASS_HANDLEBOX) {
                switch (PARAM->Value.ToInt()) {
                    case 0:
                        ((QDockWidget *)item->ptr)->setAllowedAreas(Qt::LeftDockWidgetArea);
                        break;

                    case 1:
                        ((QDockWidget *)item->ptr)->setAllowedAreas(Qt::RightDockWidgetArea);
                        break;

                    case 2:
                        ((QDockWidget *)item->ptr)->setAllowedAreas(Qt::TopDockWidgetArea);
                        break;

                    case 3:
                        ((QDockWidget *)item->ptr)->setAllowedAreas(Qt::BottomDockWidgetArea);
                        break;

                    case 4:
                        ((QDockWidget *)item->ptr)->setAllowedAreas(Qt::NoDockWidgetArea);
                        break;

                    case 5:
                    default:
                        ((QDockWidget *)item->ptr)->setAllowedAreas(Qt::AllDockWidgetAreas);
                        break;
                }
                property_set = true;
            }
            break;

        case P_TOOLTIPS:
            // not needed
            property_set = true;
            break;

        case P_VISIBLEVERTICAL:
        case P_VISIBLEHORIZONTAL:
            // not supported
            property_set = true;
            break;

        case P_EXPAND:
            // not supported
            property_set = true;
            break;

        case P_SEARCHENTRY:
            {
                switch (item->type) {
                    case CLASS_TREEVIEW:
                        {
                            GTKControl *item2 = Client->Controls[PARAM->Value.ToInt()];

                            EventAwareWidget<QTreeWidget> *tree = (EventAwareWidget<QTreeWidget> *)item->ptr;
                            if (tree->wfriend) {
                                if (!Client->ControlsReversed[tree->wfriend])
                                    delete tree->wfriend;
                                tree->wfriend = NULL;
                            }

                            if ((!item2) || (item2->type != CLASS_EDIT))
                                break;

                            ((EventAwareWidget<QTreeWidget> *)item->ptr)->wfriend = (QWidget *)item2->ptr;

                            // set wfriend for edit too
                            ((EventAwareWidget<QTreeWidget> *)item2->ptr)->wfriend = (QWidget *)item->ptr;

                            ClientEventHandler *events = (ClientEventHandler *)item->events;
                            if (!events) {
                                events       = new ClientEventHandler(item->ID, Client);
                                item->events = events;
                            }

                            QObject::connect((QLineEdit *)item2->ptr, SIGNAL(textEdited(const QString &)), events, SLOT(on_textEdited(const QString &)));
                        }
                        property_set = true;
                        break;

                    case CLASS_TEXTVIEW:
                        {
                            QString     str  = QString::fromUtf8(PARAM->Value.c_str());
                            QStringList list = str.split(QRegularExpression("\\W+"), QString::SkipEmptyParts);

                            QTextCharFormat errorFormat;
                            errorFormat.setUnderlineColor(QColor(Qt::red));
                            errorFormat.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);

                            int size = list.size();
                            if (size) {
                                int        tag_id = list[0].toInt();
                                GTKControl *item2 = Client->Controls[tag_id];
                                if ((item2) && (item2->type == CLASS_TEXTTAG)) {
                                    QTextCharFormat *fmt = (QTextCharFormat *)item2->ptr2;
                                    errorFormat = *fmt;
                                }
                            }
                            for (int i = 1; i < size; i++) {
                                QString     s = list[i];
                                QTextCursor highlightCursor = ((QTextEdit *)item->ptr)->document()->find(s);
                                if (highlightCursor.isNull() == false) {
                                    QTextCharFormat highlightFormat;
                                    highlightFormat.setUnderlineColor(Qt::red);
                                    highlightFormat.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
                                    highlightCursor.mergeCharFormat(highlightFormat);
                                }
                            }
                        }
                        property_set = true;
                        break;
                }
            }
            break;

        case P_SEARCHCOLUMN:
            {
                switch (item->type) {
                    case CLASS_TREEVIEW:
                        ((EventAwareWidget<QTreeWidget> *)item->ptr)->ratio = PARAM->Value.ToInt();
                        property_set = true;
                        break;
                }
            }
            break;

        case P_ENABLESEARCH:
            // not supported, replaced by P_SEARCHENTRY
            property_set = true;
            break;

        case P_TREELINES:
            {
                switch (item->type) {
                    case CLASS_TREEVIEW:
                        ((QTreeWidget *)item->ptr)->setRootIsDecorated((bool)PARAM->Value.ToInt());
                        property_set = true;
                        break;
                }
            }
            break;

        case P_LEVELIDENTATION:
            {
                switch (item->type) {
                    case CLASS_TREEVIEW:
                        ((QTreeWidget *)item->ptr)->setIndentation(PARAM->Value.ToInt());
                        property_set = true;
                        break;
                }
            }
            break;

        case P_SHOWEXPANDERS:
            {
                switch (item->type) {
                    case CLASS_TREEVIEW:
                        ((QTreeWidget *)item->ptr)->setItemsExpandable((bool)PARAM->Value.ToInt());
                        property_set = true;
                        break;
                }
            }
            break;

        case P_MULTIPLESELECTION:
            {
                switch (item->type) {
                    case CLASS_TREEVIEW:
                        if (PARAM->Value.ToInt())
                            ((QTreeWidget *)item->ptr)->setSelectionMode(QAbstractItemView::ExtendedSelection);
                        else
                            ((QTreeWidget *)item->ptr)->setSelectionMode(QAbstractItemView::SingleSelection);
                        property_set = true;
                        break;

                    case CLASS_ICONVIEW:
                        if (PARAM->Value.ToInt())
                            ((QListWidget *)item->ptr)->setSelectionMode(QAbstractItemView::ExtendedSelection);
                        else
                            ((QListWidget *)item->ptr)->setSelectionMode(QAbstractItemView::SingleSelection);
                        property_set = true;
                        break;
                }
            }
            break;

        case P_SCROLLTOROW:
            {
                switch (item->type) {
                    case CLASS_TREEVIEW:
                        {
                            QTreeWidgetItem *element = ItemByPath((QTreeWidget *)item->ptr, PARAM->Value);
                            if (element)
                                ((QTreeWidget *)item->ptr)->scrollToItem(element);
                        }
                        property_set = true;
                        break;

                    case CLASS_ICONVIEW:
                        {
                            int index = PARAM->Value.ToInt();
                            if ((index >= 0) && (index < ((QListWidget *)item->ptr)->count())) {
                                QListWidgetItem *element = ((QListWidget *)item->ptr)->item(index);
                                if (element)
                                    ((QListWidget *)item->ptr)->scrollToItem(element);
                            }
                        }
                        property_set = true;
                        break;
                }
            }
            break;

        case P_SCROLLTOCOLUMN:
            {
                switch (item->type) {
                    case CLASS_TREEVIEW:
                        {
                            QTreeWidgetItem *element = ((QTreeWidget *)item->ptr)->currentItem();
                            if (element) {
                                QModelIndex index = computeModelIndex(element, PARAM->Value.ToInt());
                                if (index.isValid())
                                    ((QTreeWidget *)item->ptr)->scrollTo(index);
                            }
                        }
                        property_set = true;
                        break;
                }
            }
            break;

        case P_SELECTEDCOLUMN:
            {
                switch (item->type) {
                    case CLASS_TREEVIEW:
                        {
                            QTreeWidgetItem *element = ((QTreeWidget *)item->ptr)->currentItem();
                            if (element)
                                ((QTreeWidget *)item->ptr)->setCurrentItem(element, PARAM->Value.ToInt());
                        }
                        property_set = true;
                        break;
                }
            }
            break;

        case P_SELECT:
            {
                switch (item->type) {
                    case CLASS_TREEVIEW:
                        {
                            QTreeWidgetItem *element = ItemByPath((QTreeWidget *)item->ptr, PARAM->Value);
                            if (element)
                                ((QTreeWidget *)item->ptr)->setCurrentItem(element, ((QTreeWidget *)item->ptr)->currentColumn(), QItemSelectionModel::Select);
                        }
                        property_set = true;
                        break;

                    case CLASS_ICONVIEW:
                        {
                            int index = PARAM->Value.ToInt();
                            if ((index >= 0) && (index < ((QListWidget *)item->ptr)->count())) {
                                QListWidgetItem *element = ((QListWidget *)item->ptr)->item(index);
                                if (element)
                                    ((QListWidget *)item->ptr)->setCurrentItem(element, QItemSelectionModel::Select);
                            }
                        }
                        property_set = true;
                        break;
                }
            }
            break;

        case P_UNSELECT:
            {
                switch (item->type) {
                    case CLASS_TREEVIEW:
                        {
                            QTreeWidgetItem *element = ItemByPath((QTreeWidget *)item->ptr, PARAM->Value);
                            if (element)
                                ((QTreeWidget *)item->ptr)->setCurrentItem(element, ((QTreeWidget *)item->ptr)->currentColumn(), QItemSelectionModel::Deselect);
                        }
                        property_set = true;
                        break;

                    case CLASS_ICONVIEW:
                        {
                            int index = PARAM->Value.ToInt();
                            if ((index >= 0) && (index < ((QListWidget *)item->ptr)->count())) {
                                QListWidgetItem *element = ((QListWidget *)item->ptr)->item(index);
                                if (element)
                                    ((QListWidget *)item->ptr)->setCurrentItem(element, QItemSelectionModel::Deselect);
                            }
                        }
                        property_set = true;
                        break;
                }
            }
            break;

        case P_CURSOR:
            {
                switch (item->type) {
                    case CLASS_TREEVIEW:
                        {
                            QTreeWidgetItem *element = ItemByPath((QTreeWidget *)item->ptr, PARAM->Value);
                            ((QTreeWidget *)item->ptr)->setCurrentItem(element);
                        }
                        property_set = true;
                        break;

                    case CLASS_COMBOBOXTEXT:
                    case CLASS_COMBOBOX:
                        ((QComboBox *)item->ptr)->setCurrentIndex(PARAM->Value.ToInt());
                        property_set = true;
                        break;

                    case CLASS_ICONVIEW:
                        {
                            int index = PARAM->Value.ToInt();
                            QListWidgetItem *element = NULL;
                            if ((index >= 0) && (index < ((QListWidget *)item->ptr)->count()))
                                element = ((QListWidget *)item->ptr)->item(index);
                            ((QListWidget *)item->ptr)->setCurrentItem(element);
                        }
                        property_set = true;
                        break;

                    case CLASS_SCROLLEDWINDOW:
                        if (PARAM->Value.ToInt()) {
                            ((QScrollArea *)item->ptr)->verticalScrollBar()->setSliderPosition(((QScrollArea *)item->ptr)->verticalScrollBar()->maximum());
                            QWidget *child = ((QScrollArea *)item->ptr)->widget();
                            if (child) {
                                GTKControl *item2 = Client->ControlsReversed[child];
                                if ((item2) && (item2->type == CLASS_VIEWPORT)) {
                                    child = ((QScrollArea *)item2->ptr)->widget();
                                    item2 = Client->ControlsReversed[child];
                                }
                                if (item2) {
                                    switch (item2->type) {
                                        case CLASS_TREEVIEW:
                                        case CLASS_ICONVIEW:
                                        case CLASS_TEXTVIEW:
                                            {
                                                ClientEventHandler *events = (ClientEventHandler *)item2->events;
                                                if (!events) {
                                                    events        = new ClientEventHandler(item2->ID, Client);
                                                    item2->events = events;
                                                }

                                                QScrollBar *scrollbar = ((QAbstractScrollArea *)item2->ptr)->verticalScrollBar();
                                                QObject::disconnect(scrollbar, SIGNAL(rangeChanged(int, int)), events, SLOT(on_rangeChanged(int, int)));
                                                QObject::connect(scrollbar, SIGNAL(rangeChanged(int, int)), events, SLOT(on_rangeChanged(int, int)));

                                                scrollbar->setSliderPosition(scrollbar->maximum());
                                            }
                                            break;
                                    }
                                }
                            }
                        }
                        property_set = true;
                        break;
                }
            }
            break;

        case P_ADDTEAROFFS:
            // deprecated
            property_set = true;
            break;

        case P_FOCUSONCLICK:
            if (item->ptr) {
                if (PARAM->Value.ToInt())
                    ((QWidget *)item->ptr)->setFocusPolicy(Qt::ClickFocus);
                else
                    ((QWidget *)item->ptr)->setFocusPolicy(Qt::NoFocus);
            }
            property_set = true;
            break;

        case P_WRAPWIDTH:
            // not supported
            property_set = true;
            break;

        case P_ROWSPANCOLUMN:
            // not supported
            property_set = true;
            break;

        case P_DAY:
            {
                if (item->type == CLASS_CALENDAR) {
                    int   year, month, day;
                    QDate date = ((QCalendarWidget *)item->ptr)->selectedDate();
                    date.getDate(&year, &month, &day);
                    day = PARAM->Value.ToInt();
                    if (date.setDate(year, month, day))
                        ((QCalendarWidget *)item->ptr)->setSelectedDate(date);
                    property_set = true;
                }
            }
            break;

        case P_MONTH:
            {
                if (item->type == CLASS_CALENDAR) {
                    int   year, month, day;
                    QDate date = ((QCalendarWidget *)item->ptr)->selectedDate();
                    date.getDate(&year, &month, &day);
                    month = PARAM->Value.ToInt();
                    if (date.setDate(year, month, day))
                        ((QCalendarWidget *)item->ptr)->setSelectedDate(date);
                    property_set = true;
                }
            }
            break;

        case P_YEAR:
            {
                if (item->type == CLASS_CALENDAR) {
                    int   year, month, day;
                    QDate date = ((QCalendarWidget *)item->ptr)->selectedDate();
                    date.getDate(&year, &month, &day);
                    year = PARAM->Value.ToInt();
                    if (date.setDate(year, month, day))
                        ((QCalendarWidget *)item->ptr)->setSelectedDate(date);
                    property_set = true;
                }
            }
            break;

        case P_FONTNAME:
            if ((item->parent) && (((item->type == CLASS_TOOLBUTTON) || (item->type == CLASS_TOGGLETOOLBUTTON) || (item->type == CLASS_RADIOTOOLBUTTON)))) {
                GTKControl *parent = (GTKControl *)item->parent;
                QWidget    *widget = ((QToolBar *)parent->ptr)->widgetForAction((QAction *)item->ptr2);
                if (widget)
                    widget->setFont(fontDesc(widget->font(), PARAM->Value));
            } else
            if (item->ptr)
                ((QWidget *)item->ptr)->setFont(fontDesc(((QWidget *)item->ptr)->font(), PARAM->Value));
            else
            if (item->type == CLASS_TEXTTAG)
                ((QTextCharFormat *)item->ptr2)->setFont(fontDesc(((QTextCharFormat *)item->ptr2)->font(), PARAM->Value));
            else
            if (item->ptr2)
                ((QAction *)item->ptr2)->setFont(fontDesc(((QAction *)item->ptr2)->font(), PARAM->Value));
            property_set = true;
            break;

        case P_FONTFAMILY:
            if ((item->parent) && (((item->type == CLASS_TOOLBUTTON) || (item->type == CLASS_TOGGLETOOLBUTTON) || (item->type == CLASS_RADIOTOOLBUTTON)))) {
                GTKControl *parent = (GTKControl *)item->parent;
                QWidget    *widget = ((QToolBar *)parent->ptr)->widgetForAction((QAction *)item->ptr2);
                if (widget) {
                    QFont font = widget->font();
                    font.setFamily(QString::fromUtf8(PARAM->Value.c_str()));
                    widget->setFont(font);
                }
            } else
            if (item->ptr) {
                QFont font = ((QWidget *)item->ptr)->font();
                font.setFamily(QString::fromUtf8(PARAM->Value.c_str()));
                ((QWidget *)item->ptr)->setFont(font);
            } else
            if (item->type == CLASS_TEXTTAG) {
                QFont font = ((QTextCharFormat *)item->ptr2)->font();
                font.setFamily(QString::fromUtf8(PARAM->Value.c_str()));
                ((QTextCharFormat *)item->ptr2)->setFont(font);
            } else
            if (item->ptr2) {
                QFont font = ((QAction *)item->ptr2)->font();
                font.setFamily(QString::fromUtf8(PARAM->Value.c_str()));
                ((QAction *)item->ptr2)->setFont(font);
            }
            property_set = true;
            break;

        case P_FONTSTYLE:
            {
                QFont::Style style = QFont::StyleNormal;
                switch (PARAM->Value.ToInt()) {
                    case 1:
                        style = QFont::StyleOblique;
                        break;

                    case 2:
                        style = QFont::StyleItalic;
                        break;
                }
                if ((item->parent) && (((item->type == CLASS_TOOLBUTTON) || (item->type == CLASS_TOGGLETOOLBUTTON) || (item->type == CLASS_RADIOTOOLBUTTON)))) {
                    GTKControl *parent = (GTKControl *)item->parent;
                    QWidget    *widget = ((QToolBar *)parent->ptr)->widgetForAction((QAction *)item->ptr2);
                    if (widget) {
                        QFont font = widget->font();
                        font.setStyle(style);
                        widget->setFont(font);
                    }
                } else
                if (item->ptr) {
                    QFont font = ((QWidget *)item->ptr)->font();
                    font.setStyle(style);
                    ((QWidget *)item->ptr)->setFont(font);
                } else
                if (item->type == CLASS_TEXTTAG) {
                    QFont font = ((QTextCharFormat *)item->ptr2)->font();
                    font.setStyle(style);
                    ((QTextCharFormat *)item->ptr2)->setFont(font);
                } else
                if (item->ptr2) {
                    QFont font = ((QAction *)item->ptr2)->font();
                    font.setStyle(style);
                    ((QAction *)item->ptr2)->setFont(font);
                }
            }
            property_set = true;
            break;

        case P_FONTVARIANT:
            {
                QFont::Capitalization val = QFont::MixedCase;
                switch (PARAM->Value.ToInt()) {
                    case 1:
                        val = QFont::SmallCaps;
                        break;

                    case 2:
                        val = QFont::AllUppercase;
                        break;

                    case 3:
                        val = QFont::AllLowercase;
                        break;

                    case 4:
                        val = QFont::Capitalize;
                        break;
                }
                if ((item->parent) && (((item->type == CLASS_TOOLBUTTON) || (item->type == CLASS_TOGGLETOOLBUTTON) || (item->type == CLASS_RADIOTOOLBUTTON)))) {
                    GTKControl *parent = (GTKControl *)item->parent;
                    QWidget    *widget = ((QToolBar *)parent->ptr)->widgetForAction((QAction *)item->ptr2);
                    if (widget) {
                        QFont font = widget->font();
                        font.setCapitalization(val);
                        widget->setFont(font);
                    }
                } else
                if (item->ptr) {
                    QFont font = ((QWidget *)item->ptr)->font();
                    font.setCapitalization(val);
                    ((QWidget *)item->ptr)->setFont(font);
                } else
                if (item->type == CLASS_TEXTTAG) {
                    QFont font = ((QTextCharFormat *)item->ptr2)->font();
                    font.setCapitalization(val);
                    ((QTextCharFormat *)item->ptr2)->setFont(font);
                } else
                if (item->ptr2) {
                    QFont font = ((QAction *)item->ptr2)->font();
                    font.setCapitalization(val);
                    ((QAction *)item->ptr2)->setFont(font);
                }
            }
            property_set = true;
            break;

        case P_FONTWEIGHT:
            {
                if ((item->parent) && (((item->type == CLASS_TOOLBUTTON) || (item->type == CLASS_TOGGLETOOLBUTTON) || (item->type == CLASS_RADIOTOOLBUTTON)))) {
                    GTKControl *parent = (GTKControl *)item->parent;
                    QWidget    *widget = ((QToolBar *)parent->ptr)->widgetForAction((QAction *)item->ptr2);
                    if (widget) {
                        QFont font = widget->font();
                        font.setWeight(PARAM->Value.ToInt() / 10);
                        widget->setFont(font);
                    }
                } else
                if (item->ptr) {
                    QFont font = ((QWidget *)item->ptr)->font();
                    font.setWeight(PARAM->Value.ToInt() / 10);
                    ((QWidget *)item->ptr)->setFont(font);
                } else
                if (item->type == CLASS_TEXTTAG) {
                    QFont font = ((QTextCharFormat *)item->ptr2)->font();
                    font.setWeight(PARAM->Value.ToInt() / 10);
                    ((QTextCharFormat *)item->ptr2)->setFont(font);
                } else
                if (item->ptr2) {
                    QFont font = ((QAction *)item->ptr2)->font();
                    font.setWeight(PARAM->Value.ToInt() / 10);
                    ((QAction *)item->ptr2)->setFont(font);
                }
            }
            property_set = true;
            break;

        case P_FONTSTRETCH:
            {
                if ((item->parent) && (((item->type == CLASS_TOOLBUTTON) || (item->type == CLASS_TOGGLETOOLBUTTON) || (item->type == CLASS_RADIOTOOLBUTTON)))) {
                    GTKControl *parent = (GTKControl *)item->parent;
                    QWidget    *widget = ((QToolBar *)parent->ptr)->widgetForAction((QAction *)item->ptr2);
                    if (widget) {
                        QFont font = widget->font();
                        font.setStretch(PARAM->Value.ToInt());
                        widget->setFont(font);
                    }
                } else
                if (item->ptr) {
                    QFont font = ((QWidget *)item->ptr)->font();
                    font.setStretch(PARAM->Value.ToInt());
                    ((QWidget *)item->ptr)->setFont(font);
                } else
                if (item->type == CLASS_TEXTTAG) {
                    QFont font = ((QTextCharFormat *)item->ptr2)->font();
                    font.setStretch(PARAM->Value.ToInt());
                    ((QTextCharFormat *)item->ptr2)->setFont(font);
                } else
                if (item->ptr2) {
                    QFont font = ((QAction *)item->ptr2)->font();
                    font.setStretch(PARAM->Value.ToInt());
                    ((QAction *)item->ptr2)->setFont(font);
                }
            }
            property_set = true;
            break;

        case P_FONTSIZE:
            {
                if ((item->parent) && (((item->type == CLASS_TOOLBUTTON) || (item->type == CLASS_TOGGLETOOLBUTTON) || (item->type == CLASS_RADIOTOOLBUTTON)))) {
                    GTKControl *parent = (GTKControl *)item->parent;
                    QWidget    *widget = ((QToolBar *)parent->ptr)->widgetForAction((QAction *)item->ptr2);
                    if (widget) {
                        QFont font = widget->font();
                        font.setPointSize(PARAM->Value.ToInt());
                        widget->setFont(font);
                    }
                } else
                if (item->ptr) {
                    QFont font = ((QWidget *)item->ptr)->font();
                    font.setPointSize(PARAM->Value.ToInt());
                    ((QWidget *)item->ptr)->setFont(font);
                } else
                if (item->type == CLASS_TEXTTAG) {
                    QFont font = ((QTextCharFormat *)item->ptr2)->font();
                    font.setPointSize(PARAM->Value.ToInt());
                    ((QTextCharFormat *)item->ptr2)->setFont(font);
                } else
                if (item->ptr2) {
                    QFont font = ((QAction *)item->ptr2)->font();
                    font.setPointSize(PARAM->Value.ToInt());
                    ((QAction *)item->ptr2)->setFont(font);
                }
            }
            property_set = true;
            break;

        case P_PRIORITY:
            // deprecated
            property_set = true;
            break;

        case P_UNDERLINE:
            {
                if ((item->parent) && (((item->type == CLASS_TOOLBUTTON) || (item->type == CLASS_TOGGLETOOLBUTTON) || (item->type == CLASS_RADIOTOOLBUTTON)))) {
                    GTKControl *parent = (GTKControl *)item->parent;
                    QWidget    *widget = ((QToolBar *)parent->ptr)->widgetForAction((QAction *)item->ptr2);
                    if (widget) {
                        QFont font = widget->font();
                        font.setUnderline((bool)PARAM->Value.ToInt());
                        widget->setFont(font);
                    }
                } else
                if (item->ptr) {
                    QFont font = ((QWidget *)item->ptr)->font();
                    font.setUnderline((bool)PARAM->Value.ToInt());
                    ((QWidget *)item->ptr)->setFont(font);
                } else
                if (item->type == CLASS_TEXTTAG) {
                    QFont font = ((QTextCharFormat *)item->ptr2)->font();
                    font.setUnderline((bool)PARAM->Value.ToInt());
                    ((QTextCharFormat *)item->ptr2)->setFont(font);
                } else
                if (item->ptr2) {
                    QFont font = ((QAction *)item->ptr2)->font();
                    font.setUnderline((bool)PARAM->Value.ToInt());
                    ((QAction *)item->ptr2)->setFont(font);
                }
            }
            property_set = true;
            break;

        case P_STRIKEOUT:
            {
                if ((item->parent) && (((item->type == CLASS_TOOLBUTTON) || (item->type == CLASS_TOGGLETOOLBUTTON) || (item->type == CLASS_RADIOTOOLBUTTON)))) {
                    GTKControl *parent = (GTKControl *)item->parent;
                    QWidget    *widget = ((QToolBar *)parent->ptr)->widgetForAction((QAction *)item->ptr2);
                    if (widget) {
                        QFont font = widget->font();
                        font.setStrikeOut((bool)PARAM->Value.ToInt());
                        widget->setFont(font);
                    }
                } else
                if (item->ptr) {
                    QFont font = ((QWidget *)item->ptr)->font();
                    font.setStrikeOut((bool)PARAM->Value.ToInt());
                    ((QWidget *)item->ptr)->setFont(font);
                } else
                if (item->type == CLASS_TEXTTAG) {
                    QFont font = ((QTextCharFormat *)item->ptr2)->font();
                    font.setStrikeOut((bool)PARAM->Value.ToInt());
                    ((QTextCharFormat *)item->ptr2)->setFont(font);
                } else
                if (item->ptr2) {
                    QFont font = ((QAction *)item->ptr2)->font();
                    font.setStrikeOut((bool)PARAM->Value.ToInt());
                    ((QAction *)item->ptr2)->setFont(font);
                }
            }
            property_set = true;
            break;

        case P_STRETCH:
            {
                if ((item->parent) && (((item->type == CLASS_TOOLBUTTON) || (item->type == CLASS_TOGGLETOOLBUTTON) || (item->type == CLASS_RADIOTOOLBUTTON)))) {
                    GTKControl *parent = (GTKControl *)item->parent;
                    QWidget    *widget = ((QToolBar *)parent->ptr)->widgetForAction((QAction *)item->ptr2);
                    if (widget) {
                        QFont font = widget->font();
                        font.setStretch(PARAM->Value.ToInt());
                        widget->setFont(font);
                    }
                } else
                if (item->ptr) {
                    QFont font = ((QWidget *)item->ptr)->font();
                    font.setStretch(PARAM->Value.ToInt());
                    ((QWidget *)item->ptr)->setFont(font);
                } else
                if (item->type == CLASS_TEXTTAG) {
                    QFont font = ((QTextCharFormat *)item->ptr2)->font();
                    font.setStretch(PARAM->Value.ToInt());
                    ((QTextCharFormat *)item->ptr2)->setFont(font);
                } else
                if (item->ptr2) {
                    QFont font = ((QAction *)item->ptr2)->font();
                    font.setStretch(PARAM->Value.ToInt());
                    ((QAction *)item->ptr2)->setFont(font);
                }
            }
            property_set = true;
            break;

        case P_SCALE:
        case P_RISE:
        case P_LANGUAGE:
        case P_BACKGROUNDFULL:
            // not implemented
            property_set = true;
            break;

        case P_FORECOLOR:
            if (item->type == CLASS_TEXTTAG) {
                ((QTextCharFormat *)item->ptr2)->setForeground(QBrush(doColor((unsigned int)PARAM->Value.ToFloat())));
                property_set = true;
            }
            break;

        case P_BACKCOLOR:
            if (item->type == CLASS_TEXTTAG) {
                ((QTextCharFormat *)item->ptr2)->setBackground(QBrush(doColor((unsigned int)PARAM->Value.ToFloat())));
                property_set = true;
            }
            break;

        case P_DIRECTION:
            if (item->type == CLASS_TEXTTAG) {
                switch (PARAM->Value.ToInt()) {
                    case 1:
                        ((QTextCharFormat *)item->ptr2)->setLayoutDirection(Qt::LeftToRight);
                        break;

                    case 2:
                        ((QTextCharFormat *)item->ptr2)->setLayoutDirection(Qt::RightToLeft);
                        break;

                    case 0:
                    default:
                        ((QTextCharFormat *)item->ptr2)->setLayoutDirection(Qt::LayoutDirectionAuto);
                }
                property_set = true;
            }
            break;

        case P_DISPOSE:
            {
                if ((PARAM->Value.ToInt()) && (item->ptr)) {
                    if (Client->LastObject) {
                        ((QWidget *)Client->LastObject)->setUpdatesEnabled(true);
                        Client->LastObject = NULL;
                    }
                    Dispose(PARAM->Owner, item);
                    return;
                }
            }
            break;

        case P_COLUMNS:
            if (item->type == CLASS_ICONVIEW) {
                int columns = PARAM->Value.ToInt();
                if (columns == 1) {
                    ((QListWidget *)item->ptr)->setViewMode(QListView::ListMode);
                    ((QListWidget *)item->ptr)->setWrapping(false);
                } else {
                    ((QListWidget *)item->ptr)->setWrapping(true);
                    ((QListWidget *)item->ptr)->setViewMode(QListView::IconMode);
                }
                property_set = true;
            }
            break;

        case P_PARENT:
            {
                int        ctrlid = PARAM->Value.ToInt();
                GTKControl *item2 = Client->Controls[ctrlid];
                if ((ctrlid > 0) && (item->ptr) && (item2) && (item2->ptr) && (item->ptr != item2->ptr)) {
                    ((QWidget *)item->ptr)->setParent((QWidget *)item2->ptr);
                    if (item2->ptr2)
                        ((QLayout *)item2->ptr2)->addWidget((QWidget *)item->ptr);
                } else {
                    if (item->ptr) {
                        ((QWidget *)item->ptr)->setParent(NULL);
#ifdef __APPLE__
                        if (item->type == CLASS_MENUBAR)
                            ((QMenuBar *)item->ptr)->setNativeMenuBar(true);
#endif
                    }
                }
                property_set = true;
            }
            break;

        case P_MOUSECURSOR:
            {
                if (item->ptr) {
                    GTKControl *item2 = Client->Controls[PARAM->Value.ToInt()];
                    if ((!item2) || (item2->type != CLASS_IMAGE))
                        break;

                    int cursor = PARAM->Value.ToInt();
                    switch (cursor) {
                        case 0:
                            ((QWidget *)item->ptr)->setCursor(Qt::CrossCursor);
                            break;

                        case 1:
                            ((QWidget *)item->ptr)->setCursor(Qt::ArrowCursor);
                            break;

                        case 2:
                        case 3:
                            ((QWidget *)item->ptr)->setCursor(Qt::UpArrowCursor);
                            break;

                        case 6:
                            ((QWidget *)item->ptr)->setCursor(Qt::SizeBDiagCursor);
                            break;

                        case 7:
                            ((QWidget *)item->ptr)->setCursor(Qt::SizeFDiagCursor);
                            break;

                        case 8:
                            ((QWidget *)item->ptr)->setCursor(Qt::SplitVCursor);
                            break;

                        case 9:
                            ((QWidget *)item->ptr)->setCursor(Qt::SplitVCursor);
                            break;

                        case 12:
                            ((QWidget *)item->ptr)->setCursor(Qt::ForbiddenCursor);
                            break;

                        case 29:
                            ((QWidget *)item->ptr)->setCursor(Qt::OpenHandCursor);
                            break;

                        case 30:
                            ((QWidget *)item->ptr)->setCursor(Qt::ClosedHandCursor);
                            break;

                        case 75:
                            ((QWidget *)item->ptr)->setCursor(Qt::WaitCursor);
                            break;

                        default:
                            if (cursor < 0)
                                ((QWidget *)item->ptr)->setCursor((Qt::CursorShape)(-cursor));
                    }
                    property_set = true;
                }
                break;
            }
            break;

        case P_MOUSECURSORIMAGE:
            {
                if (item->ptr) {
                    GTKControl *item2 = Client->Controls[PARAM->Value.ToInt()];
                    if ((!item2) || (item2->type != CLASS_IMAGE))
                        break;

                    const QPixmap *pmap = ((QLabel *)item2->ptr)->pixmap();
                    if (pmap) {
                        QCursor cursor(*pmap);
                        ((QWidget *)item->ptr)->setCursor(cursor);
                    }
                    property_set = true;
                }
                break;
            }
            break;

        case P_USERRESIZABLE:
            // deprecated
            property_set = true;
            break;

        case P_SCREEN:
            // TO DO
            property_set = true;
            break;

        case P_COOLSYSTEMWINDOW:
            {
#ifdef _WIN32

                /*QStringList s=QString(PARAM->Value.c_str()).replace(";", ":").replace(",", ":").split(":");
                   if (s.size()==4) {
                    ((QWidget *)item->ptr)->setAttribute(Qt::WA_NoSystemBackground);
                    QtWin::enableBlurBehindWindow((QWidget *)item->ptr, QRegion(s[0].toInt(), s[1].toInt(), s[2].toInt(), s[3].toInt()));
                   }*/
#endif
            }
            property_set = true;
            break;

        case P_PROPERTIES:
            if (item->type == CLASS_PROPERTIESBOX) {
                QtVariantPropertyManager *variantManager = (QtVariantPropertyManager *)item->ptr3;
                QtTreePropertyBrowser    *widget         = (QtTreePropertyBrowser *)item->ptr;
                item->flags = 1;
                variantManager->clear();

                QList<QtProperty *> properties = widget->properties();
                int len = properties.size();
                for (int i = 0; i < len; i++)
                    widget->removeProperty(properties[i]);

                QXmlStreamReader  reader(PARAM->Value.c_str());
                QtVariantProperty *pending = NULL;
                QStringList       list;
                QString           default_value;
                int pending_index = -1;

                int idx = 0;
                QMap<QString, QtProperty *> categories;
                while ((!reader.atEnd()) && (!reader.hasError())) {
                    QXmlStreamReader::TokenType token = reader.readNext();
                    if (token == QXmlStreamReader::StartDocument)
                        continue;

                    if (token == QXmlStreamReader::StartElement) {
                        if (reader.name() == "pr") {
                            pending = NULL;
                            continue;
                        }
                        if (reader.name() == "o") {
                            QString s = reader.readElementText();
                            list << s;
                            if (pending)
                                pending->setAttribute("enumNames", list);
                            if (pending_index == -1) {
                                if (s == default_value) {
                                    pending_index = list.size() - 1;
                                    pending->setValue(pending_index);
                                }
                            } else
                            if (pending >= 0)
                                pending->setValue(pending_index);
                        } else
                        if (reader.name() == "pp") {
                            QXmlStreamAttributes attr = reader.attributes();
                            pending = NULL;
                            idx++;
                            if (attr.hasAttribute(QLatin1String("n"))) {
                                if (attr.hasAttribute(QLatin1String("t"))) {
                                    int        stype       = attr.value("t").toInt();
                                    QString    cat         = attr.value("c").toString();
                                    QtProperty *target_cat = NULL;
                                    if (cat.size()) {
                                        target_cat = categories[cat];
                                        if (!target_cat) {
                                            target_cat      = variantManager->addProperty(QtVariantPropertyManager::groupTypeId(), cat);
                                            categories[cat] = target_cat;
                                            widget->addProperty(target_cat);
                                        }
                                    }

                                    int type = stype;
                                    if (type < 0)
                                        type *= -1;
                                    switch (type) {
                                        case 0x02:
                                        case 0x03:
                                            {
                                                list.clear();
                                                pending_index = -1;
                                                QtVariantProperty *topItem = variantManager->addProperty(QtVariantPropertyManager::enumTypeId(), attr.value("n").toString());
                                                topItem->propertyIndex = idx - 1;
                                                if (stype < 0)
                                                    topItem->setModified(true);
                                                topItem->setAttribute("enumNames", list);
                                                if (target_cat)
                                                    target_cat->addSubProperty(topItem);
                                                else
                                                    widget->addProperty(topItem);
                                                pending = topItem;
                                                if (attr.hasAttribute(QLatin1String("v")))
                                                    default_value = attr.value("v").toString();
                                                else
                                                    default_value = "";
                                            }
                                            break;

                                        case 0x04:
                                            {
                                                QtVariantProperty *topItem = variantManager->addProperty(QVariant::Color, attr.value("n").toString());
                                                topItem->propertyIndex = idx - 1;
                                                if (stype < 0)
                                                    topItem->setModified(true);
                                                if (attr.hasAttribute(QLatin1String("v"))) {
                                                    unsigned int color = attr.value("v").toString().toUInt();
                                                    topItem->setValue(doColor(color));
                                                }
                                                if (target_cat)
                                                    target_cat->addSubProperty(topItem);
                                                else
                                                    widget->addProperty(topItem);
                                            }
                                            break;

                                        case 0x05:
                                            {
                                                QtVariantProperty *topItem = variantManager->addProperty(QVariant::Font, attr.value("n").toString());
                                                topItem->propertyIndex = idx - 1;
                                                if (stype < 0)
                                                    topItem->setModified(true);
                                                if (attr.hasAttribute(QLatin1String("v"))) {
                                                    AnsiString desc((char *)attr.value("v").toString().toUtf8().constData());
                                                    topItem->setValue(fontDesc(QFont(), desc));
                                                }
                                                if (target_cat)
                                                    target_cat->addSubProperty(topItem);
                                                else
                                                    widget->addProperty(topItem);
                                            }
                                            break;

                                        case 0x06:
                                            // not supported
                                            break;

                                        case 0x09:
                                            {
                                                QtVariantProperty *topItem = variantManager->addProperty(QVariant::Bool, attr.value("n").toString());
                                                topItem->propertyIndex = idx - 1;
                                                if (stype < 0)
                                                    topItem->setModified(true);
                                                if (attr.hasAttribute(QLatin1String("v"))) {
                                                    topItem->setValue((bool)attr.value("v").toString().toInt());
                                                }
                                                if (target_cat)
                                                    target_cat->addSubProperty(topItem);
                                                else
                                                    widget->addProperty(topItem);
                                            }
                                            break;

                                        case 0x0A:
                                            {
                                                QtVariantProperty *topItem = variantManager->addProperty(QVariant::Date, attr.value("n").toString());
                                                topItem->propertyIndex = idx - 1;
                                                if (stype < 0)
                                                    topItem->setModified(true);
                                                if (attr.hasAttribute(QLatin1String("v")))
                                                    topItem->setValue(QDate::fromString(attr.value("v").toString()));
                                                if (target_cat)
                                                    target_cat->addSubProperty(topItem);
                                                else
                                                    widget->addProperty(topItem);
                                            }
                                            break;

                                        case 0x0B:
                                            {
                                                QtVariantProperty *topItem = variantManager->addProperty(QVariant::Time, attr.value("n").toString());
                                                topItem->propertyIndex = idx - 1;
                                                if (stype < 0)
                                                    topItem->setModified(true);
                                                if (attr.hasAttribute(QLatin1String("v")))
                                                    topItem->setValue(QTime::fromString(attr.value("v").toString()));
                                                if (target_cat)
                                                    target_cat->addSubProperty(topItem);
                                                else
                                                    widget->addProperty(topItem);
                                            }
                                            break;

                                        case 0x0C:
                                            {
                                                QtVariantProperty *topItem = variantManager->addProperty(QVariant::DateTime, attr.value("n").toString());
                                                topItem->propertyIndex = idx - 1;
                                                if (stype < 0)
                                                    topItem->setModified(true);
                                                if (attr.hasAttribute(QLatin1String("v")))
                                                    topItem->setValue(QDateTime::fromString(attr.value("v").toString()));
                                                if (target_cat)
                                                    target_cat->addSubProperty(topItem);
                                                else
                                                    widget->addProperty(topItem);
                                            }
                                            break;

                                        case 0x0D:
                                            {
                                                QtVariantProperty *topItem = variantManager->addProperty(QVariant::Int, attr.value("n").toString());
                                                topItem->propertyIndex = idx - 1;
                                                if (stype < 0)
                                                    topItem->setModified(true);
                                                if (attr.hasAttribute(QLatin1String("v")))
                                                    topItem->setValue(attr.value("v").toInt());
                                                if (target_cat)
                                                    target_cat->addSubProperty(topItem);
                                                else
                                                    widget->addProperty(topItem);
                                            }
                                            break;

                                        case 0x0E:
                                            {
                                                QtVariantProperty *topItem = variantManager->addProperty(QVariant::Double, attr.value("n").toString());
                                                topItem->propertyIndex = idx - 1;
                                                if (stype < 0)
                                                    topItem->setModified(true);
                                                if (attr.hasAttribute(QLatin1String("v")))
                                                    topItem->setValue(attr.value("v").toDouble());
                                                if (target_cat)
                                                    target_cat->addSubProperty(topItem);
                                                else
                                                    widget->addProperty(topItem);
                                            }
                                            break;

                                        case 0x00:
                                        case 0x01:
                                        case 0x07:
                                        case 0x08:
                                        default:
                                            {
                                                QtVariantProperty *topItem = variantManager->addProperty(QVariant::String, attr.value("n").toString());
                                                topItem->propertyIndex = idx - 1;
                                                if (stype < 0)
                                                    topItem->setModified(true);
                                                if (type == 0x07)
                                                    topItem->setEnabled(false);
                                                if (attr.hasAttribute(QLatin1String("v")))
                                                    topItem->setValue(attr.value("v").toString());
                                                if (target_cat)
                                                    target_cat->addSubProperty(topItem);
                                                else
                                                    widget->addProperty(topItem);
                                            }
                                            break;
                                    }
                                }
                            }
                        }
                    }
                }
                item->flags = 0;
                reader.clear();
            }
            break;

        case 10011:
#ifndef NO_WEBKIT
            if ((item->type == 1012) || (item->type == 1001)) {
                ((ManagedPage *)((QWebView *)item->ptr)->page())->ignoreNext = true;
                ((QWebView *)item->ptr)->setHtml(QString::fromUtf8(PARAM->Value.c_str()), QUrl(QString("file:///static/")));
                property_set = true;
            } else
#endif
            if ((item->type == 1002) && (PARAM->Value.ToInt())) {
                ((QMediaPlayer *)item->ptr2)->pause();
                property_set = true;
            }
            break;

        case 1000:
            if (item->type == 1015) {
                GTKControl *item2 = Client->Controls[PARAM->Value.ToInt()];
                if ((item2) && (item2->ptr) && (item2->type == CLASS_IMAGE)) {
                    item->ptr2 = item2->ptr;
                    if (!item2->minset)
                        ((QLabel *)item->ptr2)->setMinimumSize(600, 400);
                }
                property_set = true;
            } else
            if (item->type == 1016) {
                ((AudioObject *)item->ptr3)->SAMPLE_RATE = PARAM->Value.ToInt();
                property_set = true;
            }
            break;

        case 1004:
            if (item->type == 1015) {
                QCameraImageCapture *imageCapture = (QCameraImageCapture *)item->ptr3;
                QCamera             *camera       = (QCamera *)imageCapture->mediaObject();
                if (camera) {
                    if (PARAM->Value.ToInt())
                        camera->stop();
                    else
                        camera->start();
                }
                property_set = true;
            } else
            if (item->type == 1016) {
                int param_int = PARAM->Value.ToInt();
                if (PARAM->Value.ToInt() == -2) {
                    ((AudioObject *)item->ptr3)->Stop();
                } else {
                    if (((AudioObject *)item->ptr3)->DRM) {
                        ((AudioObject *)item->ptr3)->key = PARAM->Owner->LOCAL_PRIVATE_KEY;
                        ((AudioObject *)item->ptr3)->InitDecryption();
                    }
                    ((AudioObject *)item->ptr3)->Play();
                }
                property_set = true;
            }
            break;

        case 1005:
            if (item->type == 1015) {
                QCameraImageCapture *imageCapture = (QCameraImageCapture *)item->ptr3;
                imageCapture->setProperty("format", PARAM->Value.c_str());
                property_set = true;
            } else
            if (item->type == 1016) {
                ((AudioObject *)item->ptr3)->quality = PARAM->Value.ToInt();
                ((AudioObject *)item->ptr3)->quality_changed = true;
                property_set = true;
            }
            break;

        case 1031:
            if (item->type == 1015) {
                QCameraImageCapture *imageCapture = (QCameraImageCapture *)item->ptr3;

                QImageEncoderSettings encoder_setting = imageCapture->encodingSettings();
                encoder_setting.setCodec("image/jpeg");
                //encoder_setting.setQuality(QtMultimediaKit::NormalQuality);
                QSize resolution = encoder_setting.resolution();
                resolution.setWidth(PARAM->Value.ToInt());
                encoder_setting.setResolution(resolution);
                imageCapture->setEncodingSettings(encoder_setting);

                property_set = true;
            }
            break;

        case 1032:
            if (item->type == 1015) {
                QCameraImageCapture *imageCapture = (QCameraImageCapture *)item->ptr3;

                QImageEncoderSettings encoder_setting = imageCapture->encodingSettings();
                encoder_setting.setCodec("image/jpeg");
                //encoder_setting.setQuality(QtMultimediaKit::NormalQuality);
                QSize resolution = encoder_setting.resolution();
                resolution.setHeight(PARAM->Value.ToInt());
                encoder_setting.setResolution(resolution);
                imageCapture->setEncodingSettings(encoder_setting);

                property_set = true;
            }
            break;

        case 10001:
#ifndef NO_WEBKIT
            if ((item->type == 1012) || (item->type == 1001)) {
                ((ManagedPage *)((QWebView *)item->ptr)->page())->ignoreNext = true;
                ((QWebView *)item->ptr)->load(QString::fromUtf8(PARAM->Value.c_str()));
                property_set = true;
            } else
#endif
            if (item->type == 1017) {
                item->index  = PARAM->Value.ToInt();
                property_set = true;
            }
            break;

        case 10007:
#ifndef NO_WEBKIT
            if ((item->type == 1012) || (item->type == 1001)) {
                ((QWebView *)item->ptr)->page()->mainFrame()->evaluateJavaScript(QString::fromUtf8(PARAM->Value.c_str()));
                property_set = true;
            } else
#endif
            if ((item->type == 1002) && (PARAM->Value.ToInt())) {
                ((QMediaPlayer *)item->ptr2)->stop();

                if (item->ptr3) {
                    delete (QBuffer *)item->ptr3;
                }

                QBuffer *buffer = new QBuffer();
                ((QMediaPlayer *)item->ptr2)->setMedia(QMediaContent(), buffer);
                item->ptr3   = buffer;
                property_set = true;
            }
            break;

        case 10022:
#ifndef NO_WEBKIT
            if ((item->type == 1012) || (item->type == 1001)) {
                if (PARAM->Value.ToInt())
                    ((QWebView *)item->ptr)->reload();
                property_set = true;
            }
#endif
            break;

        case 10002:
#ifndef NO_WEBKIT
            if ((item->type == 1012) || (item->type == 1001)) {
                if (PARAM->Value.ToInt())
                    ((QWebView *)item->ptr)->stop();
                property_set = true;
            }
#endif
            break;

        case 10016:
#ifndef NO_WEBKIT
            if ((item->type == 1012) || (item->type == 1001)) {
                if (PARAM->Value.ToInt())
                    ((QWebView *)item->ptr)->forward();
                property_set = true;
            }
#endif
            break;

        case 10015:
#ifndef NO_WEBKIT
            if ((item->type == 1012) || (item->type == 1001)) {
                if (PARAM->Value.ToInt())
                    ((QWebView *)item->ptr)->back();
                property_set = true;
            } else
#endif
            if (item->type == 1012) {
                ((QMediaPlayer *)item->ptr2)->setPosition(PARAM->Value.ToInt());
                property_set = true;
            }
            break;

        case 10024:
#ifndef NO_WEBKIT
            if ((item->type == 1012) || (item->type == 1001)) {
                if (PARAM->Value.ToInt()) {
                    QPrinter     printer;
                    QPrintDialog dialog(&printer, (QWidget *)item->ptr);
                    dialogs++;
                    if (dialog.exec() == QDialog::Accepted)
                        ((QWebView *)item->ptr)->print(&printer);
                    dialogs--;
                }
                property_set = true;
            }
#endif
            break;

        case 10003:
#ifndef NO_WEBKIT
            if ((item->type == 1012) || (item->type == 1001)) {
                if (PARAM->Value.ToInt())
                    ((QWebView *)item->ptr)->page()->triggerAction(QWebPage::Copy);
                property_set = true;
            } else
#endif
            if (item->type == 1002) {
                ((QMediaPlayer *)item->ptr2)->setVolume(PARAM->Value.ToInt());
                property_set = true;
            }
            break;

        case 10004:
        case 10006:
            // delete is not supported in Qt
#ifndef NO_WEBKIT
            if ((item->type == 1012) || (item->type == 1001)) {
                if (PARAM->Value.ToInt())
                    ((QWebView *)item->ptr)->page()->triggerAction(QWebPage::Cut);
                property_set = true;
            } else
#endif
            if (item->type == 1002) {
                //QUrl url = QUrl::fromLocalFile(QString::fromUtf8(temp_dir)+QString::fromUtf8(PARAM->Value.c_str()));
                QUrl url;
                if (PARAM->Value.Pos("://") > 0)
                    url = PARAM->Value.c_str();
                else
                    url = QUrl::fromLocalFile(QString::fromUtf8(temp_dir) + QString::fromUtf8(PARAM->Value.c_str()));
                ((QMediaPlayer *)item->ptr2)->stop();
                ((QMediaPlayer *)item->ptr2)->setMedia(url);
                property_set = true;
            }
            break;

        case 10008:
            if ((item->type == 1002) && (PARAM->Value.ToInt())) {
                ((QMediaPlayer *)item->ptr2)->stop();
                ((QMediaPlayer *)item->ptr2)->setMedia(QUrl());
                property_set = true;
            }
            break;

        case 10009:
            if ((item->type == 1002) && (PARAM->Value.ToInt())) {
                ((QMediaPlayer *)item->ptr2)->play();
                property_set = true;
            }
            break;

        case 10010:
            if ((item->type == 1002) && (PARAM->Value.ToInt())) {
                ((QMediaPlayer *)item->ptr2)->pause();
                property_set = true;
            }
            break;

        case 10005:
#ifndef NO_WEBKIT
            if ((item->type == 1012) || (item->type == 1001)) {
                if (PARAM->Value.ToInt())
                    ((QWebView *)item->ptr)->page()->triggerAction(QWebPage::Paste);
                property_set = true;
            }
#endif
            break;

        case 10018:
#ifndef NO_WEBKIT
            if ((item->type == 1012) || (item->type == 1001)) {
                ManagedRequest *manager = (ManagedRequest *)((QWebView *)item->ptr)->page()->networkAccessManager();
                if (manager->caseSensitive)
                    ((QWebView *)item->ptr)->page()->findText(QString::fromUtf8(PARAM->Value.c_str()), QWebPage::FindCaseSensitively);
                else
                    ((QWebView *)item->ptr)->page()->findText(QString::fromUtf8(PARAM->Value.c_str()));
                property_set = true;
            }
#endif
            break;

        case 10019:
#ifndef NO_WEBKIT
            if ((item->type == 1012) || (item->type == 1001)) {
                ManagedRequest *manager = (ManagedRequest *)((QWebView *)item->ptr)->page()->networkAccessManager();
                if (manager->caseSensitive)
                    ((QWebView *)item->ptr)->page()->findText(QString::fromUtf8(PARAM->Value.c_str()), QWebPage::FindBackward | QWebPage::FindCaseSensitively);
                else
                    ((QWebView *)item->ptr)->page()->findText(QString::fromUtf8(PARAM->Value.c_str()), QWebPage::FindBackward);
                property_set = true;
            }
#endif
            break;

        case 10013:
#ifndef NO_WEBKIT
            if ((item->type == 1012) || (item->type == 1001)) {
                ManagedRequest *manager = (ManagedRequest *)((QWebView *)item->ptr)->page()->networkAccessManager();
                if (manager->caseSensitive)
                    ((QWebView *)item->ptr)->page()->findText(QString::fromUtf8(PARAM->Value.c_str()), QWebPage::HighlightAllOccurrences | QWebPage::FindCaseSensitively);
                else
                    ((QWebView *)item->ptr)->page()->findText(QString::fromUtf8(PARAM->Value.c_str()), QWebPage::HighlightAllOccurrences);
                property_set = true;
            }
#endif
            break;

        case 10014:
#ifndef NO_WEBKIT
            if ((item->type == 1012) || (item->type == 1001)) {
                ManagedRequest *manager = (ManagedRequest *)((QWebView *)item->ptr)->page()->networkAccessManager();
                if (manager->caseSensitive)
                    ((QWebView *)item->ptr)->page()->findText(QString::fromUtf8(PARAM->Value.c_str()), QWebPage::FindBackward | QWebPage::HighlightAllOccurrences | QWebPage::FindCaseSensitively);
                else
                    ((QWebView *)item->ptr)->page()->findText(QString::fromUtf8(PARAM->Value.c_str()), QWebPage::FindBackward | QWebPage::HighlightAllOccurrences);
                property_set = true;
            } else
#endif
            if (item->type == 1012) {
                ((QMediaPlayer *)item->ptr2)->setPosition(PARAM->Value.ToInt());
                property_set = true;
            }
            break;

        case 10017:
#ifndef NO_WEBKIT
            if ((item->type == 1012) || (item->type == 1001)) {
                if (PARAM->Value.ToInt())
                    ((QWebView *)item->ptr)->page()->findText(QString(""), QWebPage::HighlightAllOccurrences);
                property_set = true;
            }
#endif
            break;

        case 10012:
#ifndef NO_WEBKIT
            if ((item->type == 1012) || (item->type == 1001)) {
                if (PARAM->Value.ToInt())
                    ((QWebView *)item->ptr)->page()->triggerAction(QWebPage::SelectAll);
                property_set = true;
            } else
#endif
            if (item->type == 1012) {
                ((QMediaPlayer *)item->ptr2)->setPosition(PARAM->Value.ToInt());
                property_set = true;
            }
            break;

        case 10023:
#ifndef NO_WEBKIT
            if ((item->type == 1012) || (item->type == 1001)) {
                ((ManagedPage *)((QWebView *)item->ptr)->page())->responses[PARAM->Value.ToInt()] = true;
                property_set = true;
            }
#endif
            break;

        case 10026:
#ifndef NO_WEBKIT
            if ((item->type == 1012) || (item->type == 1001)) {
                ((ManagedPage *)((QWebView *)item->ptr)->page())->responses[PARAM->Value.ToInt()] = false;
                property_set = true;
            }
#endif
            break;

        case 10025:
#ifndef NO_WEBKIT
            if ((item->type == 1012) || (item->type == 1001)) {
                GTKControl *item2 = Client->Controls[PARAM->Value.ToInt()];
                if ((item2) && (item2->ptr)) {
                    QWidget       *parent = (QWidget *)item2->ptr;
                    QWebInspector *i      = new QWebInspector((QWebView *)item->ptr);
                    i->setPage(((QWebView *)item->ptr)->page());
                    i->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
                    if (item2->ptr2)
                        ((QLayout *)item2->ptr2)->addWidget(i);
                    i->show();
                }
                property_set = true;
            }
#endif
            break;

        case 10027:
#ifndef NO_WEBKIT
            if ((item->type == 1012) || (item->type == 1001)) {
                int        rid    = PARAM->Value.ToInt();
                GTKControl *item2 = Client->Controls[rid];
                if ((item2) && (item2->type == 1012)) {
                    //((QWebView *)item->ptr)->page()->setView((QWidget *)item2->ptr);
                    ((QWebView *)item->ptr)->setProperty("newwindow", rid);
                }
                property_set = true;
            }
#endif
            break;

        case 10021:
#ifndef NO_WEBKIT
            if ((item->type == 1012) || (item->type == 1001)) {
                ManagedRequest *manager = (ManagedRequest *)((QWebView *)item->ptr)->page()->networkAccessManager();
                if (manager)
                    manager->caseSensitive = (bool)PARAM->Value.ToInt();
                property_set = true;
            }
#endif
            break;

        case 10033:
#ifndef NO_WEBKIT
            if ((item->type == 1012) || (item->type == 1001)) {
                ManagedRequest *manager = (ManagedRequest *)((QWebView *)item->ptr)->page()->networkAccessManager();
                if (manager)
                    manager->cache.clear();
                property_set = true;
            }
#endif
            break;

        case 10042:
            if (item->type == 1002) {
                if (PARAM->Value.ToInt())
                    ((QVideoWidget *)item->ptr2)->setAspectRatioMode(Qt::IgnoreAspectRatio);
                else
                    ((QVideoWidget *)item->ptr2)->setAspectRatioMode(Qt::KeepAspectRatio);
                property_set = true;
            }
            break;

        case 1062:
        case 1063:
            if (item->type == 1019) {
                QStringList list = QString::fromUtf8(PARAM->Value.c_str()).split(QString(";"), QString::KeepEmptyParts);

                int row    = 0;
                int column = 0;
                int object = -1;

                if (list.size() > 0)
                    row = list[0].toInt();
                if (list.size() > 1)
                    column = list[1].toInt();
                if (prop_id == 1062) {
                    if (list.size() > 6)
                        object = list[6].toInt();
                } else {
                    if (list.size() > 2)
                        object = list[2].toInt();
                }

                GTKControl *item2 = Client->Controls[object];
                if ((item2) && (item2->ptr))
                    ((QTableWidget *)item->ptr)->setCellWidget(row, column, (QWidget *)item2->ptr);
                else
                    ((QTableWidget *)item->ptr)->removeCellWidget(row, column);
                property_set = true;
            }
            break;

        case 1064:
            if (item->type == 1019) {
                QTableWidgetItem *element = ((QTableWidget *)item->ptr)->takeHorizontalHeaderItem(PARAM->Value.ToInt());
                if (element)
                    delete element;
                property_set = true;
            }
            break;

        case 1065:
            if (item->type == 1019) {
                QStringList list  = QString::fromUtf8(PARAM->Value.c_str()).split(QString(";"), QString::KeepEmptyParts);
                int         row   = -1;
                int         count = 1;
                if (list.size())
                    row = list[0].toInt();
                if (list.size() > 1)
                    count = list[1].toInt();

                int len = ((QTableWidget *)item->ptr)->columnCount();

                for (int i = 0; i < count; i++) {
                    for (int column = 0; column < len; column++) {
                        QTableWidgetItem *element = ((QTableWidget *)item->ptr)->takeItem(row, column);
                        if (element)
                            delete element;
                    }
                    row++;
                }
                property_set = true;
            }
            break;

        case 1015:
            if (item->type == 1019) {
                ((QTableWidget *)item->ptr)->setStyleSheet(QString("QTableView { gridline-color:") + doColorString(PARAM->Value.ToInt()) + QString(";}"));
                property_set = true;
            }
            break;

        case 1016:
            if (item->type == 1019) {
                ((QTableWidget *)item->ptr)->setGridStyle((Qt::PenStyle)PARAM->Value.ToInt());
                property_set = true;
            }
            break;

        case 1017:
        case 1020:
            if (item->type == 1019) {
                QStringList list   = QString::fromUtf8(PARAM->Value.c_str()).split(QString(";"), QString::KeepEmptyParts);
                int         column = -1;

                int len = list.size();
                if (len)
                    column = list[0].toInt();

                QString s;
                for (int i = 1; i < len; i++) {
                    if (i > 1)
                        s += QString(";");
                    s += list[i];
                }

                QTableWidgetItem *header = ((QTableWidget *)item->ptr)->horizontalHeaderItem(column);
                if (header)
                    header->setText(s);
                property_set = true;
            }
            break;

        case 1018:
        case 1019:
            if (item->type == 1019) {
                QStringList list = QString::fromUtf8(PARAM->Value.c_str()).split(QString(";"), QString::KeepEmptyParts);
                int         row  = -1;

                int len = list.size();
                if (len)
                    row = list[0].toInt();

                QString s;
                for (int i = 1; i < len; i++) {
                    if (i > 1)
                        s += QString(";");
                    s += list[i];
                }

                QTableWidgetItem *header = ((QTableWidget *)item->ptr)->verticalHeaderItem(row);
                if (header)
                    header->setText(s);
                property_set = true;
            }
            break;

        case 1043:
        case 1042:
            if (item->type == 1019) {
                QStringList list = QString::fromUtf8(PARAM->Value.c_str()).split(QString(";"), QString::KeepEmptyParts);

                int row    = 0;
                int column = 0;
                int align  = -1;

                QString val;

                int len = list.size();
                if (list.size() > 0)
                    row = list[0].toInt();
                if (list.size() > 1)
                    column = list[1].toInt();
                if (prop_id == 1042) {
                    if (list.size() > 2)
                        align = list[2].toInt();

                    for (int i = 3; i < len; i++) {
                        if (i > 3)
                            val += QString(";");
                        val += list[i];
                    }
                } else {
                    //if (list.size()>2)
                    //    val = list[2];
                    for (int i = 2; i < len; i++) {
                        if (i > 2)
                            val += QString(";");
                        val += list[i];
                    }
                }

                QTableWidgetItem *cellitem = ((QTableWidget *)item->ptr)->item(row, column);
                if (cellitem) {
                    cellitem->setText(val);
                } else {
                    cellitem = new QTableWidgetItem(val);
                    ((QTableWidget *)item->ptr)->setItem(row, column, cellitem);
                    if (align > -1) {
                        // 1 = left
                        // 2 = right
                        // 3 = center
                        // 4 = jutify
                    }
                }
                switch (align) {
                    case 1:
                        cellitem->setTextAlignment(Qt::AlignLeft);
                        break;

                    case 2:
                        cellitem->setTextAlignment(Qt::AlignRight);
                        break;

                    case 3:
                        cellitem->setTextAlignment(Qt::AlignHCenter);
                        break;

                    case 4:
                        cellitem->setTextAlignment(Qt::AlignJustify);
                        break;
                }
                property_set = true;
            }
            break;

        case 1049:
            if (item->type == 1019) {
                QStringList list   = QString::fromUtf8(PARAM->Value.c_str()).split(QString(";"), QString::KeepEmptyParts);
                int         row    = -1;
                int         height = 0;

                int len = list.size();
                if (len)
                    row = list[0].toInt();

                if (len > 0)
                    height = list[1].toInt();

                QTableWidgetItem *e = ((QTableWidget *)item->ptr)->verticalHeaderItem(row);
                if (e) {
                    QSize size = e->sizeHint();
                    size.setHeight(height);
                    e->setSizeHint(size);
                }
                property_set = true;
            }
            break;

        case 1046:
            if (item->type == 1019) {
                ((QTableWidget *)item->ptr)->clear();
            }
            break;

        case 1050:
            if (item->type == 1019) {
                QStringList list  = QString::fromUtf8(PARAM->Value.c_str()).split(QString(";"), QString::KeepEmptyParts);
                int         row   = -1;
                int         width = 0;

                int len = list.size();
                if (len)
                    row = list[0].toInt();

                if (len > 0)
                    width = list[1].toInt();

                QTableWidgetItem *e = ((QTableWidget *)item->ptr)->horizontalHeaderItem(row);
                if (e) {
                    QSize size = e->sizeHint();
                    size.setWidth(width);
                    e->setSizeHint(size);
                }
                property_set = true;
            }
            break;

        case 1051:
            if (item->type == 1019) {
                int count = PARAM->Value.ToInt();
                int len   = ((QTableWidget *)item->ptr)->columnCount();

                for (int i = 0; i < count; i++)
                    ((QTableWidget *)item->ptr)->insertColumn(len++);

                property_set = true;
            }
            break;

        case 1052:
            if (item->type == 1019) {
                int count = PARAM->Value.ToInt();
                int len   = ((QTableWidget *)item->ptr)->rowCount();

                for (int i = 0; i < count; i++)
                    ((QTableWidget *)item->ptr)->insertRow(len++);

                property_set = true;
            }
            break;

        case 1053:
            if (item->type == 1019) {
                QStringList list     = QString::fromUtf8(PARAM->Value.c_str()).split(QString(";"), QString::KeepEmptyParts);
                int         position = 0;
                int         count    = 1;
                int         len      = list.size();
                if (len)
                    position = list[0].toInt();

                if (len > 0)
                    count = list[1].toInt();

                for (int i = 0; i < count; i++)
                    ((QTableWidget *)item->ptr)->insertColumn(position++);

                property_set = true;
            }
            break;

        case 1054:
            if (item->type == 1019) {
                QStringList list     = QString::fromUtf8(PARAM->Value.c_str()).split(QString(";"), QString::KeepEmptyParts);
                int         position = 0;
                int         count    = 1;
                int         len      = list.size();
                if (len)
                    position = list[0].toInt();

                if (len > 0)
                    count = list[1].toInt();

                for (int i = 0; i < count; i++)
                    ((QTableWidget *)item->ptr)->insertRow(position++);

                property_set = true;
            }
            break;

        case 1061:
            if (item->type == 1019) {
                QStringList list  = QString::fromUtf8(PARAM->Value.c_str()).split(QString(";"), QString::KeepEmptyParts);
                int         row_s = -1;
                int         row_e = -1;
                int         col_s = -1;
                int         col_e = -1;

                int len = list.size();
                if (len >= 4) {
                    row_s = list[0].toInt();
                    col_s = list[1].toInt();
                    row_e = list[2].toInt();
                    col_e = list[3].toInt();
                }

                QString s;
                for (int i = 4; i < len; i++) {
                    if (i > 4)
                        s += QString(";");
                    s += list[i];
                }

                AnsiString desc((char *)s.toUtf8().data());
                for (int i = row_s; i < row_e; i++) {
                    for (int j = col_s; j < col_e; j++) {
                        QTableWidgetItem *e = ((QTableWidget *)item->ptr)->item(i, j);
                        if (!e) {
                            e = new QTableWidgetItem(val);
                            ((QTableWidget *)item->ptr)->setItem(i, j, e);
                        }
                        e->setFont(fontDesc(e->font(), desc));
                    }
                }

                property_set = true;
            }
            break;

        case 1055:
            if (item->type == 1019) {
                QStringList list  = QString::fromUtf8(PARAM->Value.c_str()).split(QString(";"), QString::KeepEmptyParts);
                int         row_s = -1;
                int         row_e = -1;
                int         col_s = -1;
                int         col_e = -1;

                int len = list.size();
                if (len >= 4) {
                    row_s = list[0].toInt();
                    col_s = list[1].toInt();
                    row_e = list[2].toInt();
                    col_e = list[3].toInt();
                }

                QString s;
                for (int i = 4; i < len; i++) {
                    if (i > 4)
                        s += QString(";");
                    s += list[i];
                }

                QColor color = doColor(s.toUInt());
                for (int i = row_s; i < row_e; i++) {
                    for (int j = col_s; j < col_e; j++) {
                        QTableWidgetItem *e = ((QTableWidget *)item->ptr)->item(i, j);
                        if (!e) {
                            e = new QTableWidgetItem(val);
                            ((QTableWidget *)item->ptr)->setItem(i, j, e);
                        }
                        e->setBackground(color);
                    }
                }

                property_set = true;
            }
            break;

        case 1056:
            if (item->type == 1019) {
                QStringList list  = QString::fromUtf8(PARAM->Value.c_str()).split(QString(";"), QString::KeepEmptyParts);
                int         row_s = -1;
                int         row_e = -1;
                int         col_s = -1;
                int         col_e = -1;

                int len = list.size();
                if (len >= 4) {
                    row_s = list[0].toInt();
                    col_s = list[1].toInt();
                    row_e = list[2].toInt();
                    col_e = list[3].toInt();
                }

                QString s;
                for (int i = 4; i < len; i++) {
                    if (i > 4)
                        s += QString(";");
                    s += list[i];
                }

                QColor color = doColor(s.toUInt());
                for (int i = row_s; i < row_e; i++) {
                    for (int j = col_s; j < col_e; j++) {
                        QTableWidgetItem *e = ((QTableWidget *)item->ptr)->item(i, j);
                        if (!e) {
                            e = new QTableWidgetItem(val);
                            ((QTableWidget *)item->ptr)->setItem(i, j, e);
                        }
                        e->setForeground(QBrush(color));
                    }
                }

                property_set = true;
            }
            break;

        case 1041:
            if (item->type == 1019) {
                QStringList list = QString::fromUtf8(PARAM->Value.c_str()).split(QString(";"), QString::KeepEmptyParts);
                int         row  = -1;
                int         col  = -1;

                if (list.size() > 1) {
                    row = list[0].toInt();
                    col = list[1].toInt();
                }

                ((QTableWidget *)item->ptr)->setCurrentCell(row, col);
                property_set = true;
            }
            break;

        case 1039:
            if (item->type == 1019) {
                QStringList list  = QString::fromUtf8(PARAM->Value.c_str()).split(QString(";"), QString::KeepEmptyParts);
                int         row_s = -1;
                int         row_e = -1;
                int         col_s = -1;
                int         col_e = -1;

                int len = list.size();
                if (len >= 4) {
                    row_s = list[0].toInt();
                    col_s = list[1].toInt();
                    row_e = list[2].toInt();
                    col_e = list[3].toInt();
                }

                ((QTableWidget *)item->ptr)->setRangeSelected(QTableWidgetSelectionRange(row_s, col_s, row_e, col_e), true);
                property_set = true;
            }
            break;

        case P_FIXED:
            {
                switch (item->type) {
                    case CLASS_SCROLLEDWINDOW:
                    case CLASS_HANDLEBOX:
                        item->flags  = (bool)PARAM->Value.ToInt();
                        property_set = true;
                        break;
                }
            }
            break;

        case 1002:
            if (item->type == 1017) {
                int val = PARAM->Value.ToInt();
                // stop
                if ((item->ptr3) && (item->ptr2)) {
                    QCamera            *camera = (QCamera *)item->ptr2;
                    CameraFrameGrabber *widget = (CameraFrameGrabber *)item->ptr3;
                    camera->stop();
                    delete camera;
                    delete widget;
                    item->ptr2 = NULL;
                    item->ptr3 = NULL;
                }
                if (val != -2) {
                    QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
                    if (cameras.size()) {
                        int         capture_device_id = item->index;
                        QCameraInfo info;
                        if ((capture_device_id < 0) || (capture_device_id >= cameras.size())) {
                            info = QCameraInfo::defaultCamera();
                        } else
                            info = cameras[capture_device_id];

                        QCamera *camera = new QCamera(info);
                        camera->setCaptureMode(QCamera::CaptureVideo);

                        CameraFrameGrabber *widget = new CameraFrameGrabber(Client, item->ID);
                        camera->setViewfinder(widget);
                        item->ptr3 = widget;

                        item->ptr2 = camera;
                        camera->start();
                    } else
                        fprintf(stderr, "No camera available\n");
                }
                property_set = true;
            } else
            if (item->type == 1016) {
                int param_int = PARAM->Value.ToInt();
                if (PARAM->Value.ToInt() == -2) {
                    ((AudioObject *)item->ptr3)->Stop();
                } else {
                    if (((AudioObject *)item->ptr3)->DRM) {
                        ((AudioObject *)item->ptr3)->key = PARAM->Owner->LOCAL_PRIVATE_KEY;
                        ((AudioObject *)item->ptr3)->InitEncryption();
                    }
                    ((AudioObject *)item->ptr3)->Record(param_int);
                }
                property_set = true;
            }
            break;

        case P_BITRATE:
            if (item->type == 1016) {
                ((AudioObject *)item->ptr3)->bitrate = PARAM->Value.ToInt();
                ((AudioObject *)item->ptr3)->quality_changed = true;
                property_set = true;
            }
            break;
            
        case P_USECOMPRESSION:
            if (item->type == 1016) {
                ((AudioObject *)item->ptr3)->use_compression = PARAM->Value.ToInt();
                property_set = true;
            }
            break;

        case P_BANDWIDTH:
            if (item->type == 1016) {
                ((AudioObject *)item->ptr3)->bandwidth = PARAM->Value.ToInt();
                ((AudioObject *)item->ptr3)->quality_changed = true;
                property_set = true;
            }
            break;
            
        case P_ADDBUFFER2:
            if (item->type == 1016) {
                ((AudioObject *)item->ptr3)->AddBuffer(PARAM->Value.c_str(), PARAM->Value.Length(), 1);
                property_set = true;
            }
            break;
            
        case P_CLEAR:
            if (item->type == 1016) {
                ((AudioObject *)item->ptr3)->ClearBuffer();
                property_set = true;
            }
            break;
    }

    if (!property_set)
        SetCustomProperty(item, prop_id, &PARAM->Value);
}

void GetProperty(Parameters *PARAM, CConceptClient *Client, Parameters *OUT_PARAM) {
    GTKControl *item = Client->Controls[PARAM->Sender.ToInt()];

    if (!item) {
        if (OUT_PARAM) {
            OUT_PARAM->Sender = PARAM->Sender;
            OUT_PARAM->ID     = MSG_GET_PROPERTY;
            OUT_PARAM->Target = PARAM->Target;
            OUT_PARAM->Value  = (char *)"0";
        } else
            Client->SendMessageNoWait(PARAM->Sender, MSG_GET_PROPERTY, PARAM->Target, "0");
        return;
    }
    bool       done_property = false;
    AnsiString res;
    int        prop = PARAM->Target.ToInt();
    switch (prop) {
        case P_CAPTION:
            switch (item->type) {
                case CLASS_EDIT:
                    res           = (char *)((QLineEdit *)item->ptr)->text().toUtf8().data();
                    done_property = true;
                    break;

                case CLASS_TEXTVIEW:
                    res           = (char *)((QTextEdit *)item->ptr)->toPlainText().toUtf8().data();
                    done_property = true;
                    break;

                case CLASS_COMBOBOXTEXT:
                    res           = (char *)((QComboBox *)item->ptr)->currentText().toUtf8().data();
                    done_property = true;
                    break;
#ifndef NO_WEBKIT
                case 1012:
                case 1001:
                    res           = (char *)((QWebView *)item->ptr)->title().toUtf8().constData();
                    done_property = true;
                    break;
                case CLASS_HTMLSNAP:
                    res  = (char *)((QWebView *)item->ptr)->page()->mainFrame()->evaluateJavaScript(QString::fromUtf8("document.body.innerHTML")).toString().toUtf8().constData();
                    done_property = true;
                    break;
#endif
            }
            break;

        case P_HEIGHT:
            if (item->ptr)
                res = AnsiString((long)((QWidget *)item->ptr)->height());
            else
            if ((item->ptr2) && (item->parent)) {
                GTKControl *parent = (GTKControl *)item->parent;
                if ((parent) && (parent->type == CLASS_TOOLBAR)) {
                    QWidget *widget = ((QToolBar *)parent->ptr)->widgetForAction((QAction *)item->ptr2);
                    if (widget) {
                        AnsiString x((long)widget->height());
                        res = x;
                    }
                }
            }
            done_property = true;
            break;

        case P_WIDTH:
            if (item->ptr)
                res = AnsiString((long)((QWidget *)item->ptr)->width());
            else
            if ((item->ptr2) && (item->parent)) {
                GTKControl *parent = (GTKControl *)item->parent;
                if ((parent) && (parent->type == CLASS_TOOLBAR)) {
                    QWidget *widget = ((QToolBar *)parent->ptr)->widgetForAction((QAction *)item->ptr2);
                    if (widget) {
                        AnsiString x((long)widget->width());
                        res = x;
                    }
                }
            }
            done_property = true;
            break;

        case P_VISIBLE:
            if (item->ptr)
                res = AnsiString((long)((QWidget *)item->ptr)->isVisible());
            else
            if (item->ptr2)
                res = AnsiString((long)((QAction *)item->ptr)->isVisible());
            done_property = true;
            break;

        case P_ABSOLUTE:
            if (item->ptr) {
                QWidget *widget = ((QWidget *)item->ptr)->window();

                long cx = ((QWidget *)item->ptr)->mapToGlobal(((QWidget *)item->ptr)->window()->pos()).x();
                long cy = ((QWidget *)item->ptr)->mapToGlobal(((QWidget *)item->ptr)->window()->pos()).y();
                if (widget) {
                    cx -= widget->pos().x();
                    cy -= widget->pos().y();
                }
                AnsiString x(cx);
                x  += ",";
                x  += AnsiString(cy);
                res = x;
            } else
            if ((item->ptr2) && (item->parent)) {
                GTKControl *parent = (GTKControl *)item->parent;
                if ((parent) && (parent->type == CLASS_TOOLBAR)) {
                    QWidget *widget = ((QToolBar *)parent->ptr)->widgetForAction((QAction *)item->ptr2);
                    if (widget) {
                        AnsiString x((long)widget->mapToGlobal(widget->window()->pos()).x());
                        x  += ",";
                        x  += AnsiString((long)widget->mapToGlobal(widget->window()->pos()).y());
                        res = x;
                    }
                }
            }
            done_property = true;
            break;

        case P_LEFT:
            switch (item->type) {
                case CLASS_TOOLBUTTON:
                case CLASS_TOGGLETOOLBUTTON:
                case CLASS_RADIOTOOLBUTTON:
                    if ((item->ptr2) && (item->parent)) {
                        GTKControl *parent = (GTKControl *)item->parent;
                        if ((parent) && (parent->type == CLASS_TOOLBAR)) {
                            QWidget *widget = ((QToolBar *)parent->ptr)->widgetForAction((QAction *)item->ptr2);
                            if (widget) {
                                AnsiString x((long)widget->pos().x());
                                res = x;
                            }
                        }
                    }
                    break;

                default:
                    if (item->ptr) {
                        AnsiString x((long)((QWidget *)item->ptr)->pos().x());
                        res = x;
                    }
            }
            done_property = true;
            break;

        case P_TOP:
            if (item->ptr) {
                AnsiString x((long)((QWidget *)item->ptr)->pos().y());
                res = x;
            } else
            if ((item->ptr2) && (item->parent)) {
                GTKControl *parent = (GTKControl *)item->parent;
                if ((parent) && (parent->type == CLASS_TOOLBAR)) {
                    QWidget *widget = ((QToolBar *)parent->ptr)->widgetForAction((QAction *)item->ptr2);
                    if (widget) {
                        AnsiString x((long)widget->pos().y());
                        res = x;
                    }
                }
            }
            done_property = true;
            break;

        case P_CHECKED:
            if ((item->type == CLASS_CHECKBUTTON) || (item->type == CLASS_RADIOBUTTON))
                res = (long)((QAbstractButton *)item->ptr)->isChecked();
            else
            if ((item->type == CLASS_RADIOMENUITEM) || (item->type == CLASS_CHECKMENUITEM) || (item->type == CLASS_RADIOTOOLBUTTON) || (item->type == CLASS_TOGGLETOOLBUTTON))
                res = (long)((QAction *)item->ptr2)->isChecked();
            done_property = true;
            break;

        case P_INCONSISTENT:
            {
                switch (item->type) {
                    case CLASS_CHECKBUTTON:
                        if (((QCheckBox *)item->ptr)->checkState() == Qt::PartiallyChecked)
                            res = "1";
                        else
                            res = "0";
                        done_property = 1;
                        break;
                }
            }
            break;

        case P_SELECTIONSTART:
            {
                if (item->type == CLASS_TEXTVIEW) {
                    const QTextCursor cursor = ((QTextEdit *)item->ptr)->textCursor();
                    res           = AnsiString((long)cursor.anchor());
                    done_property = true;
                }
            }
            break;

        case P_SELECTIONLENGTH:
            {
                if (item->type == CLASS_TEXTVIEW) {
                    const QTextCursor cursor = ((QTextEdit *)item->ptr)->textCursor();
                    int delta = cursor.position() - cursor.anchor();
                    if (delta < 0)
                        delta *= -1;
                    res           = AnsiString((long)delta);
                    done_property = true;
                }
            }
            break;

        case P_PAGE:
            {
                switch (item->type) {
                    case CLASS_NOTEBOOK:
                        {
                            QWidget *page = ((QTabWidget *)item->ptr)->currentWidget();
                            if (page) {
                                GTKControl *item2 = Client->ControlsReversed[page];
                                if (item2)
                                    res = AnsiString((long)item2->index);
                                else
                                    res = AnsiString((long)((QTabWidget *)item->ptr)->currentIndex());
                            } else
                                res = AnsiString((long)((QTabWidget *)item->ptr)->currentIndex());
                        }
                        done_property = true;
                        break;
                }
            }
            break;

        case P_EXPANDED:
            {
                switch (item->type) {
                    case CLASS_EXPANDER:
                        {
                            QWidget *container = ((QWidget *)item->ptr)->findChild<QWidget *>("container");
                            if (container)
                                res = AnsiString(container->isVisible());
                        }
                        done_property = true;
                        break;
                }
            }
            break;

        case P_ISDETACHED:
            switch (item->type) {
                case CLASS_HANDLEBOX:
                    res           = AnsiString((long)((QDockWidget *)item->ptr)->isFloating());
                    done_property = true;
                    break;
            }
            break;

        case P_SELECTEDCOLUMN:
            switch (item->type) {
                case CLASS_TREEVIEW:
                    {
                        int col = ((QTreeWidget *)item->ptr)->currentColumn();
                        if (col < 0)
                            col = ((TreeData *)item->ptr3)->extra;
                        res = AnsiString((long)col);
                    }
                    done_property = true;
                    break;
            }
            break;

        case P_VALUE:
            {
                GTKControl *parent = (GTKControl *)item->parent;
                if ((parent) && (item->type == CLASS_ADJUSTMENT)) {
                    switch (parent->type) {
                        case CLASS_SPINBUTTON:
                            res = AnsiString((long)((QSpinBox *)parent->ptr)->value());
                            break;

                        case CLASS_VSCALE:
                        case CLASS_HSCALE:
                            res = AnsiString((long)((QSlider *)parent->ptr)->value());
                            break;

                        case CLASS_VSCROLLBAR:
                        case CLASS_HSCROLLBAR:
                            res = AnsiString((long)((QScrollBar *)parent->ptr)->value());
                            break;

                        default:
                            if (item->ptr)
                                res = AnsiString((long)((QScrollBar *)item->ptr)->pageStep());
                    }
                    done_property = true;
                }
            }
            break;

        case P_PAGEINCREMENT:
        case P_PAGESIZE:
            {
                GTKControl *parent = (GTKControl *)item->parent;
                if ((parent) && (item->type == CLASS_ADJUSTMENT)) {
                    switch (parent->type) {
                        case CLASS_VSCALE:
                        case CLASS_HSCALE:
                            res = AnsiString((long)((QSlider *)parent->ptr)->pageStep());
                            break;

                        case CLASS_VSCROLLBAR:
                        case CLASS_HSCROLLBAR:
                            res = AnsiString((long)((QScrollBar *)parent->ptr)->pageStep());
                            break;

                        default:
                            if (item->ptr)
                                res = AnsiString((long)((QScrollBar *)item->ptr)->pageStep());
                    }
                }
                done_property = true;
            }
            break;

        case P_LOWER:
            {
                GTKControl *parent = (GTKControl *)item->parent;
                if ((parent) && (item->type == CLASS_ADJUSTMENT)) {
                    switch (parent->type) {
                        case CLASS_SPINBUTTON:
                            res = AnsiString((long)((QSpinBox *)parent->ptr)->minimum());
                            break;

                        case CLASS_VSCALE:
                        case CLASS_HSCALE:
                            res = AnsiString((long)((QSlider *)parent->ptr)->minimum());
                            break;

                        case CLASS_VSCROLLBAR:
                        case CLASS_HSCROLLBAR:
                            res = AnsiString((long)((QScrollBar *)parent->ptr)->minimum());
                            break;

                        default:
                            if (item->ptr)
                                res = AnsiString((long)((QScrollBar *)item->ptr)->minimum());
                    }
                }
                done_property = true;
            }
            break;

        case P_UPPER:
            {
                GTKControl *parent = (GTKControl *)item->parent;
                if ((parent) && (item->type == CLASS_ADJUSTMENT)) {
                    switch (parent->type) {
                        case CLASS_SPINBUTTON:
                            res = AnsiString((long)((QSpinBox *)parent->ptr)->maximum());
                            break;

                        case CLASS_VSCALE:
                        case CLASS_HSCALE:
                            res = AnsiString((long)((QSlider *)parent->ptr)->maximum());
                            break;

                        case CLASS_VSCROLLBAR:
                        case CLASS_HSCROLLBAR:
                            res = AnsiString((long)((QScrollBar *)parent->ptr)->maximum());
                            break;

                        default:
                            if (item->ptr)
                                res = AnsiString((long)((QScrollBar *)item->ptr)->maximum());
                    }
                }
                done_property = true;
            }
            break;

        case P_INCREMENT:
            {
                GTKControl *parent = (GTKControl *)item->parent;
                if ((parent) && (item->type == CLASS_ADJUSTMENT)) {
                    switch (parent->type) {
                        case CLASS_SPINBUTTON:
                            res = AnsiString((long)((QSpinBox *)parent->ptr)->singleStep());
                            break;

                        case CLASS_VSCALE:
                        case CLASS_HSCALE:
                            res = AnsiString((long)((QSlider *)parent->ptr)->singleStep());
                            break;

                        case CLASS_VSCROLLBAR:
                        case CLASS_HSCROLLBAR:
                            res = AnsiString((long)((QScrollBar *)parent->ptr)->singleStep());
                            break;

                        default:
                            if (item->ptr)
                                res = AnsiString((long)((QScrollBar *)item->ptr)->singleStep());
                    }
                }
                done_property = true;
            }
            break;

        case P_SELECT:
            {
                switch (item->type) {
                    case CLASS_TREEVIEW:
                        {
                            QList<QTreeWidgetItem *> list = ((QTreeWidget *)item->ptr)->selectedItems();
                            int len = list.size();
                            if (len) {
                                for (int i = 0; i < len; i++) {
                                    QTreeWidgetItem *titem = list[i];
                                    if (i)
                                        res += ";";
                                    res += ItemToPath((QTreeWidget *)item->ptr, titem);
                                }
                            } else
                                res = (char *)"-1";
                        }
                        done_property = true;
                        break;

                    case CLASS_ICONVIEW:
                        {
                            QList<QListWidgetItem *> list = ((QListWidget *)item->ptr)->selectedItems();
                            int len = list.size();
                            if (len) {
                                for (int i = 0; i < len; i++) {
                                    QListWidgetItem *titem = list[i];
                                    if (i)
                                        res += ";";
                                    res += AnsiString((long)((QListWidget *)item->ptr)->row(titem));
                                }
                            } else
                                res = (char *)"-1";
                        }
                        done_property = true;
                        break;
                }
            }
            break;

        case P_CURSOR:
            {
                switch (item->type) {
                    case CLASS_TREEVIEW:
                        {
                            QTreeWidgetItem *titem = ((QTreeWidget *)item->ptr)->currentItem();
                            if (titem)
                                res = ItemToPath((QTreeWidget *)item->ptr, titem);
                            else
                                res = (char *)"-1";
                        }
                        done_property = true;
                        break;

                    case CLASS_COMBOBOXTEXT:
                    case CLASS_COMBOBOX:
                        res           = AnsiString((long)((QComboBox *)item->ptr)->currentIndex());
                        done_property = true;
                        break;

                    case CLASS_ICONVIEW:
                        res           = AnsiString((long)((QListWidget *)item->ptr)->currentRow());
                        done_property = true;
                        break;
                }
            }
            break;

        case P_DAY:
            {
                if (item->type == CLASS_CALENDAR) {
                    int   year, month, day;
                    QDate date = ((QCalendarWidget *)item->ptr)->selectedDate();
                    date.getDate(&year, &month, &day);
                    res           = AnsiString((long)day);
                    done_property = true;
                }
            }
            break;

        case P_MONTH:
            {
                if (item->type == CLASS_CALENDAR) {
                    int   year, month, day;
                    QDate date = ((QCalendarWidget *)item->ptr)->selectedDate();
                    date.getDate(&year, &month, &day);
                    res           = AnsiString((long)month);
                    done_property = true;
                }
            }
            break;

        case P_YEAR:
            {
                if (item->type == CLASS_CALENDAR) {
                    int   year, month, day;
                    QDate date = ((QCalendarWidget *)item->ptr)->selectedDate();
                    date.getDate(&year, &month, &day);
                    res           = AnsiString((long)year);
                    done_property = true;
                }
            }
            break;

        case P_SCREENSHOT:
            if (item->ptr) {
                QPixmap    image = QPixmap::grabWidget((QWidget *)item->ptr);
                QByteArray bytes;
                QBuffer    buffer(&bytes);
                buffer.open(QIODevice::WriteOnly);
                image.save(&buffer, "PNG");
                res.LoadBuffer(bytes.data(), bytes.size());
                done_property = true;
            }
            break;

        case P_SCREEN:
            {
                switch (item->type) {
                    case CLASS_FORM:
                        res           = "0";
                        done_property = true;
                        break;
                }
            }
            break;

        case P_POSITION:
            {
                switch (item->type) {
                    case CLASS_VPANED:
                    case CLASS_HPANED:
                        {
                            int        pos  = ((EventAwareWidget<QSplitter> *)item->ptr)->ratio;
                            QList<int> list = ((EventAwareWidget<QSplitter> *)item->ptr)->sizes();
                            if (list.size() > 1) {
                                int total = list[0] + list[1];
                                if (total) {
                                    int e = list[0];
                                    if (e)
                                        pos = e * 1000 / total;
                                }
                            }
                            res = AnsiString((long)pos);
                        }
                        done_property = true;
                        break;
                }
            }
            break;

        case P_CLIENTHANDLE:
            {
                if (item->ptr) {
                    ((QWidget *)item->ptr)->createWinId();
                    res           = AnsiString((long)((QWidget *)item->ptr)->winId());
                    done_property = true;
                }
            }
            break;

        case P_STYLE:
            {
                QString val;
                if (item->ptr)
                    val = ((QWidget *)item->ptr)->styleSheet();
                else
                if ((item->ptr2) && (item->parent)) {
                    GTKControl *parent = (GTKControl *)item->parent;
                    if ((parent) && (parent->type == CLASS_TOOLBAR)) {
                        QWidget *widget = ((QToolBar *)parent->ptr)->widgetForAction((QAction *)item->ptr2);
                        if (widget)
                            val = widget->styleSheet();
                    }
                }
                done_property = true;
                res           = (char *)val.toUtf8().constData();
            }
            break;

        case 10001:
#ifndef NO_WEBKIT
            if ((item->type == 1012) || (item->type == 1001)) {
                res           = (char *)((QWebView *)item->ptr)->url().toString().toUtf8().constData();
                done_property = true;
            }
#endif
            break;

        case 10011:
#ifndef NO_WEBKIT
            if ((item->type == 1012) || (item->type == 1001)) {
                res           = (char *)((QWebView *)item->ptr)->page()->mainFrame()->toHtml().toUtf8().constData();
                done_property = true;
            }
#endif
            break;

        case 10007:
#ifndef NO_WEBKIT
            if ((item->type == 1012) || (item->type == 1001)) {
                QString val = ((QWebView *)item->ptr)->page()->mainFrame()->evaluateJavaScript(QString::fromUtf8(PARAM->Value.c_str())).toString();
                res           = AnsiString((char *)val.toUtf8().constData());
                done_property = true;
            }
#endif
            break;

        case 10008:
#ifndef NO_WEBKIT
            if ((item->type == 1012) || (item->type == 1001)) {
                QAction *action = ((QWebView *)item->ptr)->page()->action(QWebPage::Forward);
                if ((action) && (action->isEnabled()))
                    res = (char *)"1";
                else
                    res = (char *)"0";
                done_property = true;
            }
#endif
            break;

        case 10009:
#ifndef NO_WEBKIT
            if ((item->type == 1012) || (item->type == 1001)) {
                QAction *action = ((QWebView *)item->ptr)->page()->action(QWebPage::Back);
                if ((action) && (action->isEnabled()))
                    res = (char *)"1";
                else
                    res = (char *)"0";
                done_property = true;
            }
#endif
            break;

        case P_PRESENTYOURSELF:
            if (item->ptr) {
                QList<QByteArray> list = ((QObject *)item->ptr)->dynamicPropertyNames();
                int len = list.size();
                res = "";
                for (int i = 0; i < len; i++) {
                    if (i)
                        res += "\n";
                    res += AnsiString((char *)list[i].constData());
                }
                done_property = true;
            }
            break;

        case 1002:
        case 1033:
            if (item->type == 1015) {
                ClientEventHandler *events = (ClientEventHandler *)item->events;
                if (events) {
                    QByteArray bytes;
                    QBuffer    buffer(&bytes);
                    buffer.open(QIODevice::WriteOnly);

                    QCameraImageCapture *imageCapture = (QCameraImageCapture *)item->ptr3;
                    QString             format        = imageCapture->property("format").toString();
                    if (!format.size())
                        format = "PNG";

                    events->img.save(&buffer, format.toUtf8().constData());
                    res.LoadBuffer(bytes.data(), bytes.size());
                }
                done_property = 1;
            }
            break;

        case 1003:
            if (item->type == 1015) {
                QCameraImageCapture *imageCapture = (QCameraImageCapture *)item->ptr3;
                res           = AnsiString((long)imageCapture->isReadyForCapture());
                done_property = 1;
            }
            break;

        case 10015:
            if (item->type == 1002) {
                res           = AnsiString((long)((QMediaPlayer *)item->ptr2)->media().canonicalResource().resolution().width());
                done_property = true;
            }
            break;

        case 10016:
            if (item->type == 1002) {
                res           = AnsiString((long)((QMediaPlayer *)item->ptr2)->media().canonicalResource().resolution().height());
                done_property = true;
            }
            break;

        case 10017:
            if (item->type == 1002) {
                res           = AnsiString((long)((QMediaPlayer *)item->ptr2)->isVideoAvailable());
                done_property = true;
            }
            break;

        case 10018:
            if (item->type == 1002) {
                res           = AnsiString((long)((QMediaPlayer *)item->ptr2)->isAudioAvailable());
                done_property = true;
            }
            break;

        case 10023:
            if (item->type == 1002) {
                QString val = ((QMediaPlayer *)item->ptr2)->metaData("Title").toString();
                res           = AnsiString((char *)val.toUtf8().constData());
                done_property = true;
            }
            break;

        case 10024:
            if (item->type == 1002) {
                QStringList val = ((QMediaPlayer *)item->ptr2)->metaData("Author").toStringList();
                if (val.size())
                    res = AnsiString((char *)val[0].toUtf8().constData());
                done_property = true;
            }
            break;

        case 10025:
            if (item->type == 1002) {
                QString val = ((QMediaPlayer *)item->ptr2)->metaData("Copyright").toString();
                res           = AnsiString((char *)val.toUtf8().constData());
                done_property = true;
            }
            break;

        case 10026:
            if (item->type == 1002) {
                QString val = ((QMediaPlayer *)item->ptr2)->metaData("Comment").toString();
                res           = AnsiString((char *)val.toUtf8().constData());
                done_property = true;
            }
            break;

        case 10027:
            if (item->type == 1002) {
                QString val = ((QMediaPlayer *)item->ptr2)->metaData("AlbumTitle").toString();
                res           = AnsiString((char *)val.toUtf8().constData());
                done_property = true;
            }
            break;

        case 10028:
            if (item->type == 1002) {
                int val = ((QMediaPlayer *)item->ptr2)->metaData("Year").toInt();
                res           = AnsiString((long)val);
                done_property = true;
            }
            break;

        case 10029:
            if (item->type == 1002) {
                int val = ((QMediaPlayer *)item->ptr2)->metaData("TrackNumber").toInt();
                res           = AnsiString((long)val);
                done_property = true;
            }
            break;

        case 10030:
            if (item->type == 1002) {
                QString val = ((QMediaPlayer *)item->ptr2)->metaData("Genre").toString();
                res           = AnsiString((char *)val.toUtf8().constData());
                done_property = true;
            }
            break;

        case 10031:
            if (item->type == 1002) {
                int val = ((QMediaPlayer *)item->ptr2)->position();
                res           = AnsiString((long)val);
                done_property = true;
            }
            break;

        case 10032:
            if (item->type == 1002) {
                int val = ((QMediaPlayer *)item->ptr2)->duration();
                if (val <= 0)
                    val = ((QMediaPlayer *)item->ptr2)->metaData("Duration").toInt();
                res           = AnsiString((long)val);
                done_property = true;
            }
            break;

        case 10033:
            if (item->type == 1002) {
                double val = ((QMediaPlayer *)item->ptr2)->metaData("Size").toReal();
                res           = AnsiString((double)val);
                done_property = true;
            }
            break;

        case 10035:
            if (item->type == 1002) {
                qreal val = ((QMediaPlayer *)item->ptr2)->metaData("VideoFrameRate").toReal();
                res           = AnsiString((double)val);
                done_property = true;
            }
            break;

        case 10036:
            if (item->type == 1002) {
                int val = ((QMediaPlayer *)item->ptr2)->position();
                res           = AnsiString((long)val);
                done_property = true;
            }
            break;

        case 10037:
            if (item->type == 1002) {
                QString val  = ((QMediaPlayer *)item->ptr2)->metaData("VideoCodec").toString();
                QString val2 = ((QMediaPlayer *)item->ptr2)->metaData("AudioCodec").toString();
                res           = AnsiString((char *)val.toUtf8().constData());
                res          += ";";
                res          += AnsiString((char *)val2.toUtf8().constData());
                done_property = true;
            }
            break;

        case 10038:
            if (item->type == 1002) {
                int val = ((QMediaPlayer *)item->ptr2)->metaData("VideoBitRate").toInt();
                res           = AnsiString((long)val);
                done_property = true;
            }
            break;

        case 1009:
            if (item->type == 1019) {
                QStringList list = QString::fromUtf8(PARAM->Owner->POST_STRING.c_str()).split(QString(";"), QString::KeepEmptyParts);
                if (list.size() > 1) {
                    int row    = list[0].toInt();
                    int column = list[1].toInt();

                    QTableWidgetItem *e = ((QTableWidget *)item->ptr)->item(row, column);
                    if (e)
                        res = AnsiString(e->text().toUtf8().data());
                }
                done_property = true;
            }
            break;

        case 1041:
            if (item->type == 1019) {
                QTableWidgetItem *e = ((QTableWidget *)item->ptr)->currentItem();
                if (e)
                    res = AnsiString((long)e->row()) + AnsiString(",") + AnsiString((long)e->column());
                done_property = true;
            }
            break;

        case 1039:
            if (item->type == 1019) {
                QList<QTableWidgetSelectionRange> ranges = ((QTableWidget *)item->ptr)->selectedRanges();
                int len = ranges.size();
                res = "";
                if (len > 0) {
                    for (int i = 0; i < len; i++) {
                        QTableWidgetSelectionRange range = ranges[i];
                        res += AnsiString((long)range.topRow());
                        res += ";";
                        res += AnsiString((long)range.leftColumn());
                        res += ";";
                        res += AnsiString((long)range.bottomRow());
                        res += ";";
                        res += AnsiString((long)range.rightColumn());
                        res += ";";
                    }
                }
                done_property = true;
            }
            break;
    }
    if (!done_property)
        GetCustomProperty(item, prop, Client, &res);

    if (OUT_PARAM) {
        OUT_PARAM->Sender = PARAM->Sender;
        OUT_PARAM->ID     = MSG_GET_PROPERTY;
        OUT_PARAM->Target = PARAM->Target;
        OUT_PARAM->Value  = res;
    } else
        Client->SendMessageNoWait(PARAM->Sender, MSG_GET_PROPERTY, PARAM->Target, res);
}

void SetClientEvent(Parameters *PARAM, CConceptClient *Client) {
    GTKControl *item = Client->Controls[PARAM->Sender.ToInt()];

    if (!item)
        return;

    int CLIENT_EVENT = PARAM->Target.ToInt();
    switch (CLIENT_EVENT) {
        case EVENT_ON_CLICKED:
            switch (item->type) {
                case CLASS_BUTTON:
                    {
                        ClientEventHandler *events = (ClientEventHandler *)item->events;
                        if (!events) {
                            events       = new ClientEventHandler(item->ID, Client);
                            item->events = events;
                        }
                        QObject::connect((QAbstractButton *)item->ptr, SIGNAL(clicked()), events, SLOT(on_clicked()));
                        if (PARAM->Value.Length() > 2) {
                            if (!PARAM->Owner->parser)
                                PARAM->Owner->parser = new CompensationParser(compensated_send_message, compensated_wait_message, PARAM->Owner);

                            QByteArray ba;
                            ba.append(PARAM->Value.c_str(), PARAM->Value.Length());
                            QByteArray ba2  = QByteArray::fromBase64(ba);
                            AnsiString temp = (char *)ba2.constData();

                            events->entry_point = PARAM->Owner->parser->Parse(temp.c_str(), temp.Length());
                        }
                    }
                    break;

                case CLASS_TOOLBUTTON:
                case CLASS_TOGGLETOOLBUTTON:
                case CLASS_RADIOTOOLBUTTON:
                    {
                        ClientEventHandler *events = (ClientEventHandler *)item->events;
                        if (!events) {
                            events       = new ClientEventHandler(item->ID, Client);
                            item->events = events;
                        }
                        QObject::connect((QAction *)item->ptr2, SIGNAL(triggered(bool)), events, SLOT(on_triggered(bool)));
                        //QObject::connect((QAbstractButton *)item->ptr, SIGNAL(clicked()), events, SLOT(on_clicked()));
                        if (PARAM->Value.Length() > 2) {
                            if (!PARAM->Owner->parser)
                                PARAM->Owner->parser = new CompensationParser(compensated_send_message, compensated_wait_message, PARAM->Owner);
                            events->entry_point = PARAM->Owner->parser->Parse(PARAM->Value.c_str(), PARAM->Value.Length());
                        }
                    }
                    break;

                case CLASS_LABEL:
                    {
                        ClientEventHandler *events = (ClientEventHandler *)item->events;
                        if (!events) {
                            events       = new ClientEventHandler(item->ID, Client);
                            item->events = events;
                        }
                        QObject::connect((QLabel *)item->ptr, SIGNAL(linkActivated(const QString &)), events, SLOT(on_linkActivated(const QString &)));
                        if (PARAM->Value.Length() > 2) {
                            if (!PARAM->Owner->parser)
                                PARAM->Owner->parser = new CompensationParser(compensated_send_message, compensated_wait_message, PARAM->Owner);

                            QByteArray ba;
                            ba.append(PARAM->Value.c_str(), PARAM->Value.Length());
                            QByteArray ba2  = QByteArray::fromBase64(ba);
                            AnsiString temp = (char *)ba2.constData();

                            events->entry_point = PARAM->Owner->parser->Parse(temp.c_str(), temp.Length());
                        }
                    }
                    break;
            }
            break;

        case EVENT_ON_ACTIVATE:
            switch (item->type) {
                case CLASS_EDIT:
                    {
                        ClientEventHandler *events = (ClientEventHandler *)item->events;
                        if (!events) {
                            events       = new ClientEventHandler(item->ID, Client);
                            item->events = events;
                        }
                        QObject::connect((QLineEdit *)item->ptr, SIGNAL(returnPressed()), events, SLOT(on_returnPressed()));
                        if (PARAM->Value.Length() > 2) {
                            if (!PARAM->Owner->parser)
                                PARAM->Owner->parser = new CompensationParser(compensated_send_message, compensated_wait_message, PARAM->Owner);

                            QByteArray ba;
                            ba.append(PARAM->Value.c_str(), PARAM->Value.Length());
                            QByteArray ba2  = QByteArray::fromBase64(ba);
                            AnsiString temp = (char *)ba2.constData();

                            events->entry_point = PARAM->Owner->parser->Parse(temp.c_str(), temp.Length());
                        }
                    }
                    break;

                case CLASS_COMBOBOXTEXT:
                    {
                        ClientEventHandler *events = (ClientEventHandler *)item->events;
                        if (!events) {
                            events       = new ClientEventHandler(item->ID, Client);
                            item->events = events;
                        }
                        QLineEdit *edit = ((QComboBox *)item->ptr)->lineEdit();
                        if (edit)
                            QObject::connect(edit, SIGNAL(returnPressed()), events, SLOT(on_returnPressed()));
                        if (PARAM->Value.Length() > 2) {
                            if (!PARAM->Owner->parser)
                                PARAM->Owner->parser = new CompensationParser(compensated_send_message, compensated_wait_message, PARAM->Owner);

                            QByteArray ba;
                            ba.append(PARAM->Value.c_str(), PARAM->Value.Length());
                            QByteArray ba2  = QByteArray::fromBase64(ba);
                            AnsiString temp = (char *)ba2.constData();

                            events->entry_point = PARAM->Owner->parser->Parse(temp.c_str(), temp.Length());
                        }
                    }
                    break;

                case CLASS_IMAGEMENUITEM:
                case CLASS_STOCKMENUITEM:
                case CLASS_CHECKMENUITEM:
                case CLASS_MENUITEM:
                case CLASS_TEAROFFMENUITEM:
                case CLASS_RADIOMENUITEM:
                case CLASS_SEPARATORMENUITEM:
                    {
                        ClientEventHandler *events = (ClientEventHandler *)item->events;
                        if (!events) {
                            events       = new ClientEventHandler(item->ID, Client);
                            item->events = events;
                        }
                        QObject::connect((QAction *)item->ptr2, SIGNAL(triggered(bool)), events, SLOT(on_activated(bool)));
                        if (PARAM->Value.Length() > 2) {
                            if (!PARAM->Owner->parser)
                                PARAM->Owner->parser = new CompensationParser(compensated_send_message, compensated_wait_message, PARAM->Owner);

                            QByteArray ba;
                            ba.append(PARAM->Value.c_str(), PARAM->Value.Length());
                            QByteArray ba2  = QByteArray::fromBase64(ba);
                            AnsiString temp = (char *)ba2.constData();

                            events->entry_point = PARAM->Owner->parser->Parse(temp.c_str(), temp.Length());
                        }
                    }
                    break;
            }
            break;

        case EVENT_ON_TOGGLED:
            switch (item->type) {
                case CLASS_CHECKBUTTON:
                case CLASS_RADIOBUTTON:
                    {
                        ClientEventHandler *events = (ClientEventHandler *)item->events;
                        if (!events) {
                            events       = new ClientEventHandler(item->ID, Client);
                            item->events = events;
                        }
                        QObject::connect((QAbstractButton *)item->ptr, SIGNAL(toggled(bool)), events, SLOT(on_toggled(bool)));
                    }
                    break;

                case CLASS_TOOLBUTTON:
                case CLASS_TOGGLETOOLBUTTON:
                case CLASS_RADIOTOOLBUTTON:
                case CLASS_IMAGEMENUITEM:
                case CLASS_STOCKMENUITEM:
                case CLASS_CHECKMENUITEM:
                case CLASS_MENUITEM:
                case CLASS_TEAROFFMENUITEM:
                case CLASS_RADIOMENUITEM:
                case CLASS_SEPARATORMENUITEM:
                    {
                        ClientEventHandler *events = (ClientEventHandler *)item->events;
                        if (!events) {
                            events       = new ClientEventHandler(item->ID, Client);
                            item->events = events;
                        }
                        QObject::connect((QAction *)item->ptr2, SIGNAL(toggled(bool)), events, SLOT(on_toggled(bool)));
                    }
                    break;
            }
            break;

        case EVENT_ON_RELEASED:
            if (item->type == CLASS_EDIT) {
                ClientEventHandler *events = (ClientEventHandler *)item->events;
                if (!events) {
                    events       = new ClientEventHandler(item->ID, Client);
                    item->events = events;
                }
                if (item->ptr2)
                    QObject::connect((QAction *)item->ptr2, SIGNAL(triggered(bool)), events, SLOT(on_released(bool)));
                if (item->ptr3)
                    QObject::connect((QAction *)item->ptr3, SIGNAL(triggered(bool)), events, SLOT(on_released_secondary(bool)));
            }
            break;

        case EVENT_ON_PRESSED:
            if (item->type == CLASS_EDIT) {
                ClientEventHandler *events = (ClientEventHandler *)item->events;
                if (!events) {
                    events       = new ClientEventHandler(item->ID, Client);
                    item->events = events;
                }
                if (item->ptr2)
                    QObject::connect((QAction *)item->ptr2, SIGNAL(triggered(bool)), events, SLOT(on_pressed(bool)));
                if (item->ptr3)
                    QObject::connect((QAction *)item->ptr3, SIGNAL(triggered(bool)), events, SLOT(on_pressed_secondary(bool)));
            }
            break;

        case EVENT_ON_SWITCHPAGE:
            if (item->type == CLASS_NOTEBOOK) {
                ClientEventHandler *events = (ClientEventHandler *)item->events;
                if (!events) {
                    events       = new ClientEventHandler(item->ID, Client);
                    item->events = events;
                }
                QObject::connect((QTabWidget *)item->ptr, SIGNAL(currentChanged(int)), events, SLOT(on_currentChanged(int)));
            }
            break;

        case EVENT_ON_ROWACTIVATED:
            if (item->type == CLASS_TREEVIEW) {
                ClientEventHandler *events = (ClientEventHandler *)item->events;
                if (!events) {
                    events       = new ClientEventHandler(item->ID, Client);
                    item->events = events;
                }
                QObject::connect((QTreeWidget *)item->ptr, SIGNAL(itemActivated(QTreeWidgetItem *, int)), events, SLOT(on_itemActivated(QTreeWidgetItem *, int)));
                if (PARAM->Value.Length() > 3) {
                    if (!PARAM->Owner->parser)
                        PARAM->Owner->parser = new CompensationParser(compensated_send_message, compensated_wait_message, PARAM->Owner);

                    QByteArray ba;
                    ba.append(PARAM->Value.c_str(), PARAM->Value.Length());
                    QByteArray ba2  = QByteArray::fromBase64(ba);
                    AnsiString temp = (char *)ba2.constData();

                    events->entry_point = PARAM->Owner->parser->Parse(temp.c_str(), temp.Length());
                }
            } else
            if (item->type == CLASS_ICONVIEW) {
                ClientEventHandler *events = (ClientEventHandler *)item->events;
                if (!events) {
                    events       = new ClientEventHandler(item->ID, Client);
                    item->events = events;
                }
                QObject::connect((QListWidget *)item->ptr, SIGNAL(itemActivated(QListWidgetItem *)), events, SLOT(on_itemActivated2(QListWidgetItem *)));
                if (PARAM->Value.Length() > 3) {
                    if (!PARAM->Owner->parser)
                        PARAM->Owner->parser = new CompensationParser(compensated_send_message, compensated_wait_message, PARAM->Owner);

                    QByteArray ba;
                    ba.append(PARAM->Value.c_str(), PARAM->Value.Length());
                    QByteArray ba2  = QByteArray::fromBase64(ba);
                    AnsiString temp = (char *)ba2.constData();

                    events->entry_point = PARAM->Owner->parser->Parse(temp.c_str(), temp.Length());
                }
            }
            break;

        case EVENT_ON_TIMER:
            {
                ClientEventHandler *events = (ClientEventHandler *)item->events;
                if (!events) {
                    events       = new ClientEventHandler(item->ID, Client);
                    item->events = events;
                }
                QTimer *timer = (QTimer *)events->timer;
                if (timer) {
                    if (timer->isActive())
                        timer->stop();
                    delete timer;
                    events->timer = NULL;
                }

                if (item->ptr)
                    timer = new QTimer((QObject *)item->ptr);
                else
                    timer = new QTimer((QObject *)item->ptr2);

                events->timer = timer;

                int ms = PARAM->Value.ToInt();
                if (ms >= 0) {
                    //timer->setSingleShot(true);
                    QObject::connect(timer, SIGNAL(timeout()), events, SLOT(on_timer()));
                    timer->start(ms);
                }
            }
            break;

        case EVENT_ON_ORIENTATIONCHANGED:
            switch (item->type) {
                case CLASS_TOOLBAR:
                    ClientEventHandler *events = (ClientEventHandler *)item->events;
                    if (!events) {
                        events       = new ClientEventHandler(item->ID, Client);
                        item->events = events;
                    }
                    QObject::connect((QToolBar *)item->ptr, SIGNAL(orientationChanged(Qt::Orientation)), events, SLOT(on_orientationChanged(Qt::Orientation)));
                    break;
            }
            break;

        case EVENT_ON_INSERTATCURSOR:
            switch (item->type) {
                case CLASS_EDIT:
                    {
                        ClientEventHandler *events = (ClientEventHandler *)item->events;
                        if (!events) {
                            events       = new ClientEventHandler(item->ID, Client);
                            item->events = events;
                        }
                        QObject::connect((QLineEdit *)item->ptr, SIGNAL(textEdited(const QString &)), events, SLOT(on_textEdited(const QString &)));
                    }
                    break;

                case CLASS_TEXTVIEW:
                    {
                        ClientEventHandler *events = (ClientEventHandler *)item->events;
                        if (!events) {
                            events       = new ClientEventHandler(item->ID, Client);
                            item->events = events;
                        }
                        QObject::connect((QTextEdit *)item->ptr, SIGNAL(textChanged()), events, SLOT(on_textChanged()));
                    }
                    break;
            }
            break;

        case EVENT_ON_SETANCHOR:
            switch (item->type) {
                case CLASS_EDIT:
                    {
                        ClientEventHandler *events = (ClientEventHandler *)item->events;
                        if (!events) {
                            events       = new ClientEventHandler(item->ID, Client);
                            item->events = events;
                        }
                        QObject::connect((QLineEdit *)item->ptr, SIGNAL(selectionChanged()), events, SLOT(on_selectionChanged()));
                    }
                    break;

                case CLASS_TEXTVIEW:
                    {
                        ClientEventHandler *events = (ClientEventHandler *)item->events;
                        if (!events) {
                            events       = new ClientEventHandler(item->ID, Client);
                            item->events = events;
                        }
                        QObject::connect((QTextEdit *)item->ptr, SIGNAL(selectionChanged()), events, SLOT(on_selectionChanged()));
                    }
                    break;
            }
            break;

        case EVENT_ON_VALUECHANGED:
            switch (item->type) {
                case CLASS_SPINBUTTON:
                    {
                        ClientEventHandler *events = (ClientEventHandler *)item->events;
                        if (!events) {
                            events       = new ClientEventHandler(item->ID, Client);
                            item->events = events;
                        }
                        QObject::connect((QSpinBox *)item->ptr, SIGNAL(valueChanged(int)), events, SLOT(on_valueChanged(int)));
                    }
                    break;

                case CLASS_ADJUSTMENT:
                    {
                        GTKControl *parent = (GTKControl *)item->parent;
                        if (parent) {
                            switch (parent->type) {
                                case CLASS_SPINBUTTON:
                                    {
                                        ClientEventHandler *events = (ClientEventHandler *)item->events;
                                        if (!events) {
                                            events       = new ClientEventHandler(item->ID, Client);
                                            item->events = events;
                                        }

                                        QObject::connect((QWidget *)parent->ptr, SIGNAL(valueChanged(int)), events, SLOT(on_valueChanged3(int)));
                                    }
                                    break;

                                case CLASS_TREEVIEW:
                                case CLASS_SCROLLEDWINDOW:
                                case CLASS_HSCROLLBAR:
                                case CLASS_VSCROLLBAR:
                                    {
                                        if (item->ptr) {
                                            ClientEventHandler *events = (ClientEventHandler *)item->events;
                                            if (!events) {
                                                events       = new ClientEventHandler(item->ID, Client);
                                                item->events = events;
                                            }

                                            QObject::connect((QWidget *)item->ptr, SIGNAL(valueChanged(int)), events, SLOT(on_valueChanged(int)));
                                        }
                                    }
                                    break;
                            }
                        }
                    }
                    break;
            }
            break;

        case EVENT_ON_CHANGED:
            {
                switch (item->type) {
                    case CLASS_ICONVIEW:
                        {
                            ClientEventHandler *events = (ClientEventHandler *)item->events;
                            if (!events) {
                                events       = new ClientEventHandler(item->ID, Client);
                                item->events = events;
                            }
                            QObject::connect((QListWidget *)item->ptr, SIGNAL(currentRowChanged(int)), events, SLOT(on_currentRowChanged(int)));
                        }
                        break;

                    case CLASS_ADJUSTMENT:
                        break;

                    case CLASS_COMBOBOXTEXT:
                    case CLASS_COMBOBOX:
                        {
                            ClientEventHandler *events = (ClientEventHandler *)item->events;
                            if (!events) {
                                events       = new ClientEventHandler(item->ID, Client);
                                item->events = events;
                            }
                            if (item->type == CLASS_COMBOBOXTEXT)
                                QObject::connect((QComboBox *)item->ptr, SIGNAL(editTextChanged(const QString &)), events, SLOT(on_currentIndexChanged(const QString &)));
                            else
                                QObject::connect((QComboBox *)item->ptr, SIGNAL(currentIndexChanged(int)), events, SLOT(on_currentIndexChanged(int)));
                        }
                        break;

                    case CLASS_EDIT:
                        {
                            ClientEventHandler *events = (ClientEventHandler *)item->events;
                            if (!events) {
                                events       = new ClientEventHandler(item->ID, Client);
                                item->events = events;
                            }
                            QObject::connect((QLineEdit *)item->ptr, SIGNAL(textChanged(const QString &)), events, SLOT(on_textChanged(const QString &)));
                        }
                        break;

                    case CLASS_PROPERTIESBOX:
                        {
                            ClientEventHandler *events = (ClientEventHandler *)item->events;
                            if (!events) {
                                events       = new ClientEventHandler(item->ID, Client);
                                item->events = events;
                            }
                            QObject::connect((QtVariantPropertyManager *)item->ptr3, SIGNAL(propertyChanged(QtProperty *)), events, SLOT(on_propertyChanged(QtProperty *)));
                        }
                        break;
                }
            }
            break;

        case EVENT_ON_CHILDATTACHED:
        case EVENT_ON_CHILDDETACHED:
            {
                switch (item->type) {
                    case CLASS_HANDLEBOX:
                        {
                            ClientEventHandler *events = (ClientEventHandler *)item->events;
                            if (!events) {
                                events       = new ClientEventHandler(item->ID, Client);
                                item->events = events;
                            }
                            QObject::connect((QDockWidget *)item->ptr, SIGNAL(on_topLevelChanged(bool)), events, SLOT(on_topLevelChanged(bool)));
                        }
                        break;
                }
            }
            break;

        case EVENT_ON_CURSORCHANGED:
            {
                switch (item->type) {
                    case CLASS_TREEVIEW:
                        {
                            ClientEventHandler *events = (ClientEventHandler *)item->events;
                            if (!events) {
                                events       = new ClientEventHandler(item->ID, Client);
                                item->events = events;
                            }
                            QObject::connect((QTreeWidget *)item->ptr, SIGNAL(currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)), events, SLOT(on_currentItemChanged(QTreeWidgetItem *, QTreeWidgetItem *)));
                        }
                        break;
                }
            }
            break;

        case EVENT_ON_HEADERCLICKED:
            {
                switch (item->type) {
                    case CLASS_TREEVIEW:
                        {
                            ClientEventHandler *events = (ClientEventHandler *)item->events;
                            if (!events) {
                                events       = new ClientEventHandler(item->ID, Client);
                                item->events = events;
                            }
                            QObject::connect(((QTreeWidget *)item->ptr)->header(), SIGNAL(sectionClicked(int)), events, SLOT(on_sectionClicked(int)));
                        }
                        break;
                }
            }
            break;

        case EVENT_ON_COLUMNRESIZED:
            {
                switch (item->type) {
                    case CLASS_TREEVIEW:
                        {
                            ClientEventHandler *events = (ClientEventHandler *)item->events;
                            if (!events) {
                                events       = new ClientEventHandler(item->ID, Client);
                                item->events = events;
                            }
                            QObject::connect(((QTreeWidget *)item->ptr)->header(), SIGNAL(sectionResized(int, int, int)), events, SLOT(on_columnResized(int, int, int)));
                        }
                        break;
                }
            }
            break;

        case EVENT_ON_ROWCOLLAPSED:
            {
                switch (item->type) {
                    case CLASS_TREEVIEW:
                        {
                            ClientEventHandler *events = (ClientEventHandler *)item->events;
                            if (!events) {
                                events       = new ClientEventHandler(item->ID, Client);
                                item->events = events;
                            }
                            QObject::connect((QTreeWidget *)item->ptr, SIGNAL(itemCollapsed(QTreeWidgetItem *)), events, SLOT(on_itemCollapsed(QTreeWidgetItem *)));
                        }
                        break;
                }
            }
            break;

        case EVENT_ON_ROWEXPANDED:
            {
                switch (item->type) {
                    case CLASS_TREEVIEW:
                        {
                            ClientEventHandler *events = (ClientEventHandler *)item->events;
                            if (!events) {
                                events       = new ClientEventHandler(item->ID, Client);
                                item->events = events;
                            }
                            QObject::connect((QTreeWidget *)item->ptr, SIGNAL(itemExpanded(QTreeWidgetItem *)), events, SLOT(on_itemExpanded(QTreeWidgetItem *)));
                        }
                        break;
                }
            }
            break;

        case EVENT_ON_STARTEDITING:
            switch (item->type) {
                case CLASS_TREEVIEW:
                    {
                        ClientEventHandler *events = (ClientEventHandler *)item->events;
                        if (!events) {
                            events       = new ClientEventHandler(item->ID, Client);
                            item->events = events;
                        }
                        QObject::connect((QTreeWidget *)item->ptr, SIGNAL(itemChanged(QTreeWidgetItem *, int)), events, SLOT(on_itemChanged(QTreeWidgetItem *, int)));
                    }
                    break;
            }
            break;

        case EVENT_ON_ENDEDITING:
            switch (item->type) {
                case CLASS_TREEVIEW:
                    {
                        ClientEventHandler *events = (ClientEventHandler *)item->events;
                        if (!events) {
                            events       = new ClientEventHandler(item->ID, Client);
                            item->events = events;
                        }
                        QObject::connect((QTreeWidget *)item->ptr, SIGNAL(itemChanged(QTreeWidgetItem *, int)), events, SLOT(on_itemChanged2(QTreeWidgetItem *, int)));
                    }
                    break;
            }
            break;

        case EVENT_ON_CANCELEDITING:
            switch (item->type) {
                case CLASS_TREEVIEW:
                    // not needed anymore
                    break;
            }
            break;

        case EVENT_ON_DAYSELECTED:
            {
                switch (item->type) {
                    case CLASS_CALENDAR:
                        {
                            ClientEventHandler *events = (ClientEventHandler *)item->events;
                            if (!events) {
                                events       = new ClientEventHandler(item->ID, Client);
                                item->events = events;
                            }
                            QObject::connect((QCalendarWidget *)item->ptr, SIGNAL(clicked(const QDate &)), events, SLOT(on_dayClicked(const QDate &)));
                        }
                        break;
                }
            }
            break;

        case EVENT_ON_DAYSELECTEDDBCLICK:
            {
                switch (item->type) {
                    case CLASS_CALENDAR:
                        {
                            ClientEventHandler *events = (ClientEventHandler *)item->events;
                            if (!events) {
                                events       = new ClientEventHandler(item->ID, Client);
                                item->events = events;
                            }
                            QObject::connect((QCalendarWidget *)item->ptr, SIGNAL(activated(const QDate &)), events, SLOT(on_dayDBClicked(const QDate &)));
                        }
                        break;
                }
            }
            break;

        case EVENT_ON_MONTHCHANGED:
        case EVENT_ON_NEXTMONTH:
        case EVENT_ON_PREVMONTH:
        case EVENT_ON_NEXTYEAR:
        case EVENT_ON_PREVYEAR:
            {
                switch (item->type) {
                    case CLASS_CALENDAR:
                        {
                            ClientEventHandler *events = (ClientEventHandler *)item->events;
                            if (!events) {
                                events       = new ClientEventHandler(item->ID, Client);
                                item->events = events;
                            }
                            QObject::connect((QCalendarWidget *)item->ptr, SIGNAL(currentPageChanged(int, int)), events, SLOT(on_monthChanged(int, int)));
                        }
                        break;
                }
            }
            break;

        case EVENT_ON_CHANGEVALUE:
            switch (item->type) {
                case CLASS_VSCALE:
                case CLASS_HSCALE:
                case CLASS_VSCROLLBAR:
                case CLASS_HSCROLLBAR:
                    {
                        ClientEventHandler *events = (ClientEventHandler *)item->events;
                        if (!events) {
                            events       = new ClientEventHandler(item->ID, Client);
                            item->events = events;
                        }
                        QObject::connect((QAbstractSlider *)item->ptr, SIGNAL(valueChanged(int)), events, SLOT(on_valueChanged2(int)));
                    }
                    break;
            }
            break;

        case EVENT_ON_MOVESLIDER:
            switch (item->type) {
                case CLASS_VSCALE:
                case CLASS_HSCALE:
                case CLASS_VSCROLLBAR:
                case CLASS_HSCROLLBAR:
                    {
                        ClientEventHandler *events = (ClientEventHandler *)item->events;
                        if (!events) {
                            events       = new ClientEventHandler(item->ID, Client);
                            item->events = events;
                        }
                        QObject::connect((QAbstractSlider *)item->ptr, SIGNAL(sliderMoved(int)), events, SLOT(on_sliderMoved(int)));
                    }
                    break;
            }
            break;

        case 350:
            switch (item->type) {
                case 1019:
                    {
                        ClientEventHandler *events = (ClientEventHandler *)item->events;
                        if (!events) {
                            events       = new ClientEventHandler(item->ID, Client);
                            item->events = events;
                        }
                        QObject::connect((QTableWidget *)item->ptr, SIGNAL(cellActivated(int, int)), events, SLOT(on_cellActivated(int, int)));
                    }
                    break;
                case 1016:
                    ((AudioObject *)item->ptr3)->on_buffer_full = true;
                    break;
                default:
                    REGISTER_EVENT(CLIENT_EVENT);
            }
            break;

        case 351:
            switch (item->type) {
                case 1019:
                    {
                        ClientEventHandler *events = (ClientEventHandler *)item->events;
                        if (!events) {
                            events       = new ClientEventHandler(item->ID, Client);
                            item->events = events;
                        }
                        QObject::connect((QTableWidget *)item->ptr, SIGNAL(cellChanged(int, int)), events, SLOT(on_cellChanged(int, int)));
                    }
                    break;

                default:
                    REGISTER_EVENT(CLIENT_EVENT);
            }
            break;

        case 360:
            switch (item->type) {
                case 1019:
                    {
                        ClientEventHandler *events = (ClientEventHandler *)item->events;
                        if (!events) {
                            events       = new ClientEventHandler(item->ID, Client);
                            item->events = events;
                        }
                        QObject::connect((QTableWidget *)item->ptr, SIGNAL(currentCellChanged(int, int, int, int)), events, SLOT(on_currentCellChanged(int, int, int, int)));
                    }
                    break;

                default:
                    REGISTER_EVENT(CLIENT_EVENT);
            }
            break;

        /*case 352:
            switch (item->type) {
                case 1012:
                case 1001:
                    {
                        ClientEventHandler *events=(ClientEventHandler *)item->events;
                        if (!events) {
                            events=new ClientEventHandler(item->ID, Client);
                            item->events=events;
                        }

                        QObject::connect(((QWebView *)item->ptr)->page(), SIGNAL(linkHovered(const QString &, const QString &, const QString &)), events, SLOT(on_linkHovered(const QString &, const QString &, const QString &)));
                    }
                    break;
            }
            break;*/
        case 354:
            switch (item->type) {
#ifndef NO_WEBKIT
                case 1012:
                case 1001:
                    {
                        ClientEventHandler *events = (ClientEventHandler *)item->events;
                        if (!events) {
                            events       = new ClientEventHandler(item->ID, Client);
                            item->events = events;
                        }

                        QObject::connect(((QWebView *)item->ptr)->page(), SIGNAL(linkHovered(const QString &, const QString &, const QString &)), events, SLOT(on_linkHovered(const QString &, const QString &, const QString &)));
                    }
                    break;
#endif
                case 1002:
                    {
                        ClientEventHandler *events = (ClientEventHandler *)item->events;
                        if (!events) {
                            events       = new ClientEventHandler(item->ID, Client);
                            item->events = events;
                        }

                        QObject::connect((QMediaPlayer *)item->ptr2, SIGNAL(error(QMediaPlayer::Error error)), events, SLOT(on_error(QMediaPlayer::Error error)));
                    }
                    break;
            }
            break;

        case 355:
            switch (item->type) {
#ifndef NO_WEBKIT
                case 1012:
                case 1001:
                    {
                        ClientEventHandler *events = (ClientEventHandler *)item->events;
                        if (!events) {
                            events       = new ClientEventHandler(item->ID, Client);
                            item->events = events;
                        }

                        QObject::connect(((QWebView *)item->ptr)->page(), SIGNAL(iconChanged()), events, SLOT(on_iconChanged()));
                    }
                    break;
#endif

                case 1002:
                    {
                        ClientEventHandler *events = (ClientEventHandler *)item->events;
                        if (!events) {
                            events       = new ClientEventHandler(item->ID, Client);
                            item->events = events;
                        }

                        QObject::connect((QMediaPlayer *)item->ptr2, SIGNAL(error(QMediaPlayer::Error error)), events, SLOT(on_error2(QMediaPlayer::Error error)));
                    }
                    break;
            }
            break;

        case 356:
            switch (item->type) {
#ifndef NO_WEBKIT
                case 1012:
                case 1001:
                    {
                        ClientEventHandler *events = (ClientEventHandler *)item->events;
                        if (!events) {
                            events       = new ClientEventHandler(item->ID, Client);
                            item->events = events;
                        }

                        QObject::connect(((QWebView *)item->ptr)->page(), SIGNAL(loadStarted()), events, SLOT(on_loadStarted2()));
                    }
                    break;
#endif
            }
            break;

        case 357:
            switch (item->type) {
#ifndef NO_WEBKIT
                case 1012:
                case 1001:
                    {
                        ClientEventHandler *events = (ClientEventHandler *)item->events;
                        if (!events) {
                            events       = new ClientEventHandler(item->ID, Client);
                            item->events = events;
                        }

                        QObject::connect(((QWebView *)item->ptr)->page(), SIGNAL(loadFinished(bool)), events, SLOT(on_loadFinished(bool)));
                    }
                    break;
#endif
            }
            break;

        case 358:
            switch (item->type) {
#ifndef NO_WEBKIT
                case 1012:
                case 1001:
                    {
                        ClientEventHandler *events = (ClientEventHandler *)item->events;
                        if (!events) {
                            events       = new ClientEventHandler(item->ID, Client);
                            item->events = events;
                        }

                        QObject::connect(((QWebView *)item->ptr)->page(), SIGNAL(loadProgress(int)), events, SLOT(on_loadProgress(int)));
                    }
                    break;
#endif

                case 1019:
                    {
                        ClientEventHandler *events = (ClientEventHandler *)item->events;
                        if (!events) {
                            events       = new ClientEventHandler(item->ID, Client);
                            item->events = events;
                        }
                        QObject::connect((QTableWidget *)item->ptr, SIGNAL(itemSelectionChanged()), events, SLOT(on_itemSelectionChanged()));
                    }
                    break;
            }
            break;

        case 359:
            switch (item->type) {
#ifndef NO_WEBKIT
                case 1012:
                case 1001:
                    {
                        ClientEventHandler *events = (ClientEventHandler *)item->events;
                        if (!events) {
                            events       = new ClientEventHandler(item->ID, Client);
                            item->events = events;
                        }

                        QObject::connect(((QWebView *)item->ptr)->page(), SIGNAL(loadStarted()), events, SLOT(on_loadStarted()));
                    }
                    break;
#endif
            }
            break;

        case 365:
            switch (item->type) {
#ifndef NO_WEBKIT
                case 1012:
                case 1001:
                    {
                        ClientEventHandler *events = (ClientEventHandler *)item->events;
                        if (!events) {
                            events       = new ClientEventHandler(item->ID, Client);
                            item->events = events;
                        }

                        QObject::connect(((QWebView *)item->ptr)->page(), SIGNAL(selectionChanged()), events, SLOT(on_selectionChanged2()));
                    }
                    break;
#endif
            }
            break;

        case 366:
            switch (item->type) {
#ifndef NO_WEBKIT
                case 1012:
                case 1001:
                    {
                        ClientEventHandler *events = (ClientEventHandler *)item->events;
                        if (!events) {
                            events       = new ClientEventHandler(item->ID, Client);
                            item->events = events;
                        }

                        QObject::connect(((QWebView *)item->ptr), SIGNAL(statusBarMessage(const QString &)), events, SLOT(on_statusBarMessage(const QString &)));
                    }
                    break;
#endif
            }
            break;

        case 367:
            switch (item->type) {
#ifndef NO_WEBKIT
                case 1012:
                case 1001:
                    {
                        ClientEventHandler *events = (ClientEventHandler *)item->events;
                        if (!events) {
                            events       = new ClientEventHandler(item->ID, Client);
                            item->events = events;
                        }

                        QObject::connect(((QWebView *)item->ptr), SIGNAL(titleChanged(const QString &)), events, SLOT(on_titleChanged(const QString &)));
                    }
                    break;
#endif
            }
            break;

        case 372:
            switch (item->type) {
#ifndef NO_WEBKIT
                case 1012:
                case 1001:
                    {
                        ClientEventHandler *events = (ClientEventHandler *)item->events;
                        if (!events) {
                            events       = new ClientEventHandler(item->ID, Client);
                            item->events = events;
                        }

                        QObject::connect(((QWebView *)item->ptr)->page(), SIGNAL(downloadRequested(const QNetworkRequest &)), events, SLOT(on_downloadRequested(const QNetworkRequest &)));
                    }
                    break;
#endif
            }
            break;

        case 353:
            switch (item->type) {
                case 1002:
                    {
                        ClientEventHandler *events = (ClientEventHandler *)item->events;
                        if (!events) {
                            events       = new ClientEventHandler(item->ID, Client);
                            item->events = events;
                        }

                        QObject::connect((QMediaPlayer *)item->ptr2, SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus status)), events, SLOT(on_mediaStatusChanged(QMediaPlayer::MediaStatus status)));
                    }
                    break;
            }
            break;

        case EVENT_ON_GRABFOCUS:
        case EVENT_ON_FOCUS:
        case EVENT_ON_FOCUSIN:
        case EVENT_ON_SETFOCUS:
            if (item->ptr) {
                if (((QWidget *)item->ptr)->focusPolicy() == Qt::NoFocus)
                    ((QWidget *)item->ptr)->setFocusPolicy(Qt::ClickFocus);
            }

        // no break here !
        default:
            REGISTER_EVENT(CLIENT_EVENT);
    }

    if (item->type >= 1000)
        SetCustomEvent(item, CLIENT_EVENT, &PARAM->Value);
}

void PutStream(Parameters *PARAM, CConceptClient *Client) {
    GTKControl *item = Client->Controls[PARAM->Sender.ToInt()];

    if (!item)
        return;

    switch (item->type) {
        case CLASS_IMAGE:
            {
                QMovie  *movie = NULL;
                QPixmap pixmap;
                if (PARAM->ID == MSG_PUT_SECONDARY_STREAM) {
                    /*AnsiString source=":/resources/";
                       source+=PARAM->Target;
                       source+=".png";
                       if (!pixmap.load(QString::fromUtf8(source.c_str())))
                        fprintf(stderr, "Unknown stock item: %s\n", PARAM->Target.c_str());*/
                    pixmap = StockIcon(PARAM->Target.c_str());
                    if (!pixmap.isNull()) {
                        int isize = PARAM->Value.ToInt();
                        switch (isize) {
                            case 0:
                            case 1:
                            case 2:
                                pixmap = pixmap.scaledToWidth(16, Qt::SmoothTransformation);
                                break;

                            case 3:
                                pixmap = pixmap.scaledToWidth(24, Qt::SmoothTransformation);
                                break;

                            case 6:
                                pixmap = pixmap.scaledToWidth(32, Qt::SmoothTransformation);
                                break;

                            default:
                                if (isize > 7)
                                    pixmap = pixmap.scaledToWidth(isize, Qt::SmoothTransformation);
                                else
                                    pixmap = pixmap.scaledToWidth(32, Qt::SmoothTransformation);
                        }
                    }
                } else
                    pixmap.loadFromData((const unsigned char *)PARAM->Value.c_str(), PARAM->Value.Length());

                GTKControl *parent = (GTKControl *)item->parent;
                if ((parent) && (parent->type == CLASS_BUTTON)) {
                    ((QAbstractButton *)parent->ptr)->setIcon(QIcon(pixmap));
                    ((QAbstractButton *)parent->ptr)->setIconSize(pixmap.rect().size());
                    //if ((((QAbstractButton *)parent->ptr)->minimumHeight()<pixmap.rect().size().height()) || (((QAbstractButton *)parent->ptr)->minimumWidth()<pixmap.rect().size().width()))
                    //    ((QAbstractButton *)parent->ptr)->setMinimumSize(pixmap.size());
                } else {
                    ((QLabel *)item->ptr)->setPixmap(pixmap);
                    ((QLabel *)item->ptr)->setMinimumSize(pixmap.size());

                    if (PARAM->ID != MSG_PUT_SECONDARY_STREAM) {
                        AnsiString ext = Ext(&PARAM->Target);
                        if ((ext == (char *)"gif") || (ext == (char *)"mng")) {
                            QBuffer *buffer = new QBuffer();
                            buffer->setData(PARAM->Value.c_str(), PARAM->Value.Length());
                            buffer->open(QIODevice::ReadOnly);

                            QMovie *movie = ((QLabel *)item->ptr)->movie();
                            if (movie) {
                                QBuffer *prev_buffer = (QBuffer *)movie->device();
                                delete movie;

                                if (prev_buffer)
                                    delete prev_buffer;
                            }
                            movie = new QMovie(buffer);
                            if ((movie->isValid()) && (movie->frameCount() > 1)) {
                                ((QLabel *)item->ptr)->setMovie(movie);
                                ((QLabel *)item->ptr)->setMinimumSize(movie->frameRect().size());
                                movie->start();
                            } else {
                                delete buffer;
                                delete movie;
                            }
                        }
                    }

                    //if ((!((QLabel *)item->ptr)->minimumHeight()) && (!((QLabel *)item->ptr)->minimumWidth()))

                    if ((item->ref) && (((GTKControl *)item->ref)->type == CLASS_NOTEBOOK)) {
                        GTKControl  *parent = (GTKControl *)item->ref;
                        NotebookTab *nt     = (NotebookTab *)parent->pages->Item(item->index);
                        if ((nt) && (nt->visible)) {
                            ((QTabWidget *)((GTKControl *)item->ref)->ptr)->setTabIcon(parent->AdjustIndex(nt), pixmap);
                        }
                    }
                }
            }
            break;
#ifndef NO_WEBKIT
        case 1001:
        case 1012:
            if (PARAM->ID == MSG_PUT_STREAM) {
                if (PARAM->Target == (char *)"zoom") {
                    double zoom = PARAM->Value.ToFloat();
                    ((QWebView *)item->ptr)->setZoomFactor(zoom);
                } else
                if (PARAM->Target == (char *)"editable") {
                    int val = PARAM->Value.ToInt();

                    QWebPage *page = ((QWebView *)item->ptr)->page();
                    if (page)
                        page->setContentEditable(val);
                } else
                if (PARAM->Target == (char *)"full-content-zoom") {
                    double zoom = PARAM->Value.ToFloat();
                    ((QWebView *)item->ptr)->setZoomFactor(zoom);
                } else
                if (PARAM->Target == (char *)"user-agent") {
                    ((ManagedPage *)((QWebView *)item->ptr)->page())->userAgent = QString::fromUtf8(PARAM->Value.c_str());
                }
            } else {
                ManagedRequest *manager = (ManagedRequest *)((QWebView *)item->ptr)->page()->networkAccessManager();
                if (manager) {
                    QString key = QString::fromUtf8(PARAM->Target.c_str());
                    manager->cache.remove(key);
                    manager->cache[key] = QByteArray(PARAM->Value.c_str(), PARAM->Value.Length());
                }
            }
            break;
#endif
        case 1002:
            if (PARAM->ID == MSG_PUT_STREAM) {
                QBuffer *buffer = (QBuffer *)item->ptr3;
                if (buffer) {
                    QByteArray arr = buffer->buffer();
                    arr.append(PARAM->Value.c_str(), PARAM->Value.ToInt());
                    buffer->setData(arr);
                    //buffer->writeData();
                }
            }
            break;

        default:
            break;
    }
}

void resizeColumnsToContents(QTreeWidget *treeWidget) {
    int cCols  = treeWidget->columnCount();
    int cItems = treeWidget->topLevelItemCount();

    QHeaderView *header = (QHeaderView *)treeWidget->header();

    //int indent = treeWidget->indentation();
    //QTreeWidgetItem *headeritem = treeWidget->headerItem();
    for (int col = 0; col < cCols; col++) {
        if (header->sectionResizeMode(col) == QHeaderView::Interactive) {
            //header->resizeSection(col, header->sectionWidth(col));

            /*int w = headeritem->text(col).size()*8;//header->sectionSizeHint(col);
               int w2 = w;
               for(int i = 0; i < cItems; i++ )
                w2 = qMax( w2, treeWidget->topLevelItem(i)->text(col).size()*8 + (col == 0 ? indent : 0));
               if (w2>w)
                header->resizeSection(col, w2+5);*/
            treeWidget->resizeColumnToContents(col);
        }
    }
}

void AdjustAlign(TreeColumn *tc, QTreeWidgetItem *titem, QString *xalign_ref = NULL, QString *yalign_ref = NULL) {
    AnsiString xalign;

    if (xalign_ref)
        xalign = (char *)xalign_ref->toUtf8().constData();
    else
        xalign = tc->Properties[P_XALIGN];
    int  align    = titem->textAlignment(tc->index);
    bool modified = false;
    if (xalign.Length()) {
        align &= ~Qt::AlignHorizontal_Mask;
        double d = xalign.ToFloat();
        if (d == -1)
            align |= Qt::AlignJustify;
        else
        if (d <= 0.1)
            align |= Qt::AlignLeft;
        else
        if (d >= 0.9)
            align |= Qt::AlignRight;
        else
            align |= Qt::AlignHCenter;
        modified = true;
    }

    AnsiString yalign;
    if (yalign_ref)
        yalign = (char *)yalign_ref->toUtf8().constData();
    else
        yalign = tc->Properties[P_YALIGN];

    if (yalign.Length()) {
        align &= Qt::AlignVertical_Mask;
        double d = yalign.ToFloat();

        if (d <= 0.1)
            align |= Qt::AlignTop;
        else
        if (d >= 0.9)
            align |= Qt::AlignBottom;
        else
            align |= Qt::AlignVCenter;
        modified = true;
    } else
        align |= Qt::AlignVCenter;
    if (modified)
        titem->setTextAlignment(tc->index, align);
}

void CreateTreeItem(GTKControl *item, Parameters *PARAM, CConceptClient *Client) {
    ((QTreeWidget *)item->ptr)->blockSignals(true);
    QTreeWidgetItem *titem = NULL;
    if (PARAM->ID == MSG_CUSTOM_MESSAGE6) {
        QTreeWidgetItem *olditem = ItemByPath((QTreeWidget *)item->ptr, PARAM->Value);
        if (olditem) {
            bool setCurrent = false;
            if (olditem == ((QTreeWidget *)item->ptr)->currentItem())
                setCurrent = true;

            QTreeWidgetItem *parent = olditem->parent();
            if (!parent)
                parent = ((QTreeWidget *)item->ptr)->invisibleRootItem();
            if ((parent /*==((QTreeWidget *)item->ptr)->invisibleRootItem()*/) && (!olditem->childCount())) {
                int itemIndex = parent->indexOfChild(olditem);
                QList<QTreeWidgetItem *> children = olditem->takeChildren();
                delete olditem;
                olditem = new QTreeWidgetItem((QTreeWidget *)0, ((TreeData *)item->ptr3)->columns);
                if (!children.isEmpty())
                    olditem->addChildren(children);
                parent->insertChild(itemIndex, olditem);
                if (setCurrent)
                    ((QTreeWidget *)item->ptr)->setCurrentItem(olditem);
            } else
                *olditem = QTreeWidgetItem((QTreeWidget *)0, ((TreeData *)item->ptr3)->columns);
        }
        titem = olditem;
        //titem = new QTreeWidgetItem((QTreeWidget*)0, ((TreeData *)item->ptr3)->columns);
    }

    if (!titem) {
        titem = new QTreeWidgetItem((QTreeWidget *)0, ((TreeData *)item->ptr3)->columns);
        if (PARAM->ID == MSG_CUSTOM_MESSAGE4) {
            int index = PARAM->Target.ToInt();

            /*QString path(PARAM->Value.c_str());
               QStringList list=path.split(QString(":"), QString::SkipEmptyParts);

               int len=list.size();
               QTreeWidgetItem *parent = ((QTreeWidget *)item->ptr)->invisibleRootItem();
               if ((len) && (parent)) {
                for (int i=0;i<len;i++) {
                    int sindex=list[i].toInt();

                    int ccount = parent->childCount();
                    if (sindex >= ccount) {
                        if (ccount > 0)
                            sindex = ccount-1;
                        else
                            break;
                    } else
                    if (sindex<0)
                        break;

                    parent = parent->child(sindex);
                }
               }*/
            QTreeWidgetItem *parent = ItemByPath((QTreeWidget *)item->ptr, PARAM->Value);
            if (parent) {
                if ((index < 0) || (index >= parent->childCount()))
                    parent->addChild(titem);
                else
                    parent->insertChild(index, titem);
            } else
                ((QTreeWidget *)item->ptr)->addTopLevelItem(titem);
        } else {
            //if ((PARAM->ID == MSG_CUSTOM_MESSAGE2) && (PARAM->Target.ToInt() == 0)) {
            QStringList list = QString::fromUtf8(PARAM->Value.c_str()).split(QString(":"), QString::SkipEmptyParts);

            int len = list.size() - 1;
            if (len > 0) {
                AnsiString path;
                for (int i = 0; i < len; i++) {
                    if (i)
                        path += ":";
                    path += (char *)list[i].toUtf8().constData();
                }
                QTreeWidgetItem *parent_item = ItemByPath((QTreeWidget *)item->ptr, path);
                if (parent_item) {
                    int index = list[len].toInt();
                    if ((index < 0) || (index >= parent_item->childCount()))
                        parent_item->addChild(titem);
                    else
                        parent_item->insertChild(index, titem);
                }
            } else {
                int index = PARAM->Value.ToInt();
                if ((index < 0) || (index >= ((QTreeWidget *)item->ptr)->topLevelItemCount()))
                    ((QTreeWidget *)item->ptr)->addTopLevelItem(titem);
                else
                    ((QTreeWidget *)item->ptr)->insertTopLevelItem(index, titem);
            }

            /*} else {
                int index=PARAM->Value.ToInt();
                if ((index<0) || (index>=((QTreeWidget *)item->ptr)->topLevelItemCount()))
                    ((QTreeWidget *)item->ptr)->addTopLevelItem(titem);
                else
                    ((QTreeWidget *)item->ptr)->insertTopLevelItem(index, titem);
               }*/
        }
    }

    int attrsize = ((TreeData *)item->ptr3)->attributes.size();
    //if (attrsize) {
    int size       = ((TreeData *)item->ptr3)->meta.size();
    int attr_index = 0;
    int len        = ((TreeData *)item->ptr3)->columns.size();

    TreeColumn *last_tc = NULL;
    for (int i = 0; i < size; i++) {
        TreeColumn *tc      = ((TreeData *)item->ptr3)->meta[i];
        int        editable = tc->type & 0x80;

        if (tc->type > -1)
            AdjustAlign(tc, titem);

        Qt::ItemFlags orig_flags = titem->flags();
        if (tc->type > 0x80)
            titem->setFlags(Qt::ItemIsEditable | Qt::ItemIsSelectable | titem->flags());

        if ((tc->index >= 0) && (tc->index == ((TreeData *)item->ptr3)->hint)) {
            int icols = titem->columnCount();
            for (int j = 0; j < icols; j++) {
                titem->setToolTip(j, ((TreeData *)item->ptr3)->columns[tc->index]);
            }
        }

        switch (tc->type) {
            case -1:
                if (attr_index < attrsize) {
                    if ((tc->name == (char *)"background") || (tc->name == (char *)"cell-background")) {
                        QString background = ((TreeData *)item->ptr3)->attributes[attr_index];
                        if (background.size()) {
                            if (tc->index < 0) {
                                QBrush bcolor = QBrush(QColor(background));
                                for (int j = 0; j < len; j++) {
                                    titem->setBackground(j, bcolor);

                                    QWidget *widget = (QLabel *)((QTreeWidget *)item->ptr)->itemWidget(titem, j);
                                    if (widget) {
                                        widget->setAutoFillBackground(true);
                                        QPalette palette = widget->palette();
                                        palette.setColor(widget->backgroundRole(), QColor(background));
                                        widget->setPalette(palette);
                                    }
                                }
                            } else {
                                titem->setBackground(tc->index, QBrush(QColor(background)));
                                QWidget *widget = (QLabel *)((QTreeWidget *)item->ptr)->itemWidget(titem, tc->index);
                                if (widget) {
                                    widget->setAutoFillBackground(true);
                                    QPalette palette = widget->palette();
                                    palette.setColor(widget->backgroundRole(), QColor(background));
                                    widget->setPalette(palette);
                                }
                            }
                        }
                    } else
                    if ((tc->name == (char *)"foreground") || (tc->name == (char *)"cell-foreground")) {
                        QString background = ((TreeData *)item->ptr3)->attributes[attr_index];
                        if (background.size()) {
                            if (tc->index < 0) {
                                QBrush fcolor = QBrush(QColor(background));
                                for (int j = 0; j < len; j++) {
                                    titem->setForeground(j, fcolor);
                                    QWidget *widget = (QLabel *)((QTreeWidget *)item->ptr)->itemWidget(titem, j);
                                    if (widget) {
                                        widget->setAutoFillBackground(true);
                                        QPalette palette = widget->palette();
                                        palette.setColor(widget->foregroundRole(), QColor(background));
                                        widget->setPalette(palette);
                                    }
                                }
                            } else {
                                titem->setForeground(tc->index, QBrush(QColor(background)));
                                QWidget *widget = (QLabel *)((QTreeWidget *)item->ptr)->itemWidget(titem, tc->index);
                                if (widget) {
                                    widget->setAutoFillBackground(true);
                                    QPalette palette = widget->palette();
                                    palette.setColor(widget->foregroundRole(), QColor(background));
                                    widget->setPalette(palette);
                                }
                            }
                        }
                    }  else
                    if (tc->name == (char *)"font") {
                        QString font = ((TreeData *)item->ptr3)->attributes[attr_index];
                        if (font.size()) {
                            AnsiString font2 = (char *)font.toUtf8().constData();
                            if (tc->index < 0) {
                                for (int j = 0; j < len; j++) {
                                    QFont newfont = fontDesc(titem->font(j), font2);
                                    titem->setFont(j, newfont);
                                    QWidget *widget = (QLabel *)((QTreeWidget *)item->ptr)->itemWidget(titem, j);
                                    if (widget)
                                        widget->setFont(newfont);
                                }
                            } else {
                                QFont fontdesc = fontDesc(titem->font(tc->index), font2);
                                titem->setFont(tc->index, fontdesc);
                                QWidget *widget = (QLabel *)((QTreeWidget *)item->ptr)->itemWidget(titem, tc->index);
                                if (widget)
                                    widget->setFont(fontdesc);
                            }
                        }
                    } else
                    if (tc->name == (char *)"editable") {
                        int editable = ((TreeData *)item->ptr3)->attributes[attr_index].toInt();
                        if (editable)
                            titem->setFlags(Qt::ItemIsEditable | Qt::ItemIsSelectable | orig_flags);
                        else
                            titem->setFlags(orig_flags);
                        if (tc->index >= 0) {
                            QWidget *widget = ((QTreeWidget *)item->ptr)->itemWidget(titem, tc->index);
                            widget->setEnabled(editable);
                        }
                    } else
                    if (tc->name == (char *)"visible") {
                        int visible = ((TreeData *)item->ptr3)->attributes[attr_index].toInt();
                        titem->setHidden(!visible);
                    } else
                    if ((tc->name == (char *)"xalign") || (tc->name == (char *)"text-xalign")) {
                        QString a = ((TreeData *)item->ptr3)->attributes[attr_index];
                        if (a.size())
                            AdjustAlign(tc, titem, &a, NULL);
                    } else
                    if ((tc->name == (char *)"yalign") || (tc->name == (char *)"text-yalign")) {
                        QString a = ((TreeData *)item->ptr3)->attributes[attr_index];
                        if (a.size())
                            AdjustAlign(tc, titem, NULL, &a);
                    } else
                    if (tc->name == (char *)"height") {
                        int height = ((TreeData *)item->ptr3)->attributes[attr_index].toInt();
                        if (height >= 0) {
                            QSize size = titem->sizeHint(0);
                            size.setHeight(height);
                            titem->setSizeHint(0, size);
                        }
                    } else
                    if (tc->name == (char *)"model") {
                        QString model = ((TreeData *)item->ptr3)->attributes[attr_index];

                        if (tc->index < ((TreeData *)item->ptr3)->columns.size()) {
                            GTKControl *item2 = Client->Controls[model.toInt()];
                            if ((item2) && ((item2->type == CLASS_COMBOBOX) || (item2->type == CLASS_COMBOBOXTEXT))) {
                                titem->setText(tc->index, QString());
                                QString s2 = ((TreeData *)item->ptr3)->columns[tc->index];

                                QComboBox *widget = new EventAwareWidget<QComboBox>(item2, NULL);
                                QComboBox *base   = (QComboBox *)item2->ptr;

                                widget->setModel(base->model());
                                widget->setStyleSheet(base->styleSheet());
                                if (tc->type == 0x06)
                                    widget->setEnabled(false);

                                if (item2->type == CLASS_COMBOBOXTEXT) {
                                    widget->setCurrentIndex(widget->findText(s2));
                                    widget->setEditable(true);
                                    QLineEdit *edit = widget->lineEdit();
                                    if (edit)
                                        edit->setText(s2);
                                } else
                                    widget->setCurrentIndex(widget->findText(s2));
                                widget->setProperty("column", tc->index);
                                widget->setProperty("item", (qulonglong)titem);
                                widget->setProperty("treeview", (qulonglong)item->ptr);
                                ClientEventHandler *events = (ClientEventHandler *)item->events;
                                if (!events) {
                                    events       = new ClientEventHandler(item->ID, Client);
                                    item->events = events;
                                }

                                if (item2->type == CLASS_COMBOBOXTEXT)
                                    QObject::connect(widget, SIGNAL(editTextChanged(const QString &)), events, SLOT(on_TreeWidgetCombo_currentIndexChanged(const QString &)));
                                else
                                    QObject::connect(widget, SIGNAL(currentIndexChanged(const QString &)), events, SLOT(on_TreeWidgetCombo_currentIndexChanged(const QString &)));

                                widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
                                ((QTreeWidget *)item->ptr)->setItemWidget(titem, tc->index, widget);
                            }
                        }
                    }
                }
                attr_index++;
                break;

            case 0x05:
            case 0x85:
                {
                    //QLabel *widget = new QLabel();
                    //widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
                    titem->setText(tc->index, QString());
                    //((QTreeWidget *)item->ptr)->setItemWidget(titem, tc->index, widget);
                    if (tc->index < ((TreeData *)item->ptr3)->columns.size()) {
                        QString    s      = ((TreeData *)item->ptr3)->columns[tc->index];
                        GTKControl *item2 = Client->Controls[s.toInt()];
                        if ((item2) && (item2->type == CLASS_IMAGE)) {
                            const QPixmap *pmap = ((QLabel *)item2->ptr)->pixmap();
                            if (pmap) {
                                QSize size     = ((QTreeWidget *)item->ptr)->iconSize();
                                QSize size_map = pmap->size();
                                bool  changed  = false;
                                if (size_map.width() > size.width()) {
                                    size.setWidth(size_map.width());
                                    changed = true;
                                }

                                if (size_map.height() > size.height()) {
                                    size.setHeight(size_map.height());
                                    changed = true;
                                }

                                if (changed)
                                    ((QTreeWidget *)item->ptr)->setIconSize(size);
                                titem->setIcon(tc->index, QIcon(*pmap));
                            }
                            //widget->setPixmap(*pmap);
                        }
                    }
                }
                break;

            case 0x06:
            case 0x86:

                /*{
                    //QLabel *widget = new QLabel();
                    //widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
                    titem->setText(tc->index, QString());
                    //((QTreeWidget *)item->ptr)->setItemWidget(titem, tc->index, widget);
                    if (tc->index < ((TreeData *)item->ptr3)->columns.size()) {
                        GTKControl *item2=Client->Controls[model.toInt()];
                        if ((item2) && ((item2->type==CLASS_COMBOBOX) || (item2->type==CLASS_COMBOBOXTEXT))) {
                            QString s2 = ((TreeData *)item->ptr3)->columns[tc->index];

                            QComboBox *widget = new QComboBox();
                            QComboBox *base = (QComboBox *)item2->ptr;

                            widget->setModel(base->model());
                            widget->setStyleSheet(base->styleSheet());
                            if (tc->type==0x06)
                                widget->setEnabled(false);

                            if (item2->type==CLASS_COMBOBOXTEXT) {
                                widget->setCurrentIndex(widget->findText(s2));
                                widget->setEditable(true);
                                QLineEdit *edit = widget->lineEdit();
                                if (edit)
                                    edit->setText(s2);
                            } else
                                widget->setCurrentIndex(widget->findText(s2));
                            widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
                            ((QTreeWidget *)item->ptr)->setItemWidget(titem, tc->index, widget);
                        }
                    }
                   }*/
                last_tc = tc;
                break;

            case 0x01:
            case 0x81:
                {
                    AnsiString font = tc->Properties[P_FONTNAME];
                    if (font.Length())
                        titem->setFont(tc->index, fontDesc(titem->font(tc->index), font));
                }
                break;

            case 0x07:
            case 0x87:
                {
                    QLabel *widget = NULL;
                    if (PARAM->ID == MSG_CUSTOM_MESSAGE6) {
                        widget = (QLabel *)((QTreeWidget *)item->ptr)->itemWidget(titem, tc->index);
                        if (widget) {
                            widget->setText(((TreeData *)item->ptr3)->columns[tc->index].replace(QString("\n"), QString("<br/>")));
                            titem->setText(tc->index, QString());
                        }
                    }

                    if (!widget) {
                        widget = new QLabel();
                        widget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
                        widget->setTextFormat(Qt::RichText);
                        widget->setText(((TreeData *)item->ptr3)->columns[tc->index].replace(QString("\n"), QString("<br/>")));
                        AnsiString font = tc->Properties[P_FONTNAME];
                        if (font.Length())
                            widget->setFont(fontDesc(widget->font(), font));
                        titem->setText(tc->index, QString());
                        ((QTreeWidget *)item->ptr)->setItemWidget(titem, tc->index, widget);
                        widget->show();
                    }
                    titem->setData(tc->index, Qt::UserRole, titem->text(tc->index));
                }
                break;

            case 0x02:
            case 0x82:
            case 0x03:
            case 0x83:
            case 0x04:
            case 0x84:
                titem->setData(tc->index, Qt::UserRole, titem->text(tc->index));
                titem->setText(tc->index, QString());
                //case 0x05:
                //case 0x85:
                //titem->setText(tc->index, QString());
                break;
        }
    }
    //}

    /*if (PARAM->ID==MSG_CUSTOM_MESSAGE6) {
        QTreeWidgetItem *olditem = ItemByPath((QTreeWidget *)item->ptr, PARAM->Value);
        if (olditem) {
            QTreeWidgetItem *parent = olditem->parent();
            if (!parent)
                parent=((QTreeWidget *)item->ptr)->invisibleRootItem();
            if (parent) {
                int itemIndex = parent->indexOfChild(olditem);
                delete olditem;
                parent->insertChild(itemIndex, titem);
            }
        }
       }*/
    ((QTreeWidget *)item->ptr)->blockSignals(false);
    if (item->type == CLASS_TREEVIEW)
        ((EventAwareWidget<QTreeWidget> *)item->ptr)->UpdateHeight();
}

void MSG_CUSTOM_MESSAGE(Parameters *PARAM, CConceptClient *Client) {
    GTKControl *item = Client->Controls[PARAM->Sender.ToInt()];

    if (!item)
        return;

    /*if ((Client->LastObject) && (Client->LastObject!=item->ptr))
        ((QWidget *)Client->LastObject)->setUpdatesEnabled(true);

       Client->LastObject = (QTreeWidget *)item->ptr;
       if (Client->LastObject)
        ((QWidget *)Client->LastObject)->setUpdatesEnabled(false);*/

    switch (PARAM->ID) {
        case MSG_CUSTOM_MESSAGE1:
            switch (item->type) {
                case CLASS_TREEVIEW:
                    {
                        int type = PARAM->Value.ToInt();

                        QStringList *list = (QStringList *)item->ptr2;
                        int         index = list->size();
                        *list << QString(PARAM->Target.c_str());
                        ((QTreeWidget *)item->ptr)->setColumnCount(list->size());
                        ((QTreeWidget *)item->ptr)->setHeaderLabels(*list);

                        if (type == 0)
                            ((QTreeWidget *)item->ptr)->setColumnHidden(index, true);

                        TreeColumn *tc = new TreeColumn(PARAM->Target, type, index);
                        ((TreeData *)item->ptr3)->meta << tc;
                        if ((type == 0x86) || (type == 0x06)) {
                            AnsiString prop("model");
                            TreeColumn *tc2 = new TreeColumn(prop, -1, index);
                            ((TreeData *)item->ptr3)->meta << tc2;
                        }

                        if ((tc->type == 0x83) || (tc->type == 0x03) ||
                            (tc->type == 0x84) || (tc->type == 0x04) ||
                            //(tc->type==0x85) || (tc->type==0x05) ||
                            (tc->type == 0x81) || (tc->type == 0x01) ||
                            //(tc->type==0x87) || (tc->type==0x07) ||
                            (tc->type == 0x82) || (tc->type == 0x02)) {
                            ((QTreeWidget *)item->ptr)->setItemDelegateForColumn(index, new TreeDelegate(type, Client));
                            ((QTreeWidget *)item->ptr)->header()->setStretchLastSection(true);
                        } else {
                            if (tc->type)
                                ((QTreeWidget *)item->ptr)->header()->setStretchLastSection(false);
                            else
                                ((QTreeWidget *)item->ptr)->header()->setStretchLastSection(true);
                        }
                    }
                    break;

                case CLASS_COMBOBOX:
                case CLASS_COMBOBOXTEXT:
                case CLASS_ICONVIEW:
                    {
                        QStringList *list = (QStringList *)item->ptr2;
                        int         count = PARAM->Value.ToInt();
                        int         type  = PARAM->Target.ToInt();

                        AnsiString dummy;
                        while (count-- > 0) {
                            TreeColumn *tc = new TreeColumn(dummy, type, list->size());
                            if (((tc->type == 0x01) || (tc->type == 0x81)) && (((TreeData *)item->ptr3)->text < 0))
                                ((TreeData *)item->ptr3)->text = list->size();
                            else
                            if (((tc->type == 0x05) || (tc->type == 0x85)) && (((TreeData *)item->ptr3)->icon < 0))
                                ((TreeData *)item->ptr3)->icon = list->size();
                            else
                            if (((tc->type == 0x07) || (tc->type == 0x87)) && (((TreeData *)item->ptr3)->markup < 0) /*&& (item->type==CLASS_ICONVIEW)*/)
                                ((TreeData *)item->ptr3)->markup = list->size();
                            else
                            if (!tc->type)
                                ((TreeData *)item->ptr3)->hidden = list->size();
                            *list << QString();
                            ((TreeData *)item->ptr3)->meta << tc;
                        }
                    }
                    break;

                case CLASS_PROGRESSBAR:
                    ((QProgressBar *)item->ptr)->setMinimum(0);
                    ((QProgressBar *)item->ptr)->setMaximum(0);
                    break;

                case CLASS_MENU:
                    ((QMenu *)item->ptr)->popup(QCursor::pos());
                    break;

                case CLASS_IMAGE:
                    {
                        QLabel        *label = (QLabel *)item->ptr;
                        const QPixmap *pmap  = label->pixmap();
                        if ((pmap) && (!pmap->isNull())) {
                            unsigned int val = PARAM->Value.ToInt();
                            unsigned int w   = val / 0xFFFF;
                            unsigned int h   = val % 0xFFFF;
                            if (PARAM->Target.ToInt())
                                label->setPixmap(pmap->scaled(w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
                            else
                                label->setPixmap(pmap->scaled(w, h, Qt::IgnoreAspectRatio, Qt::FastTransformation));
                            label->setMinimumSize(QSize(w, h));
                        }
                    }
                    break;

                case CLASS_CALENDAR:
                    {
                        QDate date = ((QCalendarWidget *)item->ptr)->selectedDate();
                        int   year, month, day;
                        date.getDate(&year, &month, &day);
                        if (PARAM->Target.Length() < 3)
                            day = PARAM->Target.ToInt();
                        else {
                            QStringList list = QString::fromUtf8(PARAM->Value.c_str()).split(QString("-"), QString::SkipEmptyParts);
                            if (list.size() == 3) {
                                year  = list[0].toInt();
                                month = list[1].toInt();
                                day   = list[2].toInt();
                            }
                        }

                        if (date.setDate(year, month, day)) {
                            QTextCharFormat format = ((QCalendarWidget *)item->ptr)->dateTextFormat(date);
                            if (PARAM->Value.ToInt())
                                format.setFontWeight(QFont::Bold);
                            else
                                format.setFontWeight(QFont::Normal);
                            ((QCalendarWidget *)item->ptr)->setDateTextFormat(date, format);
                        }
                    }
                    break;

                case CLASS_FIXED:
                    {
                        unsigned int val    = PARAM->Value.ToInt();
                        unsigned int x      = val / 0x10000;
                        unsigned int y      = val % 0x10000;
                        GTKControl   *item2 = Client->Controls[PARAM->Target.ToInt()];
                        if ((!item2) || (!item2->ptr))
                            return;
                        ((QWidget *)item2->ptr)->move(x, y);
                    }
                    break;

                case CLASS_FORM:
                    ((QWidget *)item->ptr)->raise();
                    break;

                case CLASS_TEXTVIEW:
                    {
                        GTKControl *item2 = Client->Controls[PARAM->Target.ToInt()];
                        if ((item2) && (item2->type == CLASS_TEXTTAG)) {
                            QTextCharFormat format = ((QTextEdit *)item->ptr)->currentCharFormat();
                            ((QTextEdit *)item->ptr)->moveCursor(QTextCursor::End);
                            ((QTextEdit *)item->ptr)->setCurrentCharFormat(*(QTextCharFormat *)item2->ptr2);
                            ((QTextEdit *)item->ptr)->textCursor().insertText(QString::fromUtf8(PARAM->Value.c_str()));
                            ((QTextEdit *)item->ptr)->moveCursor(QTextCursor::End);
                            ((QTextEdit *)item->ptr)->setCurrentCharFormat(format);
                        }
                    }
                    break;

                default:
                    CustomMessage(item, PARAM->ID, PARAM->Target, PARAM->Value);
                    break;
            }
            break;

        case MSG_CUSTOM_MESSAGE2:
            switch (item->type) {
                case CLASS_ICONVIEW:
                    {
                        switch (PARAM->Target.ToInt()) {
                            case 0:
                                {
                                    int index  = PARAM->Value.ToInt();
                                    int icon   = ((TreeData *)item->ptr3)->icon;
                                    int dtext  = ((TreeData *)item->ptr3)->text;
                                    int markup = ((TreeData *)item->ptr3)->markup;
                                    int hint   = ((TreeData *)item->ptr3)->hint;

                                    QListWidgetItem *titem = NULL;
                                    QString         caption;

                                    int size = ((TreeData *)item->ptr3)->columns.size();
                                    if ((markup >= 0) && (markup < size))
                                        caption = ((TreeData *)item->ptr3)->columns[markup];
                                    else
                                    if ((dtext >= 0) && (dtext < size))
                                        caption = ((TreeData *)item->ptr3)->columns[dtext];

                                    int hsize = 0;
                                    int hw    = 0;

                                    if ((icon >= 0) && (icon < size)) {
                                        QString    s      = ((TreeData *)item->ptr3)->columns[icon];
                                        GTKControl *item2 = Client->Controls[s.toInt()];
                                        QIcon      icon;
                                        if ((item2) && (item2->type == CLASS_IMAGE)) {
                                            const QPixmap *pmap = ((QLabel *)item2->ptr)->pixmap();
                                            if (pmap) {
                                                icon  = QIcon(*pmap);
                                                hsize = pmap->height();
                                                hw    = pmap->width();

                                                QSize size    = ((QListWidget *)item->ptr)->iconSize();
                                                bool  changed = false;
                                                if (hw > size.width()) {
                                                    size.setWidth(hw);
                                                    changed = true;
                                                }

                                                if (hsize > size.height()) {
                                                    size.setHeight(hsize);
                                                    changed = true;
                                                }

                                                if (changed)
                                                    ((QListWidget *)item->ptr)->setIconSize(size);
                                            }
                                        }

                                        titem = new QListWidgetItem(icon, caption);
                                        if ((hsize > 0) && (((TreeData *)item->ptr3)->extra > 0)) {
                                            if (((TreeData *)item->ptr3)->orientation)
                                                titem->setSizeHint(QSize(((TreeData *)item->ptr3)->extra, hsize + 16));
                                            else
                                                titem->setSizeHint(QSize(((TreeData *)item->ptr3)->extra, hsize));
                                        }
                                    } else
                                        titem = new QListWidgetItem(caption);
                                    if (((QWidget *)item->ptr)->acceptDrops())
                                        titem->setFlags(titem->flags() | Qt::ItemIsDropEnabled);
                                    if ((hint >= 0) && (hint < size)) {
                                        titem->setToolTip(((TreeData *)item->ptr3)->columns[hint]);
                                        titem->setWhatsThis(((TreeData *)item->ptr3)->columns[hint]);
                                    }

                                    if ((index < 0) || (index >= ((QListWidget *)item->ptr)->count()))
                                        ((QListWidget *)item->ptr)->addItem(titem);
                                    else
                                        ((QListWidget *)item->ptr)->insertItem(index, titem);

                                    ((TreeData *)item->ptr3)->columns.clear();
                                    ((TreeData *)item->ptr3)->attributes.clear();
                                    ((TreeData *)item->ptr3)->currentindex = 0;
                                }
                                break;

                            case 1:
                                ((QListWidget *)item->ptr)->clear();
                                if (item->ptr3) {
                                    ((TreeData *)item->ptr3)->columns.clear();
                                    ((TreeData *)item->ptr3)->attributes.clear();
                                    ((TreeData *)item->ptr3)->currentindex = 0;
                                }
                                break;

                            case 2:
                                {
                                    QListWidgetItem *litem = ((QListWidget *)item->ptr)->takeItem(PARAM->Value.ToInt());
                                    delete litem;
                                }
                                break;

                            case 3:
                                {
                                    ((QListWidget *)item->ptr)->clear();
                                    if (item->ptr3) {
                                        ((TreeData *)item->ptr3)->columns.clear();
                                        ((TreeData *)item->ptr3)->attributes.clear();
                                        ((TreeData *)item->ptr3)->currentindex = 0;
                                        ((TreeData *)item->ptr3)->ClearColumns();
                                    }
                                    if (item->ptr2)
                                        ((QStringList *)item->ptr2)->clear();
                                }
                                break;
                        }
                    }
                    break;

                case CLASS_COMBOBOX:
                case CLASS_COMBOBOXTEXT:
                    {
                        switch (PARAM->Target.ToInt()) {
                            case 0:
                                {
                                    int     index  = PARAM->Value.ToInt();
                                    int     icon   = ((TreeData *)item->ptr3)->icon;
                                    int     dtext  = ((TreeData *)item->ptr3)->text;
                                    int     markup = ((TreeData *)item->ptr3)->markup;
                                    int     hint   = ((TreeData *)item->ptr3)->hint;
                                    int     hidden = ((TreeData *)item->ptr3)->hidden;
                                    QString caption;
                                    int     size = ((TreeData *)item->ptr3)->columns.size();

                                    if ((dtext >= 0) && (dtext < size)) {
                                        caption = ((TreeData *)item->ptr3)->columns[dtext];
                                    } else
                                    if ((markup >= 0) && (markup < size))
                                        caption = ((TreeData *)item->ptr3)->columns[markup].remove(QRegExp("<[^>]*>"));
                                    else
                                    if ((hidden >= 0) && (hidden < size))
                                        caption = ((TreeData *)item->ptr3)->columns[hidden];
                                    else
                                    if (((icon < 0) || (icon >= size)) && (size > 0))
                                        caption = ((TreeData *)item->ptr3)->columns[0];

                                    int hsize = 0;
                                    int hw    = 0;

                                    QIcon cicon;
                                    if ((icon >= 0) && (icon < size)) {
                                        QString    s      = ((TreeData *)item->ptr3)->columns[icon];
                                        GTKControl *item2 = Client->Controls[s.toInt()];
                                        if ((item2) && (item2->type == CLASS_IMAGE)) {
                                            const QPixmap *pmap = ((QLabel *)item2->ptr)->pixmap();
                                            if (pmap) {
                                                cicon = QIcon(*pmap);
                                                hsize = pmap->height();
                                                hw    = pmap->width();

                                                QSize size    = ((QComboBox *)item->ptr)->iconSize();
                                                bool  changed = false;
                                                if (hw > size.width()) {
                                                    size.setWidth(hw);
                                                    changed = true;
                                                }

                                                if (hsize > size.height()) {
                                                    size.setHeight(hsize);
                                                    changed = true;
                                                }

                                                if (changed)
                                                    ((QComboBox *)item->ptr)->setIconSize(size);
                                            }
                                        }
                                    }

                                    if ((index < 0) || (index >= ((QComboBox *)item->ptr)->count())) {
                                        if ((icon >= 0) && (icon < size))
                                            ((QComboBox *)item->ptr)->addItem(cicon, caption);
                                        else
                                            ((QComboBox *)item->ptr)->addItem(caption);
                                    } else {
                                        if ((icon >= 0) && (icon < size))
                                            ((QComboBox *)item->ptr)->insertItem(index, cicon, caption);
                                        else
                                            ((QComboBox *)item->ptr)->insertItem(index, caption);
                                    }

                                    ((TreeData *)item->ptr3)->columns.clear();
                                    ((TreeData *)item->ptr3)->attributes.clear();
                                    ((TreeData *)item->ptr3)->currentindex = 0;
                                }
                                break;

                            case 1:
                                {
                                    ((QComboBox *)item->ptr)->clear();
                                    if (item->ptr3) {
                                        ((TreeData *)item->ptr3)->columns.clear();
                                        ((TreeData *)item->ptr3)->attributes.clear();
                                        ((TreeData *)item->ptr3)->currentindex = 0;
                                    }
                                }
                                break;

                            case 2:
                                {
                                    ((QComboBox *)item->ptr)->removeItem(PARAM->Value.ToInt());
                                }
                                break;

                            case 3:
                                {
                                    if (item->type == CLASS_COMBOBOXTEXT)
                                        ((QComboBox *)item->ptr)->setCurrentText("");
                                    ((QComboBox *)item->ptr)->clear();
                                    if (item->ptr3) {
                                        ((TreeData *)item->ptr3)->columns.clear();
                                        ((TreeData *)item->ptr3)->attributes.clear();
                                        ((TreeData *)item->ptr3)->currentindex = 0;
                                        ((TreeData *)item->ptr3)->ClearColumns();
                                    }
                                    if (item->ptr2)
                                        ((QStringList *)item->ptr2)->clear();
                                }
                                break;
                        }
                    }
                    break;

                case CLASS_TREEVIEW:
                    {
                        switch (PARAM->Target.ToInt()) {
                            case 0:
                                {
                                    CreateTreeItem(item, PARAM, Client);
                                    ((TreeData *)item->ptr3)->columns.clear();
                                    ((TreeData *)item->ptr3)->attributes.clear();
                                    ((TreeData *)item->ptr3)->currentindex = 0;

                                    if (!item->flags) {
                                        int count = ((QTreeWidget *)item->ptr)->topLevelItemCount();
                                        if (/*(count%10==1) && */ (count < 100)) {
                                            resizeColumnsToContents((QTreeWidget *)item->ptr);
                                            if (count > 20)
                                                item->flags = 1;
                                        }
                                    }
                                }
                                break;

                            case 1:
                                ((QTreeWidget *)item->ptr)->clear();
                                ((TreeData *)item->ptr3)->columns.clear();
                                ((TreeData *)item->ptr3)->attributes.clear();
                                ((TreeData *)item->ptr3)->currentindex = 0;
                                ((EventAwareWidget<QTreeWidget> *)item->ptr)->UpdateHeight();
                                break;

                            case 2:
                                {
                                    //((QTreeWidget *)item->ptr)->setUpdatesEnabled(false);
                                    QTreeWidgetItem *titem = ItemByPath((QTreeWidget *)item->ptr, PARAM->Value);
                                    if (titem) {
                                        if (titem == ((QTreeWidget *)item->ptr)->currentItem())
                                            ((QTreeWidget *)item->ptr)->setCurrentItem(NULL);

                                        QTreeWidgetItem *parent = titem->parent();
                                        if (parent)
                                            parent->removeChild(titem);

                                        delete titem;
                                        ((EventAwareWidget<QTreeWidget> *)item->ptr)->UpdateHeight();
                                    }
                                    //((QTreeWidget *)item->ptr)->setUpdatesEnabled(true);
                                }
                                break;

                            case 3:
                                {
                                    ((QTreeWidget *)item->ptr)->clear();
                                    if (item->ptr3) {
                                        ((TreeData *)item->ptr3)->columns.clear();
                                        ((TreeData *)item->ptr3)->attributes.clear();
                                        ((TreeData *)item->ptr3)->currentindex = 0;
                                        ((TreeData *)item->ptr3)->ClearColumns();
                                    }
                                    if (item->ptr2)
                                        ((QStringList *)item->ptr2)->clear();

                                    ((QTreeWidget *)item->ptr)->setColumnCount(0);
                                    ((QTreeWidget *)item->ptr)->setHeaderLabels(*(QStringList *)item->ptr2);
                                }
                                break;

                            case 4:
                                {
                                    int idx = PARAM->Value.ToInt();
                                    if ((idx >= 0) && (idx < ((QStringList *)item->ptr2)->size())) {
                                        ((QStringList *)item->ptr2)->removeAt(idx);
                                        ((QTreeWidget *)item->ptr)->setColumnCount(((QStringList *)item->ptr2)->size());
                                        ((QTreeWidget *)item->ptr)->setHeaderLabels(*(QStringList *)item->ptr2);
                                    }
                                }
                                break;
                        }
                    }
                    break;

                case CLASS_CALENDAR:
                    {
                        QDate date = ((QCalendarWidget *)item->ptr)->selectedDate();
                        int   year, month, day;
                        date.getDate(&year, &month, &day);

                        for (int i = 1; i < 31; i++) {
                            if (date.setDate(year, month, i)) {
                                QTextCharFormat format = ((QCalendarWidget *)item->ptr)->dateTextFormat(date);
                                if (format.fontWeight() == QFont::Bold) {
                                    format.setFontWeight(QFont::Normal);
                                    ((QCalendarWidget *)item->ptr)->setDateTextFormat(date, format);
                                }
                            }
                        }
                    }
                    break;

                case CLASS_TEXTVIEW:
                    {
                        GTKControl *item2 = Client->Controls[PARAM->Target.ToInt()];
                        if ((item2) && (item2->type == CLASS_TEXTTAG)) {
                            QTextCharFormat format = ((QTextEdit *)item->ptr)->currentCharFormat();
                            ((QTextEdit *)item->ptr)->setCurrentCharFormat(*(QTextCharFormat *)item2->ptr2);
                            ((QTextEdit *)item->ptr)->textCursor().insertText(QString::fromUtf8(PARAM->Value.c_str()));
                            ((QTextEdit *)item->ptr)->setCurrentCharFormat(format);
                        }
                    }
                    break;

                case CLASS_NOTEBOOK:
                    {
                        GTKControl *item2 = Client->Controls[PARAM->Target.ToInt()];
                        if ((item2) && (item2->ptr)) {
                            NotebookTab *nt = (NotebookTab *)item->pages->Item(item2->index);
                            if (nt) {
                                nt->title = PARAM->Value.c_str();
                                if (nt->visible) {
                                    int index = item->AdjustIndex(nt);
                                    ((QTabWidget *)item->ptr)->setTabText(index, QString::fromUtf8(PARAM->Value.c_str()));
                                }
                            }
                        }
                    }
                    break;

                case CLASS_IMAGE:
                    {
                        int           op     = PARAM->Target.ToInt();
                        QLabel        *label = (QLabel *)item->ptr;
                        const QPixmap *pmap  = label->pixmap();
                        if (pmap) {
                            if (op == 1) {
                                QTransform transform;
                                QTransform trans = transform.rotate(PARAM->Value.ToInt());
                                QPixmap    pmap2(pmap->transformed(trans));
                                label->setPixmap(pmap2);
                                label->setMinimumSize(pmap2.size());
                            } else
                            if (op == 0) {
                                QImage img = pmap->toImage();
                                if (PARAM->Value.ToInt())
                                    img = img.mirrored(true, false);
                                else
                                    img = img.mirrored(false, true);
                                QPixmap pmap2 = QPixmap::fromImage(img);
                                label->setPixmap(pmap2);
                                label->setMinimumSize(pmap2.size());
                            }
                        }
                    }
                    break;

                default:
                    CustomMessage(item, PARAM->ID, PARAM->Target, PARAM->Value);
                    break;
            }
            break;

        case MSG_CUSTOM_MESSAGE3:
            switch (item->type) {
                case CLASS_TREEVIEW:
                case CLASS_ICONVIEW:
                case CLASS_COMBOBOX:
                case CLASS_COMBOBOXTEXT:
                    {
                        int        index = ((TreeData *)item->ptr3)->currentindex++;
                        TreeColumn *tc   = NULL;
                        if ((index >= 0) && (index < ((TreeData *)item->ptr3)->meta.size()))
                            tc = ((TreeData *)item->ptr3)->meta[index];

                        if (tc) {
                            if (tc->type >= 0)
                                ((TreeData *)item->ptr3)->columns << QString(PARAM->Value.c_str());
                            else
                                ((TreeData *)item->ptr3)->attributes << QString(PARAM->Value.c_str());
                        }
                    }
                    break;

                case CLASS_NOTEBOOK:
                    {
                        int         index = PARAM->Target.ToInt();
                        NotebookTab *nt   = (NotebookTab *)item->pages->Item(index);
                        if (nt) {
                            GTKControl *item3 = Client->Controls[PARAM->Value.ToInt()];
                            if (item3) {
                                int adjusted = item->AdjustIndex(nt);
                                if (nt->visible)
                                    ((QTabWidget *)item->ptr)->setTabText(index, QString());

                                MarkRecursive(Client, item, index, (QWidget *)item3->ptr, nt, adjusted);
                            }
                        }
                    }
                    break;

                case CLASS_IMAGE:
                    {
                        QLabel        *label = (QLabel *)item->ptr;
                        const QPixmap *pmap  = label->pixmap();
                        if ((pmap) && (!pmap->isNull())) {
                            double coef = PARAM->Value.ToFloat();
                            int    w    = coef * pmap->width();
                            int    h    = coef * pmap->height();

                            if (PARAM->Target.ToInt())
                                label->setPixmap(pmap->scaled(w, h, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
                            else
                                label->setPixmap(pmap->scaled(w, h, Qt::IgnoreAspectRatio, Qt::FastTransformation));
                            label->setMinimumSize(QSize(w, h));
                        }
                    }
                    break;

#ifndef NO_WEBKIT
                    case CLASS_HTMLSNAP:
                        if ((PARAM->Value.Length() > 2) && (item->ptr3)) {
                            AnsiString *class_name = (AnsiString *)item->ptr3;
                            QString str = QString::fromUtf8(class_name->c_str());
                            str += "Set({ \"Client\": { \"Fire\": function(RID, msg) { alert(\"Events not yet supported!\"); } }, \"RID\": \"\" + ";
                            AnsiString obj_id = AnsiString((long)item->ID);
                            str += obj_id.c_str();
                            str += ", \"Object\": { \"ConceptClassID\": ";
                            str += obj_id.c_str();
                            str += " } }, ";
                            str += PARAM->Value.c_str();
                            str += ");";
                            ((QWebView *)item->ptr)->page()->mainFrame()->evaluateJavaScript(str);
                        } else
                            fprintf(stderr, "Cannot call GET/SET for classless snap objects");
                        break;
#endif
                default:
                    CustomMessage(item, PARAM->ID, PARAM->Target, PARAM->Value);
                    break;
            }
            break;

        case MSG_CUSTOM_MESSAGE4:
            switch (item->type) {
                case CLASS_NOTEBOOK:
                    {
                        GTKControl *item2 = Client->Controls[PARAM->Target.ToInt()];
                        if ((item2) && (item2->ptr)) {
                            QWidget *widget = (QWidget *)item2->ptr;
                            //int index=((QTabWidget *)item->ptr)->indexOf(widget);
                            //if (index>=0) {
                            NotebookTab *nt = (NotebookTab *)item->pages->Item(item2->index);
                            if (nt) {
                                GTKControl *item3 = Client->Controls[PARAM->Value.ToInt()];
                                if (item3) {
                                    int adjusted = item->AdjustIndex(nt);
                                    if (nt->visible)
                                        ((QTabWidget *)item->ptr)->setTabText(item2->index, QString());

                                    if (nt->header) {
                                        GTKControl *item_header = (GTKControl *)nt->header;
                                        if (item_header->ptr)
                                            ((QWidget *)item_header->ptr)->setProperty("notebookheader", 0);
                                        ((QTabWidget *)item->ptr)->tabBar()->setTabButton(adjusted, QTabBar::RightSide, NULL);
                                        nt->header = NULL;
                                    }

                                    switch (item3->type) {
                                        case CLASS_HBOX:
                                        case CLASS_IMAGE:
                                            MarkRecursive(Client, item, item2->index, (QWidget *)item3->ptr, nt, adjusted);
                                            break;

                                        default:
                                            nt->header = item3;
                                            if (item3->ptr)
                                                ((QWidget *)item3->ptr)->setProperty("notebookheader", (qulonglong)nt);
                                            ((QTabWidget *)item->ptr)->tabBar()->setTabButton(adjusted, QTabBar::RightSide, (QWidget *)item3->ptr);
                                            break;
                                    }
                                }
                            }
                            //}
                        }
                    }
                    break;

                case CLASS_TREEVIEW:
                    CreateTreeItem(item, PARAM, Client);
                    ((TreeData *)item->ptr3)->columns.clear();
                    ((TreeData *)item->ptr3)->attributes.clear();
                    ((TreeData *)item->ptr3)->currentindex = 0;
                    break;

                default:
                    CustomMessage(item, PARAM->ID, PARAM->Target, PARAM->Value);
                    break;
            }
            break;

        case MSG_CUSTOM_MESSAGE5:
            {
                switch (item->type) {
                    case CLASS_TREEVIEW:
                        switch (PARAM->Target.ToInt()) {
                            case 0:
                                ((QTreeWidget *)item->ptr)->expandAll();
                                break;

                            case 1:
                                ((QTreeWidget *)item->ptr)->collapseAll();
                                break;

                            case 2:
                            case 4:
                                {
                                    QTreeWidgetItem *element = ItemByPath((QTreeWidget *)item->ptr, PARAM->Value);
                                    if (element)
                                        element->setExpanded(true);
                                }
                                break;

                            case 3:
                                {
                                    QTreeWidgetItem *element = ItemByPath((QTreeWidget *)item->ptr, PARAM->Value);
                                    if (element)
                                        element->setExpanded(false);
                                }
                                break;
                        }

#ifndef NO_WEBKIT
                    case CLASS_HTMLSNAP:
                        if (item->ptr3) {
                            AnsiString *class_name = (AnsiString *)item->ptr3;
                            QString str = QString::fromUtf8(class_name->c_str());
                            str += "Set({ \"Client\": { \"Fire\": function(RID, msg) { alert(\"Events not yet supported!\"); } }, \"RID\": \"\" + ";
                            AnsiString obj_id = AnsiString((long)item->ID);
                            str += obj_id.c_str();
                            str += ", \"Object\": { \"ConceptClassID\": ";
                            str += obj_id.c_str();
                            str += " } }, \'";

                            QString data =  QString::fromUtf8(PARAM->Value.c_str());
                            data = data.replace("\\", "\\\\");
                            data = data.replace("'", "\\'");

                            str += data;
                            str += "\');";
                            ((QWebView *)item->ptr)->page()->mainFrame()->evaluateJavaScript(str);
                        } else
                            fprintf(stderr, "Cannot call GET/SET for classless snap objects");
                        break;

#endif

                    default:
                        CustomMessage(item, PARAM->ID, PARAM->Target, PARAM->Value);
                        break;
                }
            }
            break;

        case MSG_CUSTOM_MESSAGE6:
            switch (item->type) {
                case CLASS_TREEVIEW:
                    CreateTreeItem(item, PARAM, Client);
                    ((TreeData *)item->ptr3)->columns.clear();
                    ((TreeData *)item->ptr3)->attributes.clear();
                    ((TreeData *)item->ptr3)->currentindex = 0;
                    break;

                case CLASS_COMBOBOX:
                case CLASS_COMBOBOXTEXT:
                    {
                        int     index  = PARAM->Value.ToInt();
                        int     icon   = ((TreeData *)item->ptr3)->icon;
                        int     dtext  = ((TreeData *)item->ptr3)->text;
                        int     markup = ((TreeData *)item->ptr3)->markup;
                        int     hint   = ((TreeData *)item->ptr3)->hint;
                        int     hidden = ((TreeData *)item->ptr3)->hidden;
                        QString caption;

                        int size = ((TreeData *)item->ptr3)->columns.size();
                        if ((dtext >= 0) && (dtext < size)) {
                            caption = ((TreeData *)item->ptr3)->columns[dtext];
                        } else
                        if ((markup >= 0) && (markup < size))
                            caption = ((TreeData *)item->ptr3)->columns[markup].remove(QRegExp("<[^>]*>"));
                        else
                        if ((hidden >= 0) && (hidden < size))
                            caption = ((TreeData *)item->ptr3)->columns[hidden];

                        int hsize = 0;
                        int hw    = 0;

                        QIcon cicon;
                        if ((icon >= 0) && (icon < size)) {
                            QString    s      = ((TreeData *)item->ptr3)->columns[icon];
                            GTKControl *item2 = Client->Controls[s.toInt()];
                            if ((item2) && (item2->type == CLASS_IMAGE)) {
                                const QPixmap *pmap = ((QLabel *)item2->ptr)->pixmap();
                                if (pmap) {
                                    cicon = QIcon(*pmap);
                                    hsize = pmap->height();
                                    hw    = pmap->width();

                                    QSize size    = ((QComboBox *)item->ptr)->iconSize();
                                    bool  changed = false;
                                    if (hw > size.width()) {
                                        size.setWidth(hw);
                                        changed = true;
                                    }

                                    if (hsize > size.height()) {
                                        size.setHeight(hsize);
                                        changed = true;
                                    }

                                    if (changed)
                                        ((QComboBox *)item->ptr)->setIconSize(size);
                                }
                            }
                        }

                        if ((index >= 0) && (index < size)) {
                            if ((icon >= 0) && (icon < size))
                                ((QComboBox *)item->ptr)->setItemIcon(index, cicon);

                            ((QComboBox *)item->ptr)->setItemText(index, caption);
                        }

                        ((TreeData *)item->ptr3)->columns.clear();
                        ((TreeData *)item->ptr3)->attributes.clear();
                        ((TreeData *)item->ptr3)->currentindex = 0;
                    }
                    break;

                case CLASS_ICONVIEW:
                    {
                        int index = PARAM->Value.ToInt();
                        QListWidgetItem *titem = ((QListWidget *)item->ptr)->item(index);
                        if (!titem)
                            break;

                        int icon   = ((TreeData *)item->ptr3)->icon;
                        int dtext  = ((TreeData *)item->ptr3)->text;
                        int markup = ((TreeData *)item->ptr3)->markup;
                        int hint   = ((TreeData *)item->ptr3)->hint;

                        QString caption;

                        int size = ((TreeData *)item->ptr3)->columns.size();
                        if ((markup >= 0) && (markup < size))
                            caption = ((TreeData *)item->ptr3)->columns[markup];
                        else
                        if ((dtext >= 0) && (dtext < size))
                            caption = ((TreeData *)item->ptr3)->columns[dtext];

                        int hsize = 0;
                        int hw    = 0;

                        if ((icon >= 0) && (icon < size)) {
                            QString    s      = ((TreeData *)item->ptr3)->columns[icon];
                            GTKControl *item2 = Client->Controls[s.toInt()];
                            QIcon      icon;
                            if ((item2) && (item2->type == CLASS_IMAGE)) {
                                const QPixmap *pmap = ((QLabel *)item2->ptr)->pixmap();
                                if (pmap) {
                                    icon  = QIcon(*pmap);
                                    hsize = pmap->height();
                                    hw    = pmap->width();

                                    QSize size    = ((QListWidget *)item->ptr)->iconSize();
                                    bool  changed = false;
                                    if (hw > size.width()) {
                                        size.setWidth(hw);
                                        changed = true;
                                    }

                                    if (hsize > size.height()) {
                                        size.setHeight(hsize);
                                        changed = true;
                                    }

                                    if (changed)
                                        ((QListWidget *)item->ptr)->setIconSize(size);
                                }
                            }
                        }
                        titem->setText(caption);

                        if ((hint >= 0) && (hint < size)) {
                            titem->setToolTip(((TreeData *)item->ptr3)->columns[hint]);
                            titem->setWhatsThis(((TreeData *)item->ptr3)->columns[hint]);
                        }

                        ((TreeData *)item->ptr3)->columns.clear();
                        ((TreeData *)item->ptr3)->attributes.clear();
                        ((TreeData *)item->ptr3)->currentindex = 0;
                    }
                    break;

                default:
                    CustomMessage(item, PARAM->ID, PARAM->Target, PARAM->Value);
                    break;
            }
            break;

        case MSG_CUSTOM_MESSAGE7:
            switch (item->type) {
                case CLASS_TREEVIEW:
                    {
                        int prop_id = PARAM->Target.ToInt();
                        switch (prop_id) {
                            case P_WIDGET:
                                {
                                    int        index  = PARAM->Owner->POST_STRING.ToInt();
                                    GTKControl *item2 = Client->Controls[PARAM->Value.ToInt()];
                                    if ((index >= 0) && (item2) && (item2->ptr)) {
                                        QHeaderView *header = ((QTreeWidget *)item->ptr)->header();
                                        if ((header) && (header->viewport())) {
                                            int height = header->height();
                                            ((QWidget *)item2->ptr)->hide();
                                            if (item2->type == CLASS_CHECKBUTTON)
                                                ((QWidget *)item2->ptr)->setAttribute(Qt::WA_TransparentForMouseEvents);
                                            ((QWidget *)item2->ptr)->setParent(header->viewport());
                                            item2->parent = item;
                                            ((QWidget *)item2->ptr)->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
                                            ((QWidget *)item2->ptr)->show();

                                            int h2 = ((QWidget *)item2->ptr)->height();
                                            int w2 = ((QWidget *)item2->ptr)->width();
                                            if ((height > 0) && (h2 > 0)) {
                                                int width = header->sectionSize(index) / 2;
                                                ((QWidget *)item2->ptr)->move(header->sectionPosition(index) + (width - w2) / 2, (height - h2) / 2);
                                            }
                                            int len = ((TreeData *)item->ptr3)->meta.size();
                                            if (index < len) {
                                                for (int i = index; i < len; i++) {
                                                    TreeColumn *tc = ((TreeData *)item->ptr3)->meta[index];
                                                    if ((tc) && (tc->index == index)) {
                                                        if (tc->widget)
                                                            tc->widget->hide();
                                                        tc->widget = (QWidget *)item2->ptr;
                                                        ClientEventHandler *events = (ClientEventHandler *)item->events;
                                                        if (!events) {
                                                            events       = new ClientEventHandler(item->ID, Client);
                                                            item->events = events;
                                                        }
                                                        QObject::connect(header, SIGNAL(sectionResized(int, int, int)), events, SLOT(on_sectionResized(int, int, int)));
                                                        QObject::connect(header, SIGNAL(geometriesChanged()), events, SLOT(on_geometriesChanged()));
                                                        //QObject::connect((QTreeWidget *)item->ptr, SIGNAL(itemCollapsed(QTreeWidgetItem *)), events, SLOT(on_itemCollapseExpand(QTreeWidgetItem *)));
                                                        //QObject::connect((QTreeWidget *)item->ptr, SIGNAL(itemExpanded(QTreeWidgetItem *)), events, SLOT(on_itemCollapseExpand(QTreeWidgetItem *)));
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                                break;

                            case P_VISIBLE:
                                {
                                    int index = PARAM->Owner->POST_STRING.ToInt();
                                    if ((PARAM->Value.ToInt() > 0) && (index >= 0))
                                        ((QTreeWidget *)item->ptr)->showColumn(index);
                                    else
                                        ((QTreeWidget *)item->ptr)->hideColumn(index);
                                }
                                break;

                            case P_CAPTION:
                                {
                                    int         index = PARAM->Owner->POST_STRING.ToInt();
                                    QStringList *list = (QStringList *)item->ptr2;
                                    if ((index >= 0) && (index < ((QTreeWidget *)item->ptr)->header()->count())) {
                                        list->replace(index, QString(PARAM->Value.c_str()));
                                        ((QTreeWidget *)item->ptr)->setHeaderLabels(*list);
                                    }
                                }
                                break;

                            case P_RESIZABLE:
                                {
                                    int index = PARAM->Owner->POST_STRING.ToInt();
                                    if ((index >= 0) && (index < ((QTreeWidget *)item->ptr)->header()->count())) {
                                        if (PARAM->Value.ToInt()) {
                                            ((QTreeWidget *)item->ptr)->header()->setSectionResizeMode(index, QHeaderView::Interactive);
                                            //((QTreeWidget *)item->ptr)->header()->setSectionResizeMode(index, QHeaderView::Interactive);
                                            resizeColumnsToContents((QTreeWidget *)item->ptr);
                                        } else
                                            ((QTreeWidget *)item->ptr)->header()->setSectionResizeMode(index, QHeaderView::ResizeToContents);
                                    }
                                }
                                break;

                            case P_SORTINDICATOR:
                                {
                                    int index = PARAM->Owner->POST_STRING.ToInt();
                                    if ((index >= 0) && (index < ((QTreeWidget *)item->ptr)->header()->count()))
                                        ((QTreeWidget *)item->ptr)->header()->setSortIndicatorShown((bool)PARAM->Value.ToInt());
                                }
                                break;

                            case P_SORTINDICATORTYPE:
                                {
                                    int index = PARAM->Owner->POST_STRING.ToInt();
                                    if ((index >= 0) && (index < ((QTreeWidget *)item->ptr)->header()->count())) {
                                        switch (PARAM->Value.ToInt()) {
                                            case 1:
                                                ((QTreeWidget *)item->ptr)->header()->setSortIndicator(index, Qt::DescendingOrder);
                                                break;

                                            default:
                                                // ascending
                                                ((QTreeWidget *)item->ptr)->header()->setSortIndicator(index, Qt::AscendingOrder);
                                                break;
                                        }
                                    }
                                }

                            case P_WIDTH:
                                {
                                    AnsiString res("-1");
                                    int        index = PARAM->Owner->POST_STRING.ToInt();
                                    if ((index >= 0) && (index < ((QTreeWidget *)item->ptr)->header()->count()))
                                        res = AnsiString((long)((QTreeWidget *)item->ptr)->header()->sectionSize(index));
                                    PARAM->Owner->SendMessageNoWait(PARAM->Sender, PARAM->ID, PARAM->Target, res.c_str());
                                }
                                break;

                            case P_MINWIDTH:
                            case P_FIXEDWIDTH:
                                {
                                    int index = PARAM->Owner->POST_STRING.ToInt();
                                    if ((index >= 0) && (index < ((QTreeWidget *)item->ptr)->header()->count()))
                                        ((QTreeWidget *)item->ptr)->header()->resizeSection(index, PARAM->Value.ToInt());
                                }
                                break;

                            default:
                                {
                                    int index = PARAM->Owner->POST_STRING.ToInt();
                                    int size  = ((TreeData *)item->ptr3)->meta.size();
                                    if ((index >= 0) && (index < size)) {
                                        for (int i = 0; i < size; i++) {
                                            TreeColumn *tc = ((TreeData *)item->ptr3)->meta[i];
                                            if (tc->type >= 0) {
                                                if (tc->index == index) {
                                                    tc->Properties[prop_id] = PARAM->Value;
                                                    if ((prop_id == P_XALIGN) || (prop_id == P_YALIGN))
                                                        AdjustAlign(tc, ((QTreeWidget *)item->ptr)->headerItem());
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                }
                        }
                    }
                    break;

                default:
                    CustomMessage(item, PARAM->ID, PARAM->Target, PARAM->Value);
                    break;
            }
            break;

        case MSG_CUSTOM_MESSAGE8:
            CustomMessage(item, PARAM->ID, PARAM->Target, PARAM->Value);
            break;

        case MSG_CUSTOM_MESSAGE9:
            {
                switch (item->type) {
                    case CLASS_TREEVIEW:
                        {
                            QPalette p = ((QTreeWidget *)item->ptr)->palette();
                            p.setColor(QPalette::AlternateBase, doColor(PARAM->Target.ToInt()));
                            ((QTreeWidget *)item->ptr)->setPalette(p);
                        }
                        break;

                    default:
                        CustomMessage(item, PARAM->ID, PARAM->Target, PARAM->Value);
                        break;
                }
            }
            break;

        case MSG_CUSTOM_MESSAGE10:
            switch (item->type) {
                case CLASS_TREEVIEW:
                    if (PARAM->Target.ToInt() < 0)
                        ((TreeData *)item->ptr3)->meta << new TreeColumn(PARAM->Value, -1, -1);
                    else
                        ((TreeData *)item->ptr3)->meta << new TreeColumn(PARAM->Value, -1, PARAM->Owner->POST_STRING.ToInt());
                    break;

                default:
                    CustomMessage(item, PARAM->ID, PARAM->Target, PARAM->Value);
                    break;
            }
            break;

        case MSG_CUSTOM_MESSAGE11:
            switch (item->type) {
                case CLASS_TREEVIEW:
                    {
                        QTreeWidgetItem *titem = ItemByPath((QTreeWidget *)item->ptr, PARAM->Target);
                        if (titem)
                            ((QTreeWidget *)item->ptr)->editItem(titem, PARAM->Value.ToInt());
                    }
                    break;

                default:
                    CustomMessage(item, PARAM->ID, PARAM->Target, PARAM->Value);
                    break;
            }
            break;

        case MSG_CUSTOM_MESSAGE12:
            {
                switch (item->type) {
                    case CLASS_TREEVIEW:
                        {
                            QTreeWidgetItem *titem = ItemByPath((QTreeWidget *)item->ptr, PARAM->Target);
                            if (item) {
                                QRect rect = ((QTreeWidget *)item->ptr)->visualItemRect(titem);

                                AnsiString result((long)rect.x());
                                result += (char *)",";
                                result += AnsiString((long)rect.y());
                                result += (char *)",";
                                result += AnsiString((long)rect.width());
                                result += (char *)",";
                                result += AnsiString((long)rect.height());
                                PARAM->Owner->SendMessageNoWait(PARAM->Sender, PARAM->ID, "0", result.c_str());
                            } else
                                PARAM->Owner->SendMessageNoWait(PARAM->Sender, PARAM->ID, "0", "-1,-1,-1,-1");
                        }
                        break;

                    default:
                        CustomMessage(item, PARAM->ID, PARAM->Target, PARAM->Value);
                        break;
                }
            }
            break;

        case MSG_CUSTOM_MESSAGE13:
            {
                switch (item->type) {
                    case CLASS_TREEVIEW:
                        {
                            QTreeWidgetItem *titem = ItemByPath((QTreeWidget *)item->ptr, PARAM->Target);
                            if (item) {
                                QRect rect = ((QTreeWidget *)item->ptr)->visualItemRect(titem);

                                rect.setTopLeft(((QWidget *)item->ptr)->mapToGlobal(rect.topLeft()));
                                rect.setBottomRight(((QWidget *)item->ptr)->mapToGlobal(rect.bottomRight()));

                                AnsiString result((long)rect.x());
                                result += (char *)",";
                                result += AnsiString((long)rect.y());
                                result += (char *)",";
                                result += AnsiString((long)rect.width());
                                result += (char *)",";
                                result += AnsiString((long)rect.height());
                                //fprintf(stderr, "RECT: %s\n", result.c_str());
                                PARAM->Owner->SendMessageNoWait(PARAM->Sender, PARAM->ID, "0", result.c_str());
                            } else
                                PARAM->Owner->SendMessageNoWait(PARAM->Sender, PARAM->ID, "0", "-1,-1,-1,-1");
                        }
                        break;

                    default:
                        CustomMessage(item, PARAM->ID, PARAM->Target, PARAM->Value);
                        break;
                }
            }
            break;

        case MSG_CUSTOM_MESSAGE14:
            {
                switch (item->type) {
                    case CLASS_TREEVIEW:
                        {
                            QTreeWidgetItem *titem = ((QTreeWidget *)item->ptr)->itemAt(PARAM->Target.ToInt(), PARAM->Value.ToInt());
                            AnsiString      result;
                            AnsiString      res_column((char *)"-1");
                            if (titem)
                                result = ItemToPath((QTreeWidget *)item->ptr, titem);

                            PARAM->Owner->SendMessageNoWait(PARAM->Sender, PARAM->ID, /*res_column.c_str()*/ "-1", result.c_str());
                        }
                        break;

                    default:
                        CustomMessage(item, PARAM->ID, PARAM->Target, PARAM->Value);
                        break;
                }
            }
            break;

        case MSG_REPAINT:
            {
                if (item->ptr)
                    ((QWidget *)item->ptr)->repaint();
            }
            break;
    }
}

void StaticQuery(TParameters *PARAM) {
    switch (PARAM->Sender.ToInt()) {
        case 1011:
            {
                QString    key = QString::fromUtf8(PARAM->Target.c_str());
                AnsiString command;
                AnsiString parameter;

                bool separator_found = false;
                int  param2_len      = PARAM->Value.Length();
                char *param2         = PARAM->Value.c_str();

                for (int i = 0; i < param2_len; i++) {
                    char c = param2[i];
                    if (separator_found)
                        parameter += c;
                    else {
                        if (c == ':')
                            separator_found = true;
                        else
                            command += c;
                    }
                }

                if (command == (char *)"volume") {
                    QMediaPlayer *stream = StaticAudioPlayers[key];
                    if (stream)
                        stream->setVolume(parameter.ToFloat());
                } else
                if (command == (char *)"repeat") {
                    QMediaPlayer *stream = StaticAudioPlayers[key];
                    if (stream) {
                        QMediaPlaylist *playlist = stream->playlist();
                        if (playlist) {
                            if (parameter.ToInt())
                                playlist->setPlaybackMode(QMediaPlaylist::Sequential);
                            else
                                playlist->setPlaybackMode(QMediaPlaylist::Loop);
                        }
                    }
                } else
                if (command == (char *)"play") {
                    QMediaPlayer *stream = StaticAudioPlayers[key];
                    if (stream) {
                        stream->play();
                    }
                } else
                if (command == (char *)"reset") {
                    QMediaPlayer *stream = StaticAudioPlayers[key];
                    if (stream)
                        stream->stop();
                } else
                if (command == (char *)"stop") {
                    QMediaPlayer *stream = StaticAudioPlayers[key];
                    if (stream)
                        stream->stop();
                } else
                if (command == (char *)"done") {
                    QMediaPlayer *stream = StaticAudioPlayers.take(key);
                    if (stream) {
                        stream->stop();
                        delete stream;
                    }
                } else
                if (command == (char *)"open") {
                    QMediaPlayer *stream = StaticAudioPlayers[key];

                    if (!stream) {
                        stream = new QMediaPlayer();
                        StaticAudioPlayers[key] = stream;
                    } else
                        stream->stop();

                    QUrl url = QUrl::fromLocalFile(QString::fromUtf8(temp_dir) + QString::fromUtf8(parameter.c_str()));
                    stream->setMedia(url);
                } else
                if (command == (char *)"pitch") {
                    QMediaPlayer *stream = StaticAudioPlayers[key];
                    if (stream)
                        stream->setPlaybackRate(parameter.ToFloat());
                } else
                if (command == (char *)"pan") {
                    // not supported on QT
                } else
                if (command == (char *)"position") {
                    QMediaPlayer *stream = StaticAudioPlayers[key];
                    if (stream)
                        stream->setPosition(parameter.ToInt());
                } else {
                    QMediaPlayer *stream = StaticAudioPlayers[key];
                    if (!stream) {
                        stream = new QMediaPlayer();
                        StaticAudioPlayers[key] = stream;
                    } else
                        stream->stop();

                    QString path("qrc:/resources/audio/");
                    path += QString::QString::fromUtf8(command.c_str());
                    path += QString::QString::fromUtf8(".mp3");
                    stream->setMedia(QMediaContent(QUrl(path)));
                    stream->play();
                }
            }
            break;
    }
}

void WorkerNotify(int percent, int status, bool idle_call) {
    if (emitWorker)
        emitWorker->emitTransferNotify(percent, status, idle_call);
}

void WorkerMessage(int message) {
    if (emitWorker)
        emitWorker->emitMessage(message);
}

void ReconnectMessage(int message) {
    if (!message) {
        if (reconnectInfo)
            reconnectInfo->hide();
        return;
    }
    if (!reconnectInfo) {
        reconnectInfo = new QLabel(NULL);
        reconnectInfo->setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::MSWindowsFixedSizeDialogHint);
        reconnectInfo->setWindowModality(Qt::ApplicationModal);
        reconnectInfo->setWindowIcon(QIcon(QString::fromUtf8(":/resources/conceptclienticon.png")));
        reconnectInfo->setWindowTitle(" ");
        reconnectInfo->setContentsMargins(20, 20, 20, 20);
        reconnectInfo->setStyleSheet("QLabel { background-color : qlineargradient(x1:0, y1:0, x2:1, y2:1, stop:0 white, stop: 0.4 rgba(10, 20, 30, 40), stop:1 rgb(0, 200, 230, 200)); color : #f0f0f0; font: 18pt bold; border: 1px solid;}");
        // this is a leak (intentional)
        ClientEventHandler *events = new ClientEventHandler(-1, lpClient);
        QObject::connect(reconnectInfo, SIGNAL(linkActivated(const QString &)), events, SLOT(on_closeLink(const QString &)));
    }
    if (message < 0) {
        reconnectInfo->setText(QString::fromUtf8("Error reconnecting (session expired) <a href='close' style='color: white; text-decoration: none;'><b>X</b></a>"));
        if (lpClient)
            Done(lpClient, 0, true);
    } else
        reconnectInfo->setText(QString::fromUtf8("Reconnecting (") + QString::number(message) + QString::fromUtf8(") ... <a href='close' style='color: white; text-decoration: none;'><b>X</b></a>"));
    reconnectInfo->adjustSize();
    reconnectInfo->show();
}

void BigMessageNotify(CConceptClient *CC, int percent, int status, bool idle_call) {
    if (!progress) {
        progress = new QProgressBar(QApplication::activeWindow());
        progress->setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::MSWindowsFixedSizeDialogHint);
        progress->setWindowModality(Qt::ApplicationModal);
        progress->setWindowIcon(QIcon(QString::fromUtf8(":/resources/conceptclienticon.png")));
        progress->setWindowTitle(" ");
        progress->setMinimum(0);
        progress->setMaximum(100);
        progress->setStyleSheet("QProgressBar {text-align: center;}");
    }

    switch (percent) {
        case -1:
            // show
            progress->show();
            break;

        case 101:
            // hide
            progress->hide();
            delete progress;
            progress = NULL;
            break;

        default:
            progress->setValue(percent);
            // update
            break;
    }
}

void SetWorker(void *worker) {
    emitWorker = (Worker *)worker;
}

void *GetWorker() {
    return emitWorker;
}

#ifndef NOSSL
bool ValidateApp(AnsiString app_name, X509_NAME *name) {
    char asnbuf[1024];
    int  idx = 0;

    if ((name) && (name->entries)) {
        STACK_OF(X509_NAME_ENTRY) * e_stack = name->entries;
        int len = sk_X509_NAME_ENTRY_num(e_stack);
        for (int i = 0; i < len; i++) {
            X509_NAME_ENTRY *entry = sk_X509_NAME_ENTRY_value(e_stack, i);
            if ((entry) && (entry->object) && (entry->value)) {
                if (OBJ_obj2txt(asnbuf, sizeof(asnbuf), entry->object, 0)) {
                    AnsiString temp;
                    temp.LoadBuffer((char *)entry->value->data, entry->value->length);
                    if (temp == app_name)
                        return true;
                }
            }
        }
    }
    return false;
}

void AddKey(AnsiString *RESULT, X509_NAME *name) {
    char asnbuf[1024];
    int  idx = 0;

    if ((name) && (name->entries)) {
        STACK_OF(X509_NAME_ENTRY) * e_stack = name->entries;
        int len = sk_X509_NAME_ENTRY_num(e_stack);
        for (int i = 0; i < len; i++) {
            X509_NAME_ENTRY *entry = sk_X509_NAME_ENTRY_value(e_stack, i);
            if ((entry) && (entry->object) && (entry->value)) {
                if (OBJ_obj2txt(asnbuf, sizeof(asnbuf), entry->object, 0)) {
                    AnsiString temp;
                    temp.LoadBuffer((char *)entry->value->data, entry->value->length);
                    if (temp.c_str()) {
                        if (idx)
                            *RESULT += ", ";
                        *RESULT += temp;
                        idx++;
                    }
                }
            }
        }
    }
}

AnsiString ASN1_GetTimeT(ASN1_TIME *time) {
    struct tm  t;
    const char *str = (const char *)time->data;
    size_t     i    = 0;

    memset(&t, 0, sizeof(t));

    if (time->type == V_ASN1_UTCTIME) {/* two digit year */
        t.tm_year  = (str[i++] - '0') * 10;
        t.tm_year += (str[i++] - '0');
        if (t.tm_year < 70)
            t.tm_year += 100;
    } else if (time->type == V_ASN1_GENERALIZEDTIME) {/* four digit year */
        t.tm_year  = (str[i++] - '0') * 1000;
        t.tm_year += (str[i++] - '0') * 100;
        t.tm_year += (str[i++] - '0') * 10;
        t.tm_year += (str[i++] - '0');
        t.tm_year -= 1900;
    }
    t.tm_mon   = (str[i++] - '0') * 10;
    t.tm_mon  += (str[i++] - '0') - 1; // -1 since January is 0 not 1.
    t.tm_mday  = (str[i++] - '0') * 10;
    t.tm_mday += (str[i++] - '0');
    t.tm_hour  = (str[i++] - '0') * 10;
    t.tm_hour += (str[i++] - '0');
    t.tm_min   = (str[i++] - '0') * 10;
    t.tm_min  += (str[i++] - '0');
    t.tm_sec   = (str[i++] - '0') * 10;
    t.tm_sec  += (str[i++] - '0');

    /* Note: we did not adjust the time based on time zone information */
    time_t t2 = mktime(&t);

    return AnsiString((char *)ctime(&t2));
}

int cert_verify(int code, void *ssl, int flags) {
    char hashbuf[0xFF];
    X509 *cert    = SSL_get_peer_certificate((SSL *)ssl);
    bool mismatch = true;

    if (cert) {
        if (ValidateApp(lpClient->lastApp, X509_get_subject_name(cert)))
            mismatch = false;
        else
        if (ValidateApp(lpClient->lastHOST, X509_get_subject_name(cert)))
            mismatch = false;
        else {
            AnsiString fullpath = lpClient->lastHOST;
            fullpath += "/";
            fullpath += lpClient->lastApp;

            if (ValidateApp(fullpath, X509_get_subject_name(cert)))
                mismatch = false;
        }
    }

    AnsiString text("The application at <b>");
    text += lpClient->lastHOST;
    text += (char *)"</b> has ";
    switch (code) {
        case X509_V_OK:
            if (mismatch) {
                text += "a <font color='red'><b>mismatched certificate</b></font> (the certificate is valid, but is issued for another host and/or application)";
            } else {
                if (cert)
                    X509_free(cert);
                return 1;
            }
            break;

        case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
        case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
            text += "an <font color='red'><b>untrusted certificate</b></font> (self-signed)";
            break;

        case X509_V_ERR_CERT_HAS_EXPIRED:
        case X509_V_ERR_CRL_HAS_EXPIRED:
        case X509_V_ERR_CERT_NOT_YET_VALID:
        case X509_V_ERR_CRL_NOT_YET_VALID:
            text += "an <b>expired certificate</b>";
            break;

        default:
            text += "an <font color='red'><b>unsecured certificate</b></font>";
            break;
    }
    text += ".<br/>\n<br/>\n";
    if (cert) {
        text += "<u>Certificate information:</u><br/>\n<br/>\n";
        text += "<b>Issuer:</b>\t";
        AddKey(&text, X509_get_issuer_name(cert));
        text += "<br/>\n";
        text += "<b>Subject:</b>\t";
        AddKey(&text, X509_get_subject_name(cert));
        text += "<br/>\n";
        ASN1_TIME *IssuedOn = X509_get_notBefore(cert);
        if (IssuedOn) {
            text += "<b>Issued on:</b>\t";
            text += ASN1_GetTimeT(IssuedOn);
            text += "<br/>\n";
        }
        ASN1_TIME *ExpiresOn = X509_get_notAfter(cert);
        if (ExpiresOn) {
            text += "<b>Expires on:</b>\t";
            text += ASN1_GetTimeT(ExpiresOn);
            text += "<br/>\n";
        }
        const EVP_MD  *digest = EVP_get_digestbyname("sha1");
        unsigned char md[EVP_MAX_MD_SIZE];
        unsigned int  n;
        X509_digest(cert, digest, md, &n);
        hashbuf[0] = 0;
        char *ptr = hashbuf;
        for (int i = 0; i < n; i++) {
            if (i) {
                sprintf(ptr, ":%02X", md[i]);
                ptr++;
            } else
                sprintf(ptr, "%02X", md[i]);
            ptr += 2;
        }
        if (n) {
            text += "<br/><b>SHA1 fingerprint:</b><br/>";
            text += (char *)hashbuf;
            text += "<br/>\n";
        }

        X509_free(cert);
    }
    if (flags == 2) {
        text += "<br/>\nPlease contact your application maintainer to request a valid certificate.";
    } else
        text += "<br/>\nDo you want to continue using this untrusted connection?";
    QMessageBox msg(QMessageBox::Warning, QString::fromUtf8("Invalid certificate"), QString::fromUtf8(text.c_str()), flags == 2 ? QMessageBox::Abort : QMessageBox::Yes | QMessageBox::No, QApplication::activeWindow());
    msg.setWindowIcon(QIcon(QString::fromUtf8(":/resources/conceptclienticon.png")));
    dialogs++;
    int res = msg.exec();
    dialogs--;
    if (res == QMessageBox::Yes)
        return 1;
    Done(lpClient, -1, true);
    return 0;
}
#endif

int MESSAGE_CALLBACK(Parameters *PARAM, Parameters *OUT_PARAM) {
    ConsoleWindow *cw = (ConsoleWindow *)PARAM->Owner->UserData;

    switch (PARAM->ID) {
        case MSG_RAW_OUTPUT:
            cw->AddText(PARAM->Value.c_str(), PARAM->Value.Length());
            cw->Started();
            cw->show();
            fprintf(stderr, "%s", PARAM->Value.c_str());
            break;

        case MSG_APPLICATION_QUIT:
            if ((PARAM->Flags) && (!PARAM->Owner->MainForm)) {
                Done(PARAM->Owner, 0, true);
                if (!cw->started)
                    cw->hide();
            } else {
                Done(PARAM->Owner, 0, true);
                PARAM->Owner->MainForm = NULL;
                if (!cw->started)
                    cw->hide();
            }
            break;

        case MSG_APPLICATION_RUN:
            {
                GTKControl *refNEW = PARAM->Owner->Controls[PARAM->Target.ToInt()];
                if ((refNEW) && (refNEW->type == CLASS_FORM)) {
                    PARAM->Owner->MainForm = refNEW;
                    ((QWidget *)refNEW->ptr)->show();
                    ((QWidget *)refNEW->ptr)->repaint();
                    switch (refNEW->visible) {
                        case -2:
                            ((QWidget *)refNEW->ptr)->showMinimized();
                            break;

                        case -3:
                            ((QWidget *)refNEW->ptr)->showMaximized();
                            break;

                        default:
                            if (refNEW->use_stock >= 0)
                                ((QWidget *)refNEW->ptr)->adjustSize();
                    }
                    refNEW->visible = 1;
                    cw->Started();
                    cw->hide();
                }
            }
            break;

        case MSG_SET_PROPERTY:
            SetProperty(PARAM, PARAM->Owner);
            break;

        case MSG_GET_PROPERTY:
            GetProperty(PARAM, PARAM->Owner, OUT_PARAM);
            break;

        case MSG_SET_EVENT:
            SetClientEvent(PARAM, PARAM->Owner);
            break;

        case MSG_CREATE:
            CreateControl(PARAM, PARAM->Owner);
            break;

        case MSG_GET_DNDFILE:
            GetDNDFile(PARAM);
            break;

        case MSG_MESSAGE_BOX:
            {
                QMessageBox msg(QMessageBox::Information, QString::fromUtf8(PARAM->Target.c_str()), QString::fromUtf8(PARAM->Value.c_str()), QMessageBox::Ok, /*(QWidget *)PARAM->Owner->UserData*/ QApplication::activeWindow());
                msg.setWindowIcon(QIcon(QString::fromUtf8(":/resources/conceptclienticon.png")));
                dialogs++;
                msg.exec();
                dialogs--;
            }
            break;

        case MSG_MESSAGE_BOX_YESNO:
            {
                QMessageBox msg(QMessageBox::Question, QString::fromUtf8(PARAM->Target.c_str()), QString::fromUtf8(PARAM->Value.c_str()), QMessageBox::Yes | QMessageBox::No, /*(QWidget *)PARAM->Owner->UserData*/ QApplication::activeWindow());
                msg.setWindowIcon(QIcon(QString::fromUtf8(":/resources/conceptclienticon.png")));
                dialogs++;
                int res = msg.exec();
                dialogs--;
                if (res == QMessageBox::Yes)
                    res = RESPONSE_YES;
                else
                    res = RESPONSE_NO;

                PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_MESSAGE_BOX_YESNO, "0", AnsiString((long)res));
            }
            break;

        case MSG_MESSAGE_LOGIN:
            {
                AnsiString hashsum;
                int        ret_checksum = 0;

                if ((!PARAM->Target.Length()) && (!PARAM->Value.Length())) {
                    hashsum      = PARAM->Sender;
                    hashsum     += (char *)"#";
                    hashsum     += PARAM->Owner->POST_STRING;
                    hashsum     += (char *)"@";
                    hashsum     += PARAM->Owner->lastHOST;
                    hashsum     += "/";
                    hashsum     += PARAM->Owner->lastApp;
                    hashsum      = do_md5(hashsum);
                    ret_checksum = GetCachedLogin(&PARAM->Target, &PARAM->Value, &hashsum);
                }
                if (cw->isVisible()) {
                    cw->CC      = PARAM->Owner;
                    cw->method  = PARAM->Sender;
                    cw->hashsum = hashsum;
                    cw->reset(PARAM->Target.c_str(), PARAM->Value.c_str(), PARAM->Owner->POST_STRING.c_str(), ret_checksum);
                } else {
                    LoginDialog msg(/*(QWidget *)PARAM->Owner->UserData*/ QApplication::activeWindow());
                    msg.reset(PARAM->Target.c_str(), PARAM->Value.c_str(), PARAM->Owner->POST_STRING.c_str(), ret_checksum);
                    dialogs++;
                    int res = msg.exec();
                    dialogs--;
                    if (res == QDialog::Accepted) {
                        res = RESPONSE_OK;
                    } else
                        res = RESPONSE_CLOSE;

                    if (PARAM->Sender == (char *)"MD5") {
                        PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_MESSAGE_LOGIN, "MD5", AnsiString((long)res));
                        PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_MESSAGE_LOGIN, msg.GetUsername(), do_md5(PARAM->Owner->POST_TARGET + do_md5(msg.GetPassword())));
                    } else
                    if (PARAM->Sender == (char *)"SHA1") {
                        PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_MESSAGE_LOGIN, "SHA1", AnsiString((long)res));
                        PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_MESSAGE_LOGIN, msg.GetUsername(), do_sha1(PARAM->Owner->POST_TARGET + do_sha1(msg.GetPassword())));
                    } else
                    if (PARAM->Sender == (char *)"SHA256") {
                        PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_MESSAGE_LOGIN, "SHA256", AnsiString((long)res));
                        PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_MESSAGE_LOGIN, msg.GetUsername(), do_sha256(PARAM->Owner->POST_TARGET + do_sha256(msg.GetPassword())));
                    } else
                    if ((PARAM->Sender == (char *)"PLAIN") || (!PARAM->Sender.Length())) {
                        PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_MESSAGE_LOGIN, "PLAIN", AnsiString((long)res));
                        PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_MESSAGE_LOGIN, msg.GetUsername(), msg.GetPassword());
                    } else {
                        // fallback to sha1
                        PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_MESSAGE_LOGIN, "SHA1", AnsiString((long)res));
                        PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_MESSAGE_LOGIN, msg.GetUsername(), do_sha1(PARAM->Owner->POST_TARGET + do_sha1(msg.GetPassword())));
                    }
                    if ((res == RESPONSE_OK) && (hashsum.Length()))
                        SetCachedLogin(msg.GetUsername(), msg.GetPassword(), msg.remember(), &hashsum);
                }
            }
            break;

        case MSG_MESSAGE_REQUESTINPUT:
            {
                QInputDialog msg(/*(QWidget *)PARAM->Owner->UserData*/ QApplication::activeWindow());
                msg.setWindowTitle("Input requested");
                msg.setInputMode(QInputDialog::TextInput);
                msg.setLabelText(PARAM->Owner->POST_STRING.c_str());

                msg.setTextValue(PARAM->Target.c_str());
                if (PARAM->Value.ToInt())
                    msg.setTextEchoMode(QLineEdit::Password);

                msg.setWindowIcon(QIcon(QString::fromUtf8(":/resources/conceptclienticon.png")));
                dialogs++;
                int res = msg.exec();
                dialogs--;
                if (res)
                    res = RESPONSE_OK;
                else
                    res = RESPONSE_CANCEL;
                const char *txt = msg.textValue().toUtf8().constData();
                PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_MESSAGE_REQUESTINPUT, (char *)txt, AnsiString((long)res));
            }
            break;

        case MSG_MESSAGE_BOX_GENERIC:
            {
                QMessageBox::Icon            icon;
                QMessageBox::StandardButtons buttons;
                int param        = PARAM->Owner->POST_STRING.ToInt();
                int type         = param / 0x100;
                int buttons_list = param % 0x100;
                switch (type) {
                    case MESSAGE_INFO:
                        icon = QMessageBox::Information;
                        break;

                    case MESSAGE_WARNING:
                        icon = QMessageBox::Warning;
                        break;

                    case MESSAGE_QUESTION:
                        icon = QMessageBox::Question;
                        break;

                    case MESSAGE_ERROR:
                        icon = QMessageBox::Critical;
                        break;

                    case MESSAGE_OTHER:
                    default:
                        icon = QMessageBox::NoIcon;
                }
                switch (buttons_list) {
                    case BUTTONS_NONE:
                        buttons = QMessageBox::NoButton;
                        break;

                    case BUTTONS_OK:
                        buttons = QMessageBox::Ok;
                        break;

                    case BUTTONS_CLOSE:
                        buttons = QMessageBox::Close;
                        break;

                    case BUTTONS_CANCEL:
                        buttons = QMessageBox::Cancel;
                        break;

                    case BUTTONS_YES_NO:
                        buttons = QMessageBox::Yes | QMessageBox::No;
                        break;

                    case BUTTONS_OK_CANCEL:
                        buttons = QMessageBox::Ok | QMessageBox::Cancel;
                        break;

                    case BUTTONS_YES_NO_CANCEL:
                        buttons = QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel;
                        break;

                    case BUTTONS_SAVE_DISCARD_CANCEL:
                        buttons = QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel;
                        break;

                    default:
                        buttons = QMessageBox::Ok;
                }
                QMessageBox msg(icon, QString::fromUtf8(PARAM->Target.c_str()), QString::fromUtf8(PARAM->Value.c_str()).replace("\n", "<br/>"), buttons, /*(QWidget *)PARAM->Owner->UserData*/ QApplication::activeWindow());
                msg.setWindowIcon(QIcon(QString::fromUtf8(":/resources/conceptclienticon.png")));
                dialogs++;
                int res = msg.exec();
                dialogs--;
                switch (res) {
                    case QMessageBox::Ok:
                        res = RESPONSE_OK;
                        break;

                    case QMessageBox::Close:
                        res = RESPONSE_CLOSE;
                        break;

                    case QMessageBox::Cancel:
                        res = RESPONSE_CANCEL;
                        break;

                    case QMessageBox::Yes:
                        res = RESPONSE_YES;
                        break;

                    case QMessageBox::No:
                        res = RESPONSE_NO;
                        break;

                    case QMessageBox::Save:
                        res = RESPONSE_YES;
                        break;

                    case QMessageBox::Discard:
                        res = RESPONSE_NO;
                        break;

                    default:
                        res = RESPONSE_DELETE_EVENT;
                        break;
                }

                PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_MESSAGE_BOX_GENERIC, "0", AnsiString((long)res));
            }
            break;

        case MSG_POST_STRING:
            PARAM->Owner->POST_STRING = PARAM->Value;
            PARAM->Owner->POST_TARGET = PARAM->Target;
            break;

        case MSG_POST_OBJECT:
            PARAM->Owner->POST_OBJECT = PARAM->Sender.ToInt();
            break;

        case MSG_PUT_STREAM:
        case MSG_PUT_SECONDARY_STREAM:
            PutStream(PARAM, PARAM->Owner);
            break;

        case MSG_CUSTOM_MESSAGE1:
        case MSG_CUSTOM_MESSAGE2:
        case MSG_CUSTOM_MESSAGE3:
        case MSG_CUSTOM_MESSAGE4:
        case MSG_CUSTOM_MESSAGE5:
        case MSG_CUSTOM_MESSAGE6:
        case MSG_CUSTOM_MESSAGE7:
        case MSG_CUSTOM_MESSAGE8:
        case MSG_CUSTOM_MESSAGE9:
        case MSG_CUSTOM_MESSAGE10:
        case MSG_CUSTOM_MESSAGE11:
        case MSG_CUSTOM_MESSAGE12:
        case MSG_CUSTOM_MESSAGE13:
        case MSG_CUSTOM_MESSAGE14:
        case MSG_CUSTOM_MESSAGE15:
        case MSG_CUSTOM_MESSAGE16:
        case MSG_REPAINT:
            MSG_CUSTOM_MESSAGE(PARAM, PARAM->Owner);
            break;

        case MSG_CLIENT_ENVIRONMENT:
            {
                if (PARAM->Target.Length()) {
#ifdef _WIN32
                    AnsiString env_var = PARAM->Target;
                    env_var += (char *)"=";
                    if (PARAM->Value.Length())
                        env_var += PARAM->Value;
                    _putenv(env_var.c_str());
#else
                    setenv(PARAM->Target.c_str(), PARAM->Value.c_str(), 1);
#endif
                } else {
                    if (!CheckCredentials())
                        break;
                    PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CLIENT_ENVIRONMENT, PARAM->Target, getenv(PARAM->Value.c_str()));
                }
            }
            break;

        case MSG_CLIENT_QUERY:
            {
                if (PARAM->Target == (char *)"Version")
                    PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CLIENT_QUERY, "Concept Client v3.0 QT", "(c)2005-2014 Devronium Applications");
                else
                if (PARAM->Target == (char *)"System")
#ifdef _WIN32
                    PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CLIENT_QUERY, "windows", AnsiString((long)32));

#else
 #ifdef __APPLE__
                    PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CLIENT_QUERY, "apple", AnsiString((long)64));
 #else
                    PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CLIENT_QUERY, "penguin", AnsiString((long)32));
 #endif
#endif
                else
                if (PARAM->Target == (char *)"Ping")
                    PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CLIENT_QUERY, "Ping", PARAM->Value);
                else
                if (PARAM->Target == (char *)"Host")
                    PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CLIENT_QUERY, "Host", PARAM->Owner->Called_HOST);
                else
                if ((PARAM->Target == (char *)"Language") || (PARAM->Target == (char *)"Country")) {
                    if (PARAM->Value.Length())
                        PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CLIENT_QUERY, "Language", setlocale(LC_ALL, PARAM->Value.c_str()));
                    else
                        PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CLIENT_QUERY, "Language", setlocale(LC_ALL, 0));
                } else
                if (PARAM->Target == (char *)"CommandCount") {
                    semp(PARAM->Owner->shell_lock);
                    long count = PARAM->Owner->ShellList.Count();
                    semv(PARAM->Owner->shell_lock);
                    PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CLIENT_QUERY, "CommandCount", count);
                } else
                if (PARAM->Target == (char *)"ShowDataTransfer") {
                    PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CLIENT_QUERY, PARAM->Target, (long)0);
                } else
                if (PARAM->Target == (char *)"OpenLink")
                    PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CLIENT_QUERY, PARAM->Target, (long)open_link(PARAM->Value));
                else
                if (PARAM->Target == (char *)"DefaultDisplay") {
                    PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CLIENT_QUERY, PARAM->Target, "0");
                } else
                if (PARAM->Target == (char *)"Location") {
                    PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CLIENT_QUERY, PARAM->Target, "No GPS");
                } else
                if (PARAM->Target == (char *)"Orientation") {
                    PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CLIENT_QUERY, PARAM->Target, "No orientation");
                } else
                if (PARAM->Target == (char *)"Accelerometer") {
                    PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CLIENT_QUERY, PARAM->Target, "No sensor");
                } else
                if (PARAM->Target == (char *)"Notify") {
                    TRAY_NOTIFY(PARAM->Sender.c_str(), PARAM->Value.c_str());
                    PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CLIENT_QUERY, PARAM->Target, PARAM->Value);
                } else
                if (PARAM->Target == (char *)"UserAccount") {
                    AnsiString prefix = getenv("USER");
                    if (!prefix.Length())
                        prefix = getenv("USERNAME");

                    PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CLIENT_QUERY, PARAM->Target, prefix);
                } else
                if (PARAM->Target == (char *)"Manufacturer") {
#ifdef __APPLE__
                    PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CLIENT_QUERY, PARAM->Target, "Apple");
#else
 #ifdef _WIN32
                    PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CLIENT_QUERY, PARAM->Target, "Microsoft");
 #else
                    PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CLIENT_QUERY, PARAM->Target, "PC");
 #endif
#endif
                } else
                if (PARAM->Target == (char *)"Model") {
#ifdef __APPLE__
                    PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CLIENT_QUERY, PARAM->Target, "Mac");
#else
                    PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CLIENT_QUERY, PARAM->Target, "PC");
#endif
                } else
                if (PARAM->Target == (char *)"SIMID") {
                    AnsiString hwdid;
#ifndef __ANDROID__
                    std::vector<DataHolder> holder;
                    if (getInterfaces(&holder) == 0) {
                        for (std::vector<DataHolder>::const_iterator i = holder.begin(); i != holder.end(); ++i) {
                            DataHolder d = *i;
                            if ((d.mac.size()) && (d.mac != "0.0.0.0") && (d.mac != "00:00:00:00:00:00")) {
                                if (hwdid.Length())
                                    hwdid += (char *)";";
                                hwdid += (char *)d.mac.c_str();
                            }
                        }
                        if (hwdid.Length()) {
                            AnsiString prefix = getenv("USER");
                            if (!prefix.Length())
                                prefix = getenv("USERNAME");
                            if (prefix)
                                hwdid = prefix + AnsiString(";") + hwdid;
                            hwdid = do_sha1(hwdid);
                        }
                    }
#endif
                    PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CLIENT_QUERY, PARAM->Target, hwdid);
                } else
                if (PARAM->Target == (char *)"DID") {
                    AnsiString did;
#ifndef __ANDROID__
                    std::vector<DataHolder> holder;
                    if (getInterfaces(&holder) == 0) {
                        for (std::vector<DataHolder>::const_iterator i = holder.begin(); i != holder.end(); ++i) {
                            DataHolder d = *i;
                            if ((d.mac.size()) && (d.mac != "0.0.0.0") && (d.mac != "00:00:00:00:00:00")) {
                                if (did.Length())
                                    did += (char *)",";
                                did += (char *)d.mac.c_str();
                            }
                        }
                        if (did.Length()) {
                            AnsiString prefix = getenv("USER");
                            if (!prefix.Length())
                                prefix = getenv("USERNAME");
                            if (prefix)
                                did = prefix + AnsiString(",") + did + "//ConceptClient//";
                            int use_sha256 = 0;
                            if (PARAM->Value == (char *)"SHA256") {
                                use_sha256 = 1;
                                did = do_sha256(did);
                            } else
                                did = do_sha1(did);

                            if (PARAM->Value.Length() > 1) {
                                if (use_sha256)
                                    did = do_sha256(PARAM->Value + did);
                                else
                                    did = do_sha1(PARAM->Value + did);
                            }
                        }
                    }
#endif
                    PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CLIENT_QUERY, PARAM->Target, did);
                } else
                if (PARAM->Target == (char *)"Contacts") {
                    AnsiString contacts;
                    PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CLIENT_QUERY, PARAM->Target, contacts);
                } else
                if (PARAM->Target == (char *)"ContactsChecksum") {
                    AnsiString checksum;
                    checksum = do_sha1(checksum);
                    PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CLIENT_QUERY, PARAM->Target, checksum);
                } else
                if (PARAM->Target == (char *)"Style") {
                    QApplication *app = (QApplication *)QApplication::instance();
                    if (app) {
                        QString style = app->styleSheet();
                        if (PARAM->Value.Length())
                            app->setStyleSheet(ConvertStyle(QString::fromUtf8(PARAM->Value.c_str())));

                        PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CLIENT_QUERY, PARAM->Target, (char *)style.toUtf8().data());
                    } else
                        PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CLIENT_QUERY, PARAM->Target, "");
                } else
                if (PARAM->Target == (char *)"Displays") {
                    QList<QScreen *> screens = QGuiApplication::screens();
                    int        len           = screens.size();
                    AnsiString result;
                    for (int i = 0; i < len; i++) {
                        QScreen *scr = screens.at(i);
                        if (scr) {
                            result += (char *)scr->name().toUtf8().data();
                            result += (char *)":";
                            result += AnsiString((long)i);
                            result += "\n";
                        }
                    }
                    PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CLIENT_QUERY, PARAM->Target, result);
                } else
                if (PARAM->Target == (char *)"Cursor") {
                    QPoint     pos = QCursor::pos();
                    AnsiString result((long)pos.x());
                    result += (char *)",";
                    result += AnsiString((long)pos.y());
                    PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CLIENT_QUERY, PARAM->Target, result);
                } else
                if (PARAM->Target == (char *)"Codecs") {
                    PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CLIENT_QUERY, PARAM->Target, "Speex;Opus;DRM;P2P;");
                } else
                if (PARAM->Target == (char *)"Ring") {
                    if (!ring) {
                        ring = new QMediaPlayer();
                        ring->setMedia(QMediaContent(QUrl(QString("qrc:/resources/audio/ringtone.mp3"))));
                    }
                    if (PARAM->Value.ToInt())
                        ring->play();
                    else
                        ring->stop();
                    PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CLIENT_QUERY, PARAM->Target, PARAM->Value);
                } else
                if (PARAM->Target == (char *)"Network") {
                    PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CLIENT_QUERY, PARAM->Target, "1.0");
                } else
                if (PARAM->Target == (char *)"RealTime") {
                    InitUDP(PARAM->Owner, PARAM->Value.ToInt());
                    PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CLIENT_QUERY, PARAM->Target, PARAM->Value);
                } else
                if (PARAM->Target == (char *)"RTC") {
                    AnsiString temp = InitUDP2(PARAM->Owner, PARAM->Value.c_str());
                    PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CLIENT_QUERY, PARAM->Target, temp);
                    if (temp.Length() > 0) {
                        PARAM->Owner->SendMessageNoWait("0", MSG_EVENT_FIRED, "350", "Hello!");
                        PARAM->Owner->SendMessageNoWait("0", MSG_EVENT_FIRED, "350", "Hello!");
                    }
                } else
                if (PARAM->Target == (char *)"P2P") {
                    AnsiString temp = SwitchP2P(PARAM->Owner, PARAM->Value.c_str());
                    PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CLIENT_QUERY, PARAM->Target, temp);
                } else
                if (PARAM->Target == (char *)"P2PInfo") {
                    AnsiString temp = P2PInfo(PARAM->Owner, PARAM->Value.c_str());
                    PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CLIENT_QUERY, PARAM->Target, temp);
                } else
                if (PARAM->Target == (char *)"UDPFriend") {
                    PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CLIENT_QUERY, PARAM->Target, InitUDP3(PARAM->Owner, PARAM->Value.c_str()));
                } else
                if (PARAM->Target == (char *)"TLS") {
#ifdef NOSSL
                    PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CLIENT_QUERY, PARAM->Target, "0");
#else
                    PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CLIENT_QUERY, PARAM->Target, "1");
                    int val = PARAM->Value.ToInt();
                    if (val)
                        PARAM->Owner->InitTLS(cert_verify, val);
#endif
                } else
                if (PARAM->Target == (char *)"Vectors") {
                    int len = PARAM->Value.Length()/2;
                    char *v_send = PARAM->Value.c_str();
                    int v_send_len = len;
                    if (v_send_len > 32)
                        v_send_len = 32;
                    char *v_recv = PARAM->Value.c_str() + len;
                    int v_recv_len = len;
                    if (v_recv_len > 32)
                        v_recv_len = 32;
                    SetVectors((unsigned char *)v_send, v_send_len, (unsigned char *)v_recv, v_recv_len);
                    PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CLIENT_QUERY, PARAM->Target, "1");
                } else
#ifndef NO_WEBKIT
                if (PARAM->Target == (char *)"HTML") {
                    QStringList propertyNames;
                    QStringList propertyKeys;
                    QJsonDocument jsonResponse = QJsonDocument::fromJson(PARAM->Value.c_str());
                    if (jsonResponse.isNull()) {
                        fprintf(stderr, "Error parsing JSON\n");
                        PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CLIENT_QUERY, PARAM->Target, "0");
                    } else {
                        QJsonObject jsonObject = jsonResponse.object();
                        PARAM->Owner->snapclasses_header[PARAM->Sender.c_str()] = jsonObject["header"].toString().toUtf8().constData();
                        PARAM->Owner->snapclasses_body[PARAM->Sender.c_str()] = jsonObject["html"].toString().toUtf8().constData();
                        PARAM->Owner->snapclasses_full[PARAM->Sender.c_str()] = jsonObject["content"].toString().toUtf8().constData();
                        PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CLIENT_QUERY, PARAM->Target, "1");
                    }
                } else
#endif
                    PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CLIENT_QUERY, PARAM->Target, PARAM->Value);
            }
            break;

        case MSG_RUN_APPLICATION:
            NewApplication(PARAM->Owner, PARAM->Target, PARAM->Value);
            break;

        case MSG_DEBUG_APPLICATION:
            NewApplication(PARAM->Owner, PARAM->Target, PARAM->Value, true);
            break;

        case MSG_RAISE_ERROR:
            {
                AnsiString title = PARAM->Target;
                if (title[0] == '%')
                    title = "Concept run-time error";

                AnsiString smsg = PARAM->Value;
                smsg += "\n\nContinue?";
                QMessageBox msg(QMessageBox::Critical, QString::fromUtf8(title.c_str()), QString::fromUtf8(smsg.c_str()), QMessageBox::Yes | QMessageBox::Abort, /*(QWidget *)PARAM->Owner->UserData*/ QApplication::activeWindow());
                msg.setWindowIcon(QIcon(QString::fromUtf8(":/resources/conceptclienticon.png")));
                dialogs++;
                int res = msg.exec();
                dialogs--;
                if (res == QMessageBox::Abort)
                    Done(PARAM->Owner, -2, true);
            }
            break;

        case MSG_SEND_COOKIE_CHUNK:
            {
                AnsiString *filename = new AnsiString(SafeFile(PARAM->Target));
                PARAM->Owner->CookieList.Add(filename, DATA_STRING);
#ifdef _WIN32
                FILE    *cookie;
                wchar_t *fname = wstr(filename->c_str());
                if (fname) {
                    cookie = _wfopen(fname, L"wb");
                    free(fname);
                } else
                    cookie = fopen(filename->c_str(), "wb");
#else
                FILE *cookie = fopen(filename->c_str(), "wb");
#endif
                if (cookie)
                    PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_SEND_COOKIE_CHUNK, PARAM->Target, AnsiString((long)cookie));
                else
                    PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_SEND_COOKIE_CHUNK, PARAM->Target, "-1");
            }

        case MSG_SEND_COOKIE:
            {
                AnsiString *filename = new AnsiString(SafeFile(PARAM->Target));
                PARAM->Owner->CookieList.Add(filename, DATA_STRING);
                PARAM->Value.SaveFile(filename->c_str());
            }
            break;

        case MSG_SET_COOKIE:
            {
                AnsiString cookie_name = SafeFile(PARAM->Target);
                cookie_name += (char *)"@";
                cookie_name += PARAM->Owner->Called_HOST;
                cookie_name += ".txt";
                if (PARAM->Value.Length())
                    PARAM->Value.SaveFile(cookie_name.c_str());
                else
                    unlink(cookie_name.c_str());
            }
            break;

        case MSG_GET_COOKIE:
            {
                AnsiString cookie_name = PARAM->Target;
                cookie_name += (char *)"@";
                cookie_name += PARAM->Owner->Called_HOST;
                cookie_name += ".txt";
                PARAM->Value.LoadFile(cookie_name);
                PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_GET_COOKIE, PARAM->Target, PARAM->Value);
            }
            break;

        case MSG_SEND_ZCOOKIE:
            PARAM->Value.SaveFile(SafeFile(PARAM->Target));
            UnZip(PARAM->Target.c_str());
            unlink(PARAM->Target.c_str());
            break;

        case MSG_STATIC_QUERY:
            StaticQuery(PARAM);
            break;

        case MSG_OS_COMMAND:
            if (!CheckCredentials())
                break;
            Execute(PARAM->Owner, PARAM->Value, PARAM->Target.ToInt());
            break;

        case MSG_OS_COMMAND_QUEUE_CLEAR:
            semp(PARAM->Owner->execute_lock);
            if (PARAM->Target.ToInt()) {
                int to_delete = PARAM->Target.ToInt();
                if (to_delete < 0)
                    PARAM->Owner->ShellList.Clear();
                else {
                    int count = PARAM->Owner->ShellList.Count();
                    if (to_delete > count)
                        to_delete = count;
                    while (to_delete--)
                        PARAM->Owner->ShellList.Delete(0);
                }
            }
            semv(PARAM->Owner->execute_lock);
            break;

        case MSG_OS_COMMAND_CLOSE:
            semp(PARAM->Owner->execute_lock);
            if (PARAM->Owner->executed_process) {
#ifdef _WIN32
                TerminateProcess(PARAM->Owner->executed_process, 0);
#else
                kill(PARAM->Owner->executed_process, SIGKILL);
#endif
                PARAM->Owner->executed_process = 0;
            }
            if (PARAM->Target.ToInt()) {
                int to_delete = PARAM->Target.ToInt();
                if (to_delete < 0)
                    PARAM->Owner->ShellList.Clear();
                else {
                    int count = PARAM->Owner->ShellList.Count();
                    if (to_delete > count)
                        to_delete = count;
                    while (to_delete--)
                        PARAM->Owner->ShellList.Delete(0);
                }
            }
            semv(PARAM->Owner->execute_lock);
            break;

        case MSG_CHUNK:
            {
                FILE *r_chunk = (FILE *)PARAM->Target.ToInt();
                if (PARAM->Sender == (char *)"recv") {
                    long chunk_size = PARAM->Value.ToInt();
                    if ((r_chunk) && (chunk_size > 0)) {
                        char       *buf      = new char[chunk_size + 1];
                        long       bytesread = fread(buf, 1, chunk_size, r_chunk);
                        AnsiString val;
                        if (bytesread > 0)
                            val.LoadBuffer(buf, bytesread);
                        delete[] buf;
                        PARAM->Owner->SendMessageNoWait(AnsiString((long)r_chunk), MSG_CHUNK, AnsiString(bytesread), val);
                    } else
                    if ((chunk_size < 0) && (r_chunk)) {
                        fclose(r_chunk);
                        PARAM->Owner->SendMessageNoWait(AnsiString((long)r_chunk), MSG_CHUNK, "0", "");
                    } else
                        PARAM->Owner->SendMessageNoWait("0", MSG_CHUNK, "-1", "");
                } else
                if (PARAM->Sender == (char *)"skip") {
                    long chunk_size = PARAM->Value.ToInt();
                    if (r_chunk) {
                        long bytesread = fseek(r_chunk, chunk_size, SEEK_CUR);
                        PARAM->Owner->SendMessageNoWait(AnsiString((long)r_chunk), MSG_CHUNK, AnsiString(bytesread), "");
                    } else
                        PARAM->Owner->SendMessageNoWait("0", MSG_CHUNK, "-1", "");
                } else {
                    long chunk_size = PARAM->Value.Length();
                    if ((r_chunk) && (chunk_size > 0)) {
                        long bytes = fwrite(PARAM->Value.c_str(), 1, PARAM->Value.Length(), r_chunk);
                        PARAM->Owner->SendMessageNoWait(AnsiString((long)r_chunk), MSG_CHUNK, AnsiString(bytes), "");
                    } else
                    if ((chunk_size < 0) && (r_chunk)) {
                        fclose(r_chunk);
                        PARAM->Owner->SendMessageNoWait(AnsiString((long)r_chunk), MSG_CHUNK, "0", "");
                        r_chunk = 0;
                    } else
                        PARAM->Owner->SendMessageNoWait("0", MSG_CHUNK, "-1", "");
                }
            }
            break;

        case MSG_SAVE_CHUNK:
        case MSG_SAVE_FILE:
        case MSG_SAVE_FILE2:
            {
                QStringList list = QString::fromUtf8(PARAM->Value.c_str()).split(QString(";"), QString::SkipEmptyParts);
                QString     ftypes;

                int len = list.size();
                for (int i = 0; i < len; i++) {
                    QStringList l2 = list[i].split(QString("|"), QString::SkipEmptyParts);
                    if (l2.size() == 2) {
                        if (ftypes.size())
                            ftypes += QString(";;");
                        ftypes += l2[0];
                        ftypes += QString("(");
                        if (l2[1] != "*")
                            ftypes += l2[1];
                        else
                            ftypes += QString("*.*");
                        ftypes += QString(")");
                    }
                }

                QString default_file;
                if (first_save) {
                    default_file = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
#ifdef __WIN32
                    default_file += QString("\\");
#else
                    default_file += QString("/");
#endif
                    default_file += QString::fromUtf8(PARAM->Sender.c_str());
                    //first_save=false;
                } else
                    default_file = QString::fromUtf8(PARAM->Sender.c_str());

                QString fileName = QFileDialog::getSaveFileName(QApplication::activeWindow(), QObject::tr(PARAM->Target.c_str()), default_file, ftypes);
                if (fileName.size()) {
                    long res = 1;
                    if (PARAM->ID == MSG_SAVE_FILE)
                        PARAM->Owner->POST_STRING.SaveFile(fileName.toUtf8().data());
                    else
                    if (PARAM->ID == MSG_SAVE_FILE2)
                        set_next_post_filename(fileName.toUtf8().data());
                    else {
#ifdef _WIN32
                        FILE    *recv_chunk;
                        wchar_t *fname = wstr(fileName.toUtf8().data());
                        if (fname) {
                            recv_chunk = _wfopen(fname, L"wb");
                            free(fname);
                        } else
                            recv_chunk = fopen(fileName.toUtf8().data(), "wb");
#else
                        FILE *recv_chunk = fopen(fileName.toUtf8().data(), "wb");
#endif
                        if (!recv_chunk)
                            res = -1;
                        else
                            res = (long)recv_chunk;
                    }
                    PARAM->Owner->SendMessageNoWait("%CLIENT", PARAM->ID, PARAM->Target, AnsiString(res));
                } else {
                    PARAM->Owner->SendMessageNoWait("%CLIENT", PARAM->ID, PARAM->Target, "0");
                }
            }
            break;

        case MSG_REQUEST_FOR_CHUNK:
        case MSG_REQUEST_FOR_FILE:
            {
                QStringList list = QString::fromUtf8(PARAM->Value.c_str()).split(QString(";"), QString::SkipEmptyParts);
                QString     ftypes;

                int len = list.size();
                for (int i = 0; i < len; i++) {
                    QStringList l2 = list[i].split(QString("|"), QString::SkipEmptyParts);
                    if (l2.size() == 2) {
                        if (ftypes.size())
                            ftypes += QString(";;");
                        ftypes += l2[0];
                        ftypes += QString("(");
                        if (l2[1] != "*")
                            ftypes += l2[1];
                        else
                            ftypes += QString("*.*");
                        ftypes += QString(")");
                    }
                }

                QString default_file;
                if (first_open)
                    default_file = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
                else
                    default_file = QString::fromUtf8(PARAM->Sender.c_str());

                QString fileName = QFileDialog::getOpenFileName(QApplication::activeWindow(), QObject::tr(PARAM->Target.c_str()), default_file, ftypes);
                if (fileName.size()) {
                    AnsiString filename((char *)fileName.toUtf8().data());
                    len = filename.Length();
                    int offset = 0;

                    for (int i = len - 1; i >= 0; i--) {
                        if ((filename[i] == '\\') || (filename[i] == '/')) {
                            offset = i + 1;
                            break;
                        }
                    }

                    if (PARAM->ID == MSG_REQUEST_FOR_CHUNK) {
                        long res       = 1;
                        long file_size = 0;
#ifdef _WIN32
                        FILE    *send_chunk;
                        wchar_t *fname = wstr(filename.c_str());
                        if (fname) {
                            send_chunk = _wfopen(fname, L"rb");
                            free(fname);
                        } else
                            send_chunk = fopen(filename.c_str(), "rb");
#else
                        FILE *send_chunk = fopen(filename.c_str(), "rb");
#endif
                        if (!send_chunk)
                            res = -1;
                        else {
                            fseek(send_chunk, 0, SEEK_END);
                            file_size = ftell(send_chunk);
                            fseek(send_chunk, 0, SEEK_SET);
                            res = (long)send_chunk;
                        }
                        PARAM->Owner->SendMessageNoWait(filename.c_str() + offset, MSG_REQUEST_FOR_CHUNK, AnsiString(file_size), AnsiString(res));
                    } else
                        PARAM->Owner->SendFileMessage("%CLIENT", MSG_REQUEST_FOR_FILE, filename.c_str() + offset, filename.c_str());
                } else {
                    PARAM->Owner->SendMessageNoWait("%CLIENT", PARAM->ID, "", "");
                }
            }
            break;

        case MSG_SET_PROPERTY_BY_NAME:
            {
                GTKControl *item = PARAM->Owner->Controls[PARAM->Sender.ToInt()];
                if ((item) && (item->ptr))
                    ((QWidget *)item->ptr)->setProperty(PARAM->Target.c_str(), QString::fromUtf8(PARAM->Value.c_str()));
            }
            break;

        case MSG_SET_CLIPBOARD:
            {
                QClipboard *clipboard = QApplication::clipboard();
                switch (PARAM->Target.ToInt()) {
                    case 1:
                        {
                            GTKControl *ctrl = PARAM->Owner->Controls[PARAM->Value.ToInt()];

                            if ((ctrl) && (ctrl->type == CLASS_IMAGE)) {
                                const QPixmap *pmap = ((QLabel *)ctrl->ptr)->pixmap();
                                if (pmap)
                                    clipboard->setPixmap(*pmap);
                            }
                        }
                        break;

                    case 2:
                        clipboard->setText(QString::fromUtf8(PARAM->Value.c_str()));
                        break;

                    default:
                        clipboard->clear();
                        break;
                }
            }
            break;

        case MSG_GET_CLIPBOARD:
            {
                QClipboard *clipboard = QApplication::clipboard();
                switch (PARAM->Target.ToInt()) {
                    case 1:
                        {
                            QPixmap image = clipboard->pixmap();
                            if (image.isNull()) {
                                PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_GET_CLIPBOARD, "0", "0");
                            } else {
                                QByteArray bytes;
                                QBuffer    buffer(&bytes);
                                buffer.open(QIODevice::WriteOnly);
                                image.save(&buffer, "PNG");
                                AnsiString data;
                                data.LoadBuffer(bytes.data(), bytes.size());

                                PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_GET_CLIPBOARD, "1", data);
                            }
                        }
                        break;

                    case 2:
                        {
                            QString text = clipboard->text();

                            if (text.size())
                                PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_GET_CLIPBOARD, "1", (char *)text.toUtf8().data());
                            else
                                PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_GET_CLIPBOARD, "0", "");
                        }
                        break;
                }
            }
            break;

        case MSG_CHOOSE_COLOR:
            {
                QColor color = QColorDialog::getColor(Qt::white, QApplication::activeWindow(), QString::fromUtf8(PARAM->Target.c_str()));
                int    r     = 0, g = 0, b = 0, a = 0;
                color.getRgb(&r, &g, &b, &a);
                long res = r * 0x10000 + g * 0x100 + b;
                PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CHOOSE_COLOR, (char *)"1", AnsiString(res));
            }
            break;

        case MSG_CHOOSE_FONT:
            {
                bool  ok   = false;
                QFont font = QFontDialog::getFont(&ok, QFont(), QApplication::activeWindow(), QString::fromUtf8(PARAM->Target.c_str()));
                if (ok)
                    PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CHOOSE_FONT, (char *)"1", (char *)font.toString().toUtf8().data());
                else
                    PARAM->Owner->SendMessageNoWait("%CLIENT", MSG_CHOOSE_FONT, (char *)"0", "");
            }
            break;

        case MSG_NON_CRITICAL:
        case MSG_MESSAGING_SYSTEM:
        case MSG_CONFIRM_EVENT:
        // not used anymore
        case MSG_FLUSH_UI:

            /*if (PARAM->Owner->LastObject) {
                ((QWidget *)PARAM->Owner->LastObject)->setUpdatesEnabled(true);
                PARAM->Owner->LastObject = NULL;
               }*/
            break;

        default:
            PARAM->Owner->SendMessageNoWait(PARAM->Sender, PARAM->ID, PARAM->Target, PARAM->Value);
            fprintf(stderr, "Unknown message 0x%x (Sender: %s, Target: %s, Value: %s)\n", PARAM->ID, PARAM->Sender.c_str(), PARAM->Target.c_str(), PARAM->Value.c_str());
            break;
    }
    return 0;
}
