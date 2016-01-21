#include "stdlibrary.h"

#ifdef _WIN32
 #include <windows.h>
// DLL Entry Point ... nothing ...
int WINAPI DllEntryPoint(HINSTANCE hinst, unsigned long reason, void *lpReserved) {
    return 1;
}
#endif
//-----------------------------------------------------------------------------------
