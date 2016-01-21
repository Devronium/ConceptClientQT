#ifndef __STDLIBRARY_H
#define __STDLIBRARY_H



#ifdef _WIN32
 #define PREFIX    __declspec(dllexport) _cdecl
#else
 #define PREFIX
#endif

#define P_CAPTION               100
#define P_RATIO                 1701
#define P_POSITION              803


#define MSG_CUSTOM_MESSAGE1     0x120
#define MSG_CUSTOM_MESSAGE2     0x121
#define MSG_CUSTOM_MESSAGE3     0x122
#define MSG_CUSTOM_MESSAGE4     0x123
#define MSG_CUSTOM_MESSAGE5     0x124
#define MSG_CUSTOM_MESSAGE6     0x125
#define MSG_CUSTOM_MESSAGE7     0x126
#define MSG_CUSTOM_MESSAGE8     0x127
#define MSG_CUSTOM_MESSAGE9     0x128
#define MSG_CUSTOM_MESSAGE10    0x129
#define MSG_CUSTOM_MESSAGE11    0x12A
#define MSG_CUSTOM_MESSAGE12    0x12B
#define MSG_CUSTOM_MESSAGE13    0x12C
#define MSG_CUSTOM_MESSAGE14    0x12D
#define MSG_CUSTOM_MESSAGE15    0x12E
#define MSG_CUSTOM_MESSAGE16    0x12F
#define MSG_REPAINT             0x130


// default INVOKER result
#define E_NOERROR           0

// returned if INVOKER accepts a context and processes it
#define E_ACCEPTED          1

// returned by INVOKE_CREATE if a control must not be added to its parent
#define E_ACCEPTED_NOADD    2

// returned if an invoke type is not implemented
#define E_NOTIMPLEMENTED    -1

// macro for result check
#define IS_OK(RESULT_CODE)    RESULT_CODE < 0 ? 0 : 1

// tests the presence of a class in a dll
// syntax : INVOKE_QUERYTEST(int class_id)
#define INVOKE_QUERYTEST             0

// activates an event
#define INVOKE_SETEVENT              1

// sets a property
#define INVOKE_SETPROPERTY           2

// gets a property
#define INVOKE_GETPROPERTY           3

// sets the main stream
#define INVOKE_SETSTREAM             4

// sets the secondary stream
#define INVOKE_SETSECONDARYSTREAM    5

// processes a custom control message
#define INVOKE_CUSTOMMESSAGE         6

// create a control
#define INVOKE_CREATE                7

// destory a control
#define INVOKE_DESTROY               8

// called when initializing the library
#define INVOKE_INITLIBRARY           9

// called when destroing the library
#define INVOKE_DONELIBRARY           10


// control to client messages
#define INVOKE_GETCONTROL       100

#define INVOKE_GETPOSTSTRING    101

#define INVOKE_GETPOSTOBJECT    102

#define INVOKE_GETEXTRAPTR      103

#define INVOKE_FIREEVENT        104

#define INVOKE_SENDMESSAGE      105

#define INVOKE_SETEXTRAPTR      106

#define INVOKE_GETCONTROLPTR    107

#define INVOKE_FIREEVENT2       115

#define INVOKE_GETPRIVATEKEY    116

#define INVOKE_GETPUBLICKEY     117

// INVOKER function type
typedef int (*INVOKER)(void *context, int INVOKE_TYPE, ...);

extern "C" {
PREFIX int Invoke(void *context, int INVOKE_TYPE, ...);
}
#endif //__STDLIBRARY_H
