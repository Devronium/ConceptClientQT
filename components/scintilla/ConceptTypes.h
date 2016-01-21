#ifndef __CONCEPTTYPES_H
#define __CONCEPTTYPES_H

#include "AnsiString.h"
#include "AnsiList.h"
#include <map>

typedef struct {
    AnsiString Name;
    int        Type;
} Variable;

class ClassMember {
public:
    AnsiString Name;
    AnsiString Hint;
    int        def_start;
    int        def_end;
    int        index;
    //AnsiList   Variables;
    std::map<unsigned int, Variable *> Variables;
    int        Type;
    char       fun_type;
    char       in_include;
    char       access;
    AnsiString linked_member;

    ~ClassMember() {
        for (std::map<unsigned int, Variable *>::iterator it2 = Variables.begin(); it2 != Variables.end(); ++it2) {
            Variable *var = it2->second;
            if (var)
                delete var;
        }
    }
};

class ClassContainer {
public:
    AnsiString Name;
    int        def_start;
    int        def_end;
    std::map<unsigned int, ClassMember *> Members;
    char in_include;

    ~ClassContainer() {
        for (std::map<unsigned int, ClassMember *>::iterator it2 = Members.begin(); it2 != Members.end(); ++it2) {
            ClassMember *CM = it2->second;
            // the list isn't an "autoclean" list
            if (CM)
                delete CM;
        }
    }
};

typedef struct {
    AnsiString Name;
    AnsiString Content;
    int        ses_requested;
    int        ses_requested_parse;
    int        last_mod;
    //AnsiString last_mod;
} Included;

#define V_DELEGATE    -4
#define V_ARRAY       -3
#define V_STRING      -2
#define V_NUMBER      -1
#endif //__CONCEPTTYPES_H
