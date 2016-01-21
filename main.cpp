#include <QApplication>
#include <QSplashScreen>
#include <QPushButton>
#include <QLabel>
#include <QMessageBox>
#include <QThread>

#include "connectdialog.h"
#include "consolewindow.h"
#include "worker.h"

#include "AnsiString.h"
#include "ConceptClient.h"
#include "callback.h"

#include "MacOSXApplication.h"
#ifdef __APPLE__
 #include <mach-o/dyld.h>
#endif
#include <sys/stat.h>
#include <sys/types.h>

#define UPDATED_TXT    "plugins/update.msg"

#ifdef _WIN32
 #define COOKIES       "cookies\\"
 #define COMPONENTS    "components\\"
#else
 #define COOKIES       ".ConceptClient-cookies/"
 #define COMPONENTS    "components/"
#endif

AnsiString GetDirectory() {
#ifdef _WIN32
    char buffer[4096];
    buffer[0] = 0;
    GetModuleFileNameA(NULL, buffer, 4096);
    for (int i = strlen(buffer) - 1; i >= 0; i--)
        if ((buffer[i] == '/') || (buffer[i] == '\\')) {
            buffer[i + 1] = 0;
            break;
        }
    return AnsiString(buffer);
#else
 #ifdef __APPLE__
    char buffer[4096];
    buffer[0] = 0;
    unsigned int size = 4096;
    _NSGetExecutablePath(buffer, &size);
    for (int i = strlen(buffer) - 1; i >= 0; i--)
        if ((buffer[i] == '/') || (buffer[i] == '\\')) {
            buffer[i + 1] = 0;
            break;
        }
    return AnsiString(buffer);
 #else
    return AnsiString(APP_PATH);
 #endif
#endif
}

#ifdef _WIN32
int NotifyUpdate(AnsiString path) {
    AnsiString update_msg;

    update_msg.LoadFile(path);
    char  strValue[0xFF];
    DWORD dwLength = 0xFF;
    DWORD dwType   = 0;
    if (update_msg.Length()) {
        HKEY       hk;
        DWORD      dwDisp = 0;
        AnsiString msg    = update_msg;

        RegCreateKeyExA(HKEY_CURRENT_USER, "Software\\Devronium\\ConceptClient", 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE | KEY_QUERY_VALUE, NULL, &hk, &dwDisp);

        LONG res = RegQueryValueExA(hk, "LastNotify", NULL, NULL, (LPBYTE)strValue, &dwLength);
        if ((res != ERROR_SUCCESS) || (msg != strValue)) {
            RegSetValueExA(hk, "LastNotify", 0, REG_SZ, (LPBYTE)(LPTSTR)msg.c_str(), msg.Length() + 1);
            WIN32Notify("Concept://Client update", update_msg.c_str());
        }
        RegCloseKey(hk);
        return 1;
    }
    return 0;
}
#endif

void dialog_error(const char *title, const char *secondary) {
    QMessageBox msg(NULL);

    msg.setWindowTitle(title);
    msg.setText(secondary);
    msg.setWindowIcon(QIcon(QString::fromUtf8(":/icon.ico")));
    msg.setIcon(QMessageBox::Warning);
    msg.exec();
}

AnsiString GetCookiesPath() {
#ifdef _WIN32
    char tmp[8192];
    tmp[0] = 0;
    GetTempPathA(8192, tmp);
    AnsiString path(tmp);

    if (!path.Length())
        path = (char *)getenv("TEMP");

    if (!path.Length())
        path = (char *)getenv("TMP");
    //fprintf(stderr, "Initializing with temp directory: %s\n", path.c_str());
    AnsiString full_path = path + "\\";
    full_path += COOKIES;
    int len = full_path.Length();
    if ((len) && ((full_path[len - 1] == '\\') || (full_path[len - 1] == '/'))) {
        AnsiString path2 = full_path;
        path2.c_str()[len - 1] = 0;
        mkdir(full_path.c_str());
    } else
        mkdir(full_path);

    if (!full_path.Length()) {
        dialog_error("Error initializing Concept Client", "Could not identify a temporary folder for storing cookies. Please make sure that TEMP/TMP/HOME environment variables are set.");
        exit(0);
    }
    return full_path;
#else
 #ifdef __APPLE__
    AnsiString full_path(getenv("HOME"));
    full_path += "/";
    full_path += COOKIES;
    mkdir(full_path, 0777L);
    return full_path;
 #else
    return COOKIES;
 #endif
#endif
}

void MiscInit() {
#ifdef _WIN32
    CConceptClient::path = GetDirectory();
#else
 #ifdef __APPLE__
    CConceptClient::path = GetDirectory();
 #endif
#endif
}

long C_GetString(const char *lpAppName, const char *lpKeyName, const char *lpDefault, char *lpReturnedString, long nSize, const char *lpFileName) {
    FILE *in         = 0;
    long filesize    = 0;
    char *filebuffer = 0;
    char section_name[4096];
    char key_name[4096];
    int  error = 0;

    lpReturnedString[0] = 0;
    in = fopen(lpFileName, "rb");
    if (in) {
        fseek(in, 0L, SEEK_END);
        filesize = ftell(in);
        fseek(in, 0L, SEEK_SET);
        if (filesize) {
            filebuffer = new char[filesize];
            if (filebuffer) {
                if (fread(filebuffer, 1, filesize, in) == filesize) {
                    char in_name           = 0;
                    char in_quote          = 0;
                    char in_looked_section = 0;
                    char in_looked_key     = 0;
                    char in_looked_value   = 0;
                    char in_comment        = 0;
                    char in_key            = 0;
                    int  temp_pos          = 0;
                    int  key_pos           = 0;
                    int  value_pos         = 0;

                    section_name[0] = 0;
                    key_name[0]     = 0;

                    int i;
                    for (i = 0; i < filesize; i++) {
                        if (in_comment) {
                            if ((filebuffer[i] == '\n') || (filebuffer[i] == '\r'))
                                in_comment = 0;
                            continue;
                        }

                        if (((filebuffer[i] == '\n') || (filebuffer[i] == '\r')) && (in_looked_value)) {
                            lpReturnedString[value_pos++] = 0;
                            break;
                        }

                        if (!in_quote) {
                            if ((filebuffer[i] == ';') || (filebuffer[i] == '#')) {
                                if (in_looked_value)
                                    break;
                                in_comment = 1;
                                continue;
                            }

                            if (filebuffer[i] == '[') {
                                // a inceput o sectiune noua, iar sectiunea precedenta este cea cautata ...
                                if (in_looked_section) {
                                    error = 1;
                                    break;
                                }
                                temp_pos = 0;
                                in_name  = 1;
                                continue;
                            }
                            if (in_name) {
                                if (filebuffer[i] == ']') {
                                    in_name = 0;
                                    section_name[temp_pos] = 0;
                                    if (!strcmp(section_name, lpAppName))
                                        in_looked_section = 1;
                                    continue;
                                }
                                if ((filebuffer[i] == '\n') || (filebuffer[i] == '\r')) {
                                    in_name = 0;
                                    continue;
                                }
                                if (in_name)
                                    section_name[temp_pos++] = filebuffer[i];
                            }
                        }
                        // schimb flag-ul de ghilimele, daca le-am gasit ...
                        if (!in_name) {
                            if (filebuffer[i] == '"') {
                                in_quote = !in_quote;
                                continue;
                            }
                            if (((filebuffer[i] == '\n') || (filebuffer[i] == '\r')) && (in_quote)) {
                                in_quote = 0;
                                continue;
                            }
                        }

                        if (in_looked_key) {
                            if (filebuffer[i] == '=') {
                                in_looked_value = 1;
                                continue;
                            }
                            if (in_looked_value) {
                                if (value_pos < nSize - 1) {
                                    if (value_pos)
                                        lpReturnedString[value_pos++] = filebuffer[i];
                                    else if ((filebuffer[i] != ' ') && (filebuffer[i] != '\t'))
                                        lpReturnedString[value_pos++] = filebuffer[i];
                                } else {
                                    lpReturnedString[value_pos++] = 0;
                                    break;
                                }
                                continue;
                            }
                        }

                        if (in_looked_section) {
                            if (((filebuffer[i] >= '0') && (filebuffer[i] <= '9')) ||
                                ((filebuffer[i] >= 'a') && (filebuffer[i] <= 'z')) ||
                                ((filebuffer[i] >= 'A') && (filebuffer[i] <= 'Z')) ||
                                (filebuffer[i] == '_')) {
                                if (in_key == 0) {
                                    key_pos = 0;
                                    in_key  = 1;
                                }
                                key_name[key_pos++] = filebuffer[i];
                            } else if (in_key) {
                                key_name[key_pos] = 0;
                                if (!strcmp(key_name, lpKeyName))
                                    in_looked_key = 1;
                                in_key = 0;
                            }
                            // TO DO ... sunt in sectiunea cautata ...
                        }
                    }
                    // in caz ca e ultima linie din fisier ...
                    if ((i == filesize) && (in_looked_value))
                        lpReturnedString[value_pos++] = 0;
                } else
                    error = 1;
            } else
                error = 1;
        } else
            error = 1;
        fclose(in);
    } else {
        fprintf(stderr, "Warning: Error reading from %s\n", lpFileName);
        error = 1;
    }

    if (error)
        strncpy(lpReturnedString, lpDefault, nSize);
    if (filebuffer)
        delete[] filebuffer;
    return (long)in;
}

//-----------------------------------------------------------------------------
int GetKey(char *value, char *ini_name, const char *section, const char *key, const char *def) {
    return C_GetString(section, key, def, value, 1024, ini_name);
}

//-----------------------------------------------------------------------------------------
int ParseHost(char *file, char *buffer2, int& used_port) {
    int used_port_set = 0;

    used_port = DEFAULT_PORT;
    char is_secured = 0;
    char is_http    = 0;
    char buffer_port[8192];
    int  len = 0;

    if (file[0])
        len = strlen(file);

    int state = 0;

    int buffer2_index = 0;
    int port_index    = 0;
    int in_ipv6       = 0;
    int ipv6          = is_ipv6(file, strlen(file));

    if (len > 8192)
        len = 8192;

    for (int i = 0; i < len; i++) {
        switch (state) {
            case 0:
                if ((file[i] == '\\') || (file[i] == '/'))
                    state = 2;
                else
                if ((file[i] == ':') && (!in_ipv6) && (!ipv6))
                    state = 1;
                else
                if (file[i] == '[')
                    in_ipv6 = 1;
                else
                if ((file[i] == ']') && (in_ipv6)) {
                    in_ipv6 = 0;
                    ipv6    = 0;
                } else
                    buffer2[buffer2_index++] = file[i];
                break;

            case 1:
                if ((file[i] >= '0') && (file[i] <= '9'))
                    buffer_port[port_index++] = file[i];
                else
                if ((file[i] == '\\') || (file[i] == '/'))
                    state = 2;
                break;

            case 2:
                break;
        }
    }

    buffer2[buffer2_index]  = 0;
    buffer_port[port_index] = 0;

    if (buffer_port[0]) {
        used_port = AnsiString(buffer_port).ToInt();
        if (!used_port)
            used_port = is_http ? DEFAULT_HTTP_PORT : DEFAULT_PORT;
        else
            used_port_set = 1;
    }
}

//-----------------------------------------------------------------------------------------
int LoadFromFile(char *file, char *buffer2, char *buffer3, char *buffer4, int& used_port) {
    char buffer5[0xFFF];
    int  used_port_set = 0;

    used_port = DEFAULT_PORT;

#ifdef PRINT_STATUS
    std::cerr << "Using shortcut " << file << "\n";
#endif
    char buffer_port[8192];
    char is_secured = 0;
    char is_http    = 0;

    if (strstr(file, "concept:") == file)
        file += 8;

    if (strstr(file, "concepts:") == file) {
        file      += 9;
        is_secured = 1;
    }

    if (strstr(file, "http:") == file) {
        is_http = 1;
        file   += 5;
    }

    if (strstr(file, "https:") == file) {
        file   += 6;
        is_http = 1;
    }

    if (file[0] == '"')
        file++;
    else
    if (((file[0] == '/') && (file[1] == '/')) || ((file[0] == '\\') && (file[1] == '\\'))) {
        file += 2;

        int len = 0;

        if (file[0])
            len = strlen(file);

        int state = 0;

        int buffer2_index = 0;
        int buffer3_index = 0;
        int port_index    = 0;
        int in_ipv6       = 0;
        int ipv6          = is_ipv6(file, strlen(file));

        if (len > 8192)
            len = 8192;

        for (int i = 0; i < len; i++) {
            switch (state) {
                case 0:
                    if ((file[i] == '\\') || (file[i] == '/'))
                        state = 2;
                    else
                    if ((file[i] == ':') && (!in_ipv6) && (!ipv6))
                        state = 1;
                    else
                    if (file[i] == '[')
                        in_ipv6 = 1;
                    else
                    if ((file[i] == ']') && (in_ipv6)) {
                        in_ipv6 = 0;
                        ipv6    = 0;
                    } else
                        buffer2[buffer2_index++] = file[i];
                    break;

                case 1:
                    if ((file[i] >= '0') && (file[i] <= '9'))
                        buffer_port[port_index++] = file[i];
                    else
                    if ((file[i] == '\\') || (file[i] == '/'))
                        state = 2;
                    break;

                case 2:
                    buffer3[buffer3_index++] = file[i];
                    break;
            }
        }

        buffer2[buffer2_index]  = 0;
        buffer3[buffer3_index]  = 0;
        buffer_port[port_index] = 0;

        if (buffer_port[0]) {
            AnsiString temp_port(buffer_port);
            used_port = temp_port.ToInt();
            if (!used_port)
                used_port = is_http ? DEFAULT_HTTP_PORT : DEFAULT_PORT;
            else
                used_port_set = 1;
        }

        if (is_secured)
            strcpy(buffer4, "secured");
        else
        if (is_http)
            strcpy(buffer4, "http");
        else
            buffer4[0] = 0;
    } else {
        int len = strlen(file);
        if ((len) && (file[len - 1] == '"'))
            file[len - 1] = 0;

        GetKey(buffer2, file, "Shortcut", "Host", "localhost");
        GetKey(buffer3, file, "Shortcut", "Application", "start.con");
        GetKey(buffer4, file, "Shortcut", "Secured", "");
        GetKey(buffer5, file, "Shortcut", "UseHttp", "0");


        AnsiString def_port((long)DEFAULT_PORT);
        GetKey(buffer_port, file, "Shortcut", "Port", def_port.c_str());
        used_port = 0;
        if (buffer_port[0]) {
            AnsiString s((char *)buffer_port);
            used_port = s.ToInt();
        }
        if (used_port <= 0)
            used_port = DEFAULT_PORT;
        else
            used_port_set = 1;

        if ((buffer4[0] == '1') || (buffer4[0] == 'Y') || (buffer4[0] == 'y') || (buffer4[0] == 'T') || (buffer4[0] == 't'))
            strcpy(buffer4, "secured");
        else
        if ((buffer5[0] == '1') || (buffer5[0] == 'Y') || (buffer5[0] == 'y') || (buffer5[0] == 'T') || (buffer5[0] == 't'))
            strcpy(buffer4, "http");
        else
            buffer4[0] = 0;
    }
    return 0;
}
//-----------------------------------------------------------------------------------------
bool osx_open_file(const char *fname, void *user_data) {
    int port = DEFAULT_PORT;
    char host[1024];
    char app[1024];
    char sec_buf[1024];
    AnsiString filename((char *)fname);
    LoadFromFile(filename.c_str(), host, app, sec_buf, port);
    AnsiString secured  = sec_buf;
    int is_secured = (int)(secured == (char *)"secured");

    AnsiString address;
    if (is_secured)
        address = (char *)"concepts://";
    else
        address = (char *)"concept://";

    address += host;
    address += (char *)"/";
    address += app;

    if (user_data) {
        MacOSXStep *step = (MacOSXStep *)user_data;
        step->data = address;
        if ((step->step == 1) && (step->userdata)) {
            ConnectDialog *connect = (ConnectDialog *)step->userdata;
            connect->setText(address.c_str());
            connect->setResult(QDialog::Accepted);
            connect->accept();
            return false;
        } else
        if (step->step == 0) {
            step->step = 2;
            return false;
        }
    }
    open_link(address);
    return false;
}
//-----------------------------------------------------------------------------------------
int main(int argc, char **argv) {
    int is_secured = 0;
    MiscInit();
#ifdef _WIN32
    char szPath[MAX_PATH];
    GetModuleFileNameA(NULL, szPath, MAX_PATH);
    CConceptClient::ConceptClientPath = szPath;
#else
#ifdef __APPLE__
    char buffer[4096];
    buffer[0] = 0;
    unsigned int size = 4096;
    if (!_NSGetExecutablePath(buffer, &size))
        CConceptClient::ConceptClientPath = buffer;
    else
        CConceptClient::ConceptClientPath = "conclient";
#else
    CConceptClient::ConceptClientPath = APP_PATH "ConceptClient";
#endif
#endif
    
#ifdef __APPLE__
    MacOSXStep osxstep;
    osxstep.step = 0;
    
    MacOSXApplication app(osx_open_file, &osxstep, argc, argv);
#else
    QApplication app(argc, argv);
#endif

    app.setWindowIcon(QIcon(QString::fromUtf8(":/icon.ico")));

    ConsoleWindow cw(NULL);
    ConnectDialog connect(NULL);
#ifdef __APPLE__
    osxstep.userdata = &connect;
#endif
#ifdef PRINT_STATUS
    fprintf(stderr, "%s", COPYRIGHT_NOTICE);
#endif
    char buffer1[1024];
    char buffer2[1024];
    char buffer3[1024];
    char buffer4[1024];
    char buffer5[1024];
    char buffer6[1024];
    char buffer7[1024];

    buffer1[0] = 0;
    buffer2[0] = 0;
    buffer3[0] = 0;
    buffer4[0] = 0;
    buffer5[0] = 0;
    buffer6[0] = 0;
    buffer7[0] = 0;

    for (int i = 0; i < argc; i++) {
        switch (i) {
            case 0:
                strncpy(buffer1, argv[i], 1024);
                break;

            case 1:
#ifdef __APPLE__
                if (!strncmp(argv[i], "-psn_", 5)) {
                    argc--;
                    argv++;
                    i--;
                } else
#endif
                strncpy(buffer2, argv[i], 1024);
                break;

            case 2:
                strncpy(buffer3, argv[i], 1024);
                break;

            case 3:
                strncpy(buffer4, argv[i], 1024);
                break;

            case 4:
                strncpy(buffer5, argv[i], 1024);
                break;

            case 5:
                strncpy(buffer6, argv[i], 1024);
                break;

            case 6:
                strncpy(buffer7, argv[i], 1024);
                break;
        }
    }
#ifdef __APPLE__
    if (buffer2[0] == 0) {
        app.processEvents(QEventLoop::AllEvents, 100);
        int len = osxstep.data.Length();
        if ((osxstep.step == 2) && (len)) {
            argc = 2;
            if (len > 1023)
                len = 1023;
            strncpy(buffer2, osxstep.data.c_str(), len);
            buffer2[len] = 0;
        } else {
            osxstep.userdata = &connect;
            osxstep.step = 1;
        }
    }
#endif
    if (buffer2[0] == 0) {
        if (connect.exec() == QDialog::Accepted) {
            AnsiString linkpath = connect.getText();
            int        len      = linkpath.Length();
            if (len) {
                argc = 2;
                if (len > 1023)
                    len = 1023;
                strncpy(buffer2, linkpath.c_str(), len);
                buffer2[len] = 0;

                AnsiString record = linkpath;
                record += (char *)"\n";
                if (connect.history.Pos(record) <= 0) {
                    connect.history = record + connect.history;
                    connect.history.SaveFile(connect.h_path.c_str());
                }
            }
        } else
            return 0;
    }

    int port = DEFAULT_PORT;
    if (buffer2[0] == 0) {
        dialog_error("Invalid parameters received", "<b>Usage:</b><br/><br/>ConClient server_addres application_name<br/><br/><b>or:</b><br/><br/>ConClient shortcutfile.ss");
        return -1;
    } else
    if (buffer3[0] == 0) {
        LoadFromFile(AnsiString(buffer2), buffer2, buffer3, buffer4, port);
    } else
        ParseHost(AnsiString(buffer2), buffer2, port);

    AnsiString secured  = buffer4;
    int        is_debug = (int)(secured == (char *)"debug");
    int        is_http  = (int)(secured == (char *)"http");
    is_secured = (int)(secured == (char *)"secured");

    CConceptClient CC(MESSAGE_CALLBACK, is_secured, is_debug, WorkerNotify, WorkerMessage);
#ifdef __APPLE__
    osxstep.step = 2;
#endif
    if (!CC.Connect(buffer2, port)) {
        AnsiString temp("Unable to connect to <b>");
        temp += buffer2;
        if (port != DEFAULT_PORT) {
            temp += ":";
            temp += AnsiString((long)port);
        }
        temp += "</b>.";
        dialog_error("Error connecting to server", temp.c_str());
        return -2;
    }
    int pipestream = AnsiString(buffer5).ToInt();
    CC.Run(buffer3, pipestream);

    cw.show();

    CC.UserData = &cw;
    Worker worker(&CC);
    Worker worker2(&CC);
    Worker worker3(&CC);

    QThread thread;
    //QThread thread_send;

    worker.moveToThread(&thread);
    //worker3.moveToThread(&thread_send);

    //QObject::connect(&thread_send, SIGNAL(started()), &worker3, SLOT(doSend()));
    SetWorker(&worker);
    QObject::connect(&worker, SIGNAL(valueChanged(QQueue<TParameters *> *, QMutex *)), &worker2, SLOT(callback(QQueue<TParameters *> *, QMutex *)), Qt::QueuedConnection);
    QObject::connect(&worker, SIGNAL(transferNotify(int, int, bool)), &worker2, SLOT(doTransferNotify(int, int, bool)), Qt::QueuedConnection);
    QObject::connect(&worker, SIGNAL(Message(int)), &worker2, SLOT(doMessage(int)), Qt::QueuedConnection);

    QObject::connect(&worker, SIGNAL(workRequested()), &thread, SLOT(start()));
    QObject::connect(&thread, SIGNAL(started()), &worker, SLOT(doWork()));
    QObject::connect(&worker, SIGNAL(finished()), &thread, SLOT(quit()), Qt::DirectConnection);
    worker.requestWork();
    //thread_send.start();

#ifdef _WIN32
    AnsiString path = GetDirectory();
    LoadControls(&CC, path + COMPONENTS);
    AnsiString full_cookies_path = GetCookiesPath();
    chdir(full_cookies_path.c_str());
    NotifyUpdate(path + UPDATED_TXT);
#else
 #ifdef __APPLE__
    LoadControls(&CC, GetDirectory() + COMPONENTS);
 #else
    LoadControls(&CC, COMPONENTS_PATH);
 #endif
    chdir(getenv("HOME"));
    mkdir(".ConceptClient-cookies", 0777L);
    chdir(".ConceptClient-cookies");//COOKIES_PATH
#endif
    SetCookiesDir(GetCookiesPath());
    int res = app.exec();
    worker.abort();
    Done(&CC, res);
    return res;
}
