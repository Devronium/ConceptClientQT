#ifndef __COMPONENTS_LIBRARY_H
#define __COMPONENTS_LIBRARY_H

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
#define INVOKE_GETCONTROL        100

#define INVOKE_GETPOSTSTRING     101

#define INVOKE_GETPOSTOBJECT     102

#define INVOKE_GETEXTRAPTR       103

#define INVOKE_FIREEVENT         104

#define INVOKE_SENDMESSAGE       105

#define INVOKE_SETEXTRAPTR       106

#define INVOKE_GETCONTROLPTR     107

#define INVOKE_STATICQUERY       108

#define INVOKE_STATICRESPONSE    109

#define INVOKE_GETHANDLE         110

#define INVOKE_DESTROYHANDLE     111

#define INVOKE_GETOBJECT         112

#define INVOKE_WAITMESSAGE       113

#define INVOKE_GETTMPCOOKIES     114

#define INVOKE_FIREEVENT2        115

#define INVOKE_GETPRIVATEKEY     116

#define INVOKE_GETPUBLICKEY      117

// INVOKER function type
typedef int (*INVOKER)(void *context, int INVOKE_TYPE, ...);
#endif //__COMPONENTS_LIBRARY_H
