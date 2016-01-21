#ifndef __CALLBACK_H
#define __CALLBACK_H

#include "ConceptClient.h"
#include <string>

#include <QWidget>
#include <QIcon>
#include <QFrame>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QTabWidget>
#include <QGridLayout>
#include <QMessageBox>
#include <QInputDialog>
#include <QCheckBox>
#include <QRadioButton>
#include <QProgressBar>
#include <QSplitter>
#include <QScrollArea>
#include <QSlider>
#include <QScrollBar>
#include <QSpinBox>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QCalendarWidget>
#include <QToolBox>
#include <QToolButton>
#include <QComboBox>
#include <QTreeView>
#include <QTreeWidget>
#include <QListWidget>
#include <QHeaderView>
#include <QListView>
#include <QTableWidget>
#include <QTextEdit>
#ifndef NO_WEBKIT
    #include <QWebView>
#endif
#include <QGraphicsScene>
#include <QApplication>
#include <QDesktopWidget>
#include <QScreen>
#include <QGroupBox>
#include <QDrag>
#include <QMimeData>
#include <QFileDialog>
#ifndef NO_PRINT
#include <QPrintDialog>
#include <QPrinter>
#endif
#include <QBitmap>
#include <QToolTip>
#include <QCompleter>
#include <QButtonGroup>
#include <QActionGroup>
#include <QTimer>
#include <QBuffer>
#include <QStandardPaths>

#define MAX_CONTROLS     0xFF
#ifdef _WIN32
 #define _WIN32_WINNT    0x0501
 #include <windows.h>
#else
 #include <dlfcn.h>
 #define HMODULE    void *
 #define _unlink    unlink
#endif
#include "components_library.h"

#define RESPONSE_NONE                  -1
#define RESPONSE_REJECT                -2
#define RESPONSE_ACCEPT                -3
#define RESPONSE_DELETE_EVENT          -4
#define RESPONSE_OK                    -5
#define RESPONSE_CANCEL                -6
#define RESPONSE_CLOSE                 -7
#define RESPONSE_YES                   -8
#define RESPONSE_NO                    -9
#define RESPONSE_APPLY                 -10
#define RESPONSE_HELP                  -11

#define MESSAGE_INFO                   0
#define MESSAGE_WARNING                1
#define MESSAGE_QUESTION               2
#define MESSAGE_ERROR                  3
#define MESSAGE_OTHER                  4

#define BUTTONS_NONE                   0
#define BUTTONS_OK                     1
#define BUTTONS_CLOSE                  2
#define BUTTONS_CANCEL                 3
#define BUTTONS_YES_NO                 4
#define BUTTONS_OK_CANCEL              5

#define BUTTONS_YES_NO_CANCEL          6
#define BUTTONS_SAVE_DISCARD_CANCEL    7


typedef void (*UPDATE_CB)(bool init);

int MESSAGE_CALLBACK(Parameters *PARAM, Parameters *OUT_PARAM = NULL);
int LocalInvoker(void *context, int INVOKE_TYPE, ...);
int MESSAGE_INIT(CConceptClient *CC, char *path, char *host, UPDATE_CB ustatus, AnsiString& file);
void SetCookiesDir(AnsiString dir);
std::string UriDecode(const std::string& sSrc);
std::string UriEncode(const std::string& sSrc);

AnsiString do_md5(AnsiString str);
AnsiString do_sha1(AnsiString str);
int SetCachedLogin(AnsiString username, AnsiString password, int flag, AnsiString *hash);
int is_ipv6(char *str, int len, AnsiString *replaced_host = NULL);
void Done(CConceptClient *CC, int res = 0, bool forced = false);
void BigMessageNotify(CConceptClient *CC, int how_much, int message, bool idle_call);
void ReconnectMessage(int message);
void WorkerNotify(int how_much, int message, bool idle_call);
void WorkerMessage(int message);
void SetWorker(void *worker);
void *GetWorker();
CConceptClient *GetClient();
AnsiString ItemToPath(QTreeWidget *widget, QTreeWidgetItem *item);
void LoadControls(void *CC, const char *dir);
int LocalInvoker(void *context, int INVOKE_TYPE, ...);
int open_link(AnsiString param);

#ifdef _WIN32
void WIN32Notify(char *title, char *text);
#endif
#endif
