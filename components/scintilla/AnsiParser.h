#ifndef __ANSIPARSER_H
#define __ANSIPARSER_H

#include "AnsiString.h"
#include "Codes.h"
#include "AnsiList.h"
#include "ConceptTypes.h"

#define MAX_STR_VAR    1024

class AnsiParser
{
private:
    AnsiString *Target;
    AnsiList   *ConstantList;
    long       position;
    long       linia_curenta;

/*long var_pos[MAX_STR_VAR];
 * int var_cnt;*/

    AnsiString virtual_result;
    char       has_virtual_result;

    char put_prefix;
    char is_end;
    char put_a_quote;
    char in_formatted;
    char in_expr;
public:

/*int StrVarCount();
 * long *StrVar();*/

    AnsiParser(AnsiString *TRG, AnsiList *Constants);
    ~AnsiParser(void);

    void Atomize();
    long LastLine();

    void NextAtom(AnsiString& result, int no_constants = 0);
    AnsiString GetConstant();

/*AnsiString GetVariableName(int start, AnsiString sval, int *end);
 * AnsiString GetSubStr(AnsiString sval, int start, int end);*/
};
#endif
