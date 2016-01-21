#ifndef __QUICKPARSER_H
#define __QUICKPARSER_H

#include "AnsiList.h"
#include "AnsiString.h"
#include "Codes.h"


typedef void (*INCLUDE_REQUEST)(void *owner, char *includename);

int PreParse(void *owner, AnsiString *text, INCLUDE_REQUEST cback = 0);
int Parse(void *owner, AnsiString *text, INCLUDE_REQUEST cback = 0, char in_include = 0, char clean = 0);
AnsiString GetSciList(AnsiString& ClassName);
AnsiString GetHint(AnsiString& ClassName, AnsiString& Member);
AnsiString GetElementType(AnsiString& element, int line);
AnsiString GetMemberType(AnsiString& ClassName, AnsiString& element);
char ProjectListChanged();
AnsiString GetProjectList();
AnsiString GetCheckedProjectList();
#endif // __QUICKPARSER_H
