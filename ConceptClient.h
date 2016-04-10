//#pragma once
#ifndef __CONCEPT_CLIENT_H
#define __CONCEPT_CLIENT_H
// uncomment to print to stdin (to enable console)
//#define PRINT_STATUS
#ifdef __WITH_TOMCRYPT
 #include "tomcrypt/tomcrypt.h"
#endif
#ifdef _WIN32
 #ifdef _WIN32_WINNT
  #undef _WIN32_WINNT
 #endif
 #define _WIN32_WINNT    0x0501
 #include <windows.h>
 #include <winsock2.h>
 #include <ws2tcpip.h>
 #define THREAD_TYPE2      DWORD WINAPI
 #define HANDLE2           HANDLE
#else
 #define SOCKET            int
 #define INVALID_SOCKET    -1
 #define SOCKET_ERROR      -1
 #define DWORD             long
 #define LPVOID            void *
 #define WINAPI
 #define THREAD_TYPE2      LPVOID
 #define closesocket       close

 #include <sys/types.h>
 #include <sys/socket.h>
 #include <sys/wait.h>
 #include <netinet/in.h>
 #include <netdb.h>
 #include <dlfcn.h>      // dlopen
 #include <unistd.h>
 #include <pthread.h>

 #include <sys/select.h>
 #include <netinet/tcp.h>
 #include <arpa/inet.h>

 #define HANDLE2    void *

 #define Sleep(x)    usleep(x * 1000)
#endif

#ifndef NOSSL
extern "C" {
 #include <openssl/bio.h>
 #include <openssl/ssl.h>
 #include <openssl/err.h>
 #include <openssl/x509.h>
}
#endif

#include "AnsiString.h"
#include "AnsiList.h"
#include "GTKControl.h"
#include "CompensationParser.h"
#include "semhh.h"

#include <map>

typedef void (*PROGRESS_API)(int how_much, int message, bool idle_call);
typedef void (*NOTIFY_API)(int message);
typedef int (*VERIFY_API)(int code, void *ssl, int flags);

#define SOCK_OPERATION_TIMEOUT    3

#define THREAD_TYPE               gpointer

#define WM_USER_CREATE_CONTROL    WM_USER + 0x100

#define DEFAULT_PORT              2662
#define DEFAULT_HTTP_PORT         80

#define OUTPUT_DUMP               "core-output-dump.txt"

#define     T_NUMBER              0x02
#define     T_STRING              0x03

#define INTEGER                   int

//#define PUBLIC_KEY_FILE       "client.certificate"
//#define PRIVATE_KEY_FILE	"client.key"

#define AES_KEY_FILE          "aes.key"
#define PUBLIC_KEY_FILE       AES_KEY_FILE
#define PRIVATE_KEY_FILE      AES_KEY_FILE
#define RANDOM_KEY_SIZE       16

#define CONCEPT_DLL_LINK      INTEGER
#define CONCEPT_DLL_RESULT    void *
#define SYSTEM_SOCKET         SOCKET

extern "C" {
#ifdef _WIN32
 #define CONCLIENT_DLL_API    __declspec(dllexport) int _cdecl
#else
 #define CONCLIENT_DLL_API    int
#endif

CONCLIENT_DLL_API ConceptClientRun(void *handle, const char *host, const char *app, int secured, void *reserved);
}

class CConceptClient;

typedef struct Parameters {
    AnsiString     Sender;
    int            ID;
    AnsiString     Target;
    AnsiString     Value;
    CConceptClient *Owner;
    char           Flags;
    void           *UserData;
} TParameters;

typedef int (*CALLBACK_FUNC)(Parameters *PARAM, Parameters *OUT_PARAM);

class CConceptClient {
    friend class Worker;
private:
    AnsiString REMOTE_PUBLIC_KEY;
    AnsiString SessionID;
    int        secured;
    int        debug;

    int waitconfirmation;

    int NON_CRITICAL_SECTION;

    CALLBACK_FUNC CONCEPT_CALLBACK;
    HHSEM         sem;
    AnsiList      MessageStack;

    PROGRESS_API notify;
    NOTIFY_API   reconnectInfo;
    bool         connection_err;

#ifndef NOSSL
    bool    in_ssl_neg;
    SSL_CTX *sslctx = NULL;
    SSL     *ssl    = NULL;
#endif

    struct sockaddr last_addr;
    socklen_t       last_length;

    static int sock_eof2(int stream, int sec);
    static int connect2(int socket, struct sockaddr *addr, socklen_t length);

public:
    static AnsiString ConceptClientPath;
    static AnsiString path;

    AnsiString         lastHOST;
    AnsiString         lastApp;
    AnsiList           CookieList;
    CompensationParser *parser;
    Parameters         OUT_PARAM;

    HHSEM shell_lock;
    HHSEM execute_lock;
#ifdef _WIN32
    HANDLE2 executed_process = 0;
#else
    pid_t executed_process = 0;
#endif

    AnsiList ShellList;
    std::map<int, GTKControl *>    Controls;
    std::map<void *, GTKControl *> ControlsReversed;
#ifndef NO_WEBKIT
    std::map<std::string, std::string> snapclasses_header;
    std::map<std::string, std::string> snapclasses_body;
    std::map<std::string, std::string> snapclasses_full;
#endif
    AnsiString POST_STRING;
    AnsiString POST_TARGET;
    int        POST_OBJECT;

    AnsiString LOCAL_PUBLIC_KEY;
    AnsiString LOCAL_PRIVATE_KEY;

    static int timedout_recv(SOCKET _socket, char *buffer, int size, int flags, int timeout = 5);

    void       *UserData;
    void       *MainForm;
    void       *LastObject;
    int        PORT;
    int        is_http;
    unsigned int msg_count;
    AnsiString HTTP_prefix;
    AnsiString HTTP_SendLink;
    AnsiString HTTP_RecvLink;
    AnsiString HTTP_PendingLink;

    AnsiString Called_HOST;

    void        SetCritical(int status);

    SOCKET CLIENT_SOCKET;
    SOCKET RTSOCKET;

    int         Connect(char *HOST, int PORT = DEFAULT_PORT, long extra = 0);
    int         CheckReconnect();

    int         ConnectHTML(char *HOST, int PORT = DEFAULT_HTTP_PORT);
    int         Disconnect();
    int         Run(char *app_name, int pipe = -1);

    long        SendMessage(const char *sender, int MSG, const char *target, AnsiString value, char force = 0);
    long        SendBigMessage(const char *sender, int MSG, const char *target, AnsiString& value, char force = 0);
    long        SendFileMessage(const char *sender, int MSG, const char *target, const char *filename, char force = 0);
    void        SendMessageNoWait(const char *sender, int MSG, const char *target, AnsiString value, char force = 0);
    void        SendPending();
    AnsiString  Encrypt(char *buf, int size, AnsiString *key);

    void        GenerateRandomAESKey(AnsiString *, int);

    int         NoErrors();
    int         Dump(char *);

    int         InitTLS(VERIFY_API verify, int flags = 2);
    void        ResetTLS();
    int         Send(char *buf, int len);
    int         Recv(char *buf, int len);
    int         RecvTimeout(char *buffer, int size, int timeout = 5);
    int         IsEOF();

    CConceptClient(CALLBACK_FUNC cb, int secured, int debug, PROGRESS_API _notify = 0, NOTIFY_API _reconnectInfo = 0);
    virtual ~CConceptClient();
};
#endif // __CONCEPT_CLIENT_H
