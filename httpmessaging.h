#ifndef __HTTPMESSAGING_H
#define __HTTPMESSAGING_H

#ifdef _WIN32
 #include <windows.h>
 #include <winsock.h>
#endif
#include "messages.h"

void HTTPInit();
int HTTPPostSend(char *host, char *data, int len, int max_packet = 0xFFFFF, PROGRESS_API notify_parent = 0, bool idle_call = false);
int HTTPGetRecv(char *host, char **data, int *len, PROGRESS_API notify_parent = 0, bool idle_call = false);
int HTTPHaveMessage(char *host);
int HTTPRecv(char *host, char *data, int len, int flags, PROGRESS_API notify_parent = 0, bool idle_call = false);
void HTTPFreeData(char *buffer);
#endif // __HTTPMESSAGING_H
