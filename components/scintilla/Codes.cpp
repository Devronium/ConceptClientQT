#include "Codes.h"

/*INTEGER Contains(char *charset, C_BYTE c) {
 *  if (charset->Pos(c) > -1)
 *     return 1;
 *  else
 *     return 0;
 * }*/

INTEGER GetOperatorType(INTEGER ID) {
    switch (ID) {
        // selectors
        case KEY_INDEX_OPEN:
        case KEY_INDEX_CLOSE:
        case KEY_P_OPEN:
        case KEY_SELECTOR_P:
        case KEY_SELECTOR_I:
        case KEY_P_CLOSE:
            return OPERATOR_SELECTOR;                         // () si []

        // operators
        //case KEY_DLL_CALL   :
        case KEY_NEW:
        case KEY_DELETE:
        case KEY_TYPE_OF:
        case KEY_CLASS_NAME:
        case KEY_LENGTH:
        case KEY_VALUE:
        case KEY_NOT:
        case KEY_COM:
        case KEY_INC:
        case KEY_DEC:
        case KEY_POZ:
        case KEY_NEG:
            return OPERATOR_UNARY;

        case KEY_INC_LEFT:
        case KEY_DEC_LEFT:
            return OPERATOR_UNARY_LEFT;

        //  facut binar de curand ...
        case KEY_DLL_CALL:
        case KEY_SEL:

        case KEY_MUL:
        case KEY_DIV:
        case KEY_REM:
        case KEY_SUM:
        case KEY_DIF:
        case KEY_SHL:
        case KEY_SHR:
        case KEY_LES:
        case KEY_LEQ:
        case KEY_GRE:
        case KEY_GEQ:
        case KEY_EQU:
        case KEY_NEQ:
        case KEY_AND:
        case KEY_XOR:
        case KEY_OR:
        case KEY_BAN:
        case KEY_BOR:
        //case KEY_CND        :
        case KEY_ASG:
        case KEY_BY_REF:
        case KEY_ASU:
        case KEY_ADI:
        case KEY_AMU:
        case KEY_ADV:
        case KEY_ARE:
        case KEY_AAN:
        case KEY_AXO:
        case KEY_AOR:
        case KEY_ASL:
        case KEY_ASR:
            return OPERATOR_BINARY;

        case KEY_CND_1:
        case KEY_CND_2:
            return OPERATOR_RESOLUTION;

        case KEY_COMMA:
            return KEY_COMMA;
    }
    return 0;
}

AnsiString GetKeyWord(INTEGER ID) {
    switch (ID) {
        case KEY_BEGIN:
            return BEGIN;

        case KEY_END:
            return END;

        case KEY_IF:
            return IF;

        case KEY_ELSE:
            return ELSE;

        case KEY_FOR:
            return FOR;

        case KEY_FOREACH:
            return FOREACH;

        case KEY_WHILE:
            return WHILE;

        case KEY_DO:
            return DO;

        case KEY_ECHO:
            return ECHO;

        case KEY_RETURN:
            return RETURN;

        case KEY_TRY:
            return TRY;

        case KEY_CATCH:
            return CATCH;

        case KEY_THROW:
            return THROW;

        case KEY_BREAK:
            return BREAK;

        case KEY_CONTINUE:
            return CONTINUE;

        case KEY_NEW:
            return NEW;

        case KEY_DELETE:
            return M_DELETE;

        case KEY_SEP:
            return SEP;

        case KEY_CLASS:
            return CLASS;

        case KEY_FUNCTION:
            return FUNCTION;

        case KEY_PRIVATE:
            return PRIVATE;

        case KEY_PUBLIC:
            return PUBLIC;

        case KEY_CEVENT:
            return CEVENT;

        case KEY_PROPERTY:
            return PROPERTY;

        case KEY_SET:
            return SET;

        case KEY_GET:
            return GET;

        case KEY_EXTENDS:
            return EXTENDS;

        case KEY_INCLUDE:
            return INCLUDE;

        case KEY_IMPORT:
            return IMPORT;

        case KEY_IN:
            return C_IN;

        case KEY_TRIGERS:
            return TRIGERS;

        case KEY_VAR:
            return VAR;

        case KEY_QUOTE1:
            return QUOTE1;

        case KEY_QUOTE2:
            return QUOTE2;

        case KEY_OPERATOR:
            return M_OPERATOR;

        case KEY_P_OPEN:
            return P_OPEN;

        case KEY_P_CLOSE:
            return P_CLOSE;

        case KEY_INDEX_OPEN:
            return INDEX_OPEN;

        case KEY_INDEX_CLOSE:
            return INDEX_CLOSE;

        case KEY_COMMA:
            return CONCEPT_COMMA;

        case KEY_DEFINE:
            return C_DEFINE;

        case KEY_LENGTH:
            return C_LENGTH;

        case KEY_VALUE:
            return C_VALUE;

        case KEY_OVERRIDE:
            return OVERRIDE;

        case KEY_PROTECTED:
            return PROTECTED;

        case KEY_PRAGMA:
            return C_PRAGMA;

        case KEY_STATIC:
            return C_STATIC;

        case KEY_SUPER:
            return C_SUPER;

        /*case KEY_BYTE       : return C_BYTE;
         * case KEY_SHORT      : return C_SHORT;
         * case KEY_INTEGER    : return C_INTEGER;
         * case KEY_STRING     : return C_STRING;
         * case KEY_POINTER    : return C_POINTER;
         * case KEY_FLOAT      : return C_FLOAT;*/

        // operators
        case KEY_SELECTOR_P:
            return SELECTOR_P;

        case KEY_SELECTOR_I:
            return SELECTOR_I;

        case KEY_SEL:
            return SEL;

        case KEY_NOT:
            return NOT;

        case KEY_COM:
            return COM;

        case KEY_INC_LEFT:
        case KEY_INC:
            return INC;

        case KEY_DEC_LEFT:
        case KEY_DEC:
            return DEC;

        case KEY_POZ:
            return POZ;

        case KEY_NEG:
            return NEG;

        case KEY_MUL:
            return MUL;

        case KEY_DIV:
            return DIV;

        case KEY_REM:
            return REM;

        case KEY_SUM:
            return SUM;

        case KEY_DIF:
            return DIF;

        case KEY_SHL:
            return SHL;

        case KEY_SHR:
            return SHR;

        case KEY_LES:
            return LES;

        case KEY_LEQ:
            return LEQ;

        case KEY_GRE:
            return GRE;

        case KEY_GEQ:
            return GEQ;

        case KEY_EQU:
            return EQU;

        case KEY_NEQ:
            return NEQ;

        case KEY_AND:
            return AND;

        case KEY_XOR:
            return XOR;

        case KEY_OR:
            return OR;

        case KEY_BAN:
            return BAN;

        case KEY_BOR:
            return BOR;

        case KEY_CND_1:
            return CND_1;

        case KEY_CND_2:
            return CND_2;

        case KEY_ASG:
            return ASG;

        case KEY_BY_REF:
            return ASG_BY_REF;

        case KEY_ASU:
            return ASU;

        case KEY_ADI:
            return ADI;

        case KEY_AMU:
            return AMU;

        case KEY_ADV:
            return ADV;

        case KEY_ARE:
            return ARE;

        case KEY_AAN:
            return AAN;

        case KEY_AXO:
            return AXO;

        case KEY_AOR:
            return AOR;

        case KEY_ASL:
            return ASL;

        case KEY_ASR:
            return ASR;

        case KEY_CLASS_NAME:
            return CLASS_NAME;

        case KEY_TYPE_OF:
            return TYPE_OF;

        case KEY_DLL_CALL:
            return DLL_CALL;

        case KEY_OPTIMIZED_IF:
            return "OPTIMIZED_IF";

        case KEY_OPTIMIZED_GOTO:
            return "OPTIMIZED_GOTO";

        case KEY_OPTIMIZED_ECHO:
            return "OPTIMIZED_ECHO";

        case KEY_OPTIMIZED_RETURN:
            return "OPTIMIZED_RETURN";

        case KEY_OPTIMIZED_THROW:
            return "OPTIMIZED_THROW";

        case KEY_OPTIMIZED_TRY_CATCH:
            return "OPTIMIZED_TRY_CATCH";
    }
    return NULL_STRING;
}

INTEGER GetID(AnsiString& KeyWord) {
    if (KeyWord == (char *)P_OPEN)
        return KEY_P_OPEN;
    if (KeyWord == (char *)P_CLOSE)
        return KEY_P_CLOSE;
    if (KeyWord == (char *)INDEX_OPEN)
        return KEY_INDEX_OPEN;
    if (KeyWord == (char *)INDEX_CLOSE)
        return KEY_INDEX_CLOSE;

    if (KeyWord == (char *)CONCEPT_COMMA)
        return KEY_COMMA;
    if (KeyWord == (char *)C_LENGTH)
        return KEY_LENGTH;
    if (KeyWord == (char *)C_VALUE)
        return KEY_VALUE;


    if (KeyWord == (char *)SELECTOR_I)
        return KEY_SELECTOR_I;

    // operators
    if (KeyWord == (char *)SEL)
        return KEY_SEL;
    if (KeyWord == (char *)SEL_2)
        return KEY_SEL;
    if (KeyWord == (char *)NOT)
        return KEY_NOT;
    if (KeyWord == (char *)COM)
        return KEY_COM;
    if (KeyWord == (char *)INC)
        return KEY_INC;
    if (KeyWord == (char *)DEC)
        return KEY_DEC;


    /*if (KeyWord==(char *)POZ)
     *  return KEY_POZ;
     * if (KeyWord==(char *)NEG)
     *  return KEY_NEG;*/
    if (KeyWord == (char *)MUL)
        return KEY_MUL;
    if (KeyWord == (char *)DIV)
        return KEY_DIV;
    if (KeyWord == (char *)REM)
        return KEY_REM;
    if (KeyWord == (char *)SUM)
        return KEY_SUM;
    if (KeyWord == (char *)DIF)
        return KEY_DIF;
    if (KeyWord == (char *)SHL)
        return KEY_SHL;
    if (KeyWord == (char *)SHR)
        return KEY_SHR;
    if (KeyWord == (char *)LES)
        return KEY_LES;
    if (KeyWord == (char *)LEQ)
        return KEY_LEQ;
    if (KeyWord == (char *)GRE)
        return KEY_GRE;
    if (KeyWord == (char *)GEQ)
        return KEY_GEQ;
    if (KeyWord == (char *)EQU)
        return KEY_EQU;
    if (KeyWord == (char *)NEQ)
        return KEY_NEQ;
    if (KeyWord == (char *)AND)
        return KEY_AND;
    if (KeyWord == (char *)XOR)
        return KEY_XOR;
    if (KeyWord == (char *)OR)
        return KEY_OR;
    if (KeyWord == (char *)BAN)
        return KEY_BAN;
    if (KeyWord == (char *)BOR)
        return KEY_BOR;
    // resolution !
    if (KeyWord == (char *)CND_1)
        return KEY_CND_1;
    if (KeyWord == (char *)CND_2)
        return KEY_CND_2;
    // pana aici
    if (KeyWord == (char *)ASG)
        return KEY_ASG;
    if (KeyWord == (char *)ASG_BY_REF)
        return KEY_BY_REF;
    if (KeyWord == (char *)ASU)
        return KEY_ASU;
    if (KeyWord == (char *)ADI)
        return KEY_ADI;
    if (KeyWord == (char *)AMU)
        return KEY_AMU;
    if (KeyWord == (char *)ADV)
        return KEY_ADV;
    if (KeyWord == (char *)ARE)
        return KEY_ARE;
    if (KeyWord == (char *)AAN)
        return KEY_AAN;
    if (KeyWord == (char *)AXO)
        return KEY_AXO;
    if (KeyWord == (char *)AOR)
        return KEY_AOR;
    if (KeyWord == (char *)ASL)
        return KEY_ASL;
    if (KeyWord == (char *)ASR)
        return KEY_ASR;
    if (KeyWord == (char *)CLASS_NAME)
        return KEY_CLASS_NAME;
    if (KeyWord == (char *)TYPE_OF)
        return KEY_TYPE_OF;
    if (KeyWord == (char *)DLL_CALL)
        return KEY_DLL_CALL;
    return 0;
}

INTEGER GetType(AnsiString KeyWord) {
    INTEGER length = KeyWord.Length();

    if (length == 0)
        return TYPE_EMPTY;

    if (KeyWord == (char *)SEP)
        return TYPE_SEPARATOR;

    if ((KeyWord == (char *)BEGIN) || (KeyWord == (char *)END))
        return TYPE_KEYWORD;

    if (LexicalCheck(KeyWord, TYPE_OPERATOR))
        return TYPE_OPERATOR;

    if (LexicalCheck(KeyWord, TYPE_METHOD))
        return TYPE_METHOD;

    if (length > 1) {
        char *kptr = KeyWord.c_str();
        if ((kptr[0] == kptr[length - 1]) && ((kptr[0] == QUOTE1[0]) || (kptr[0] == QUOTE2[0])))
            return TYPE_STRING;
        if ((kptr[0] == P_OPEN[0]) && (kptr[length - 1] == P_CLOSE[0]))
            return TYPE_ATOM;

        if ((kptr[0] == INDEX_OPEN[0]) && (kptr[length - 1] == INDEX_CLOSE[0]))
            return TYPE_INDEXER;
    }

    // daca nu-i nimic, banuiesc ca ramane doar sa fie numar ...
    if (LexicalCheck(KeyWord, TYPE_NUMBER))
        return TYPE_NUMBER;
    else
    if (LexicalCheck(KeyWord, TYPE_HEX_NUMBER))
        return TYPE_HEX_NUMBER;
    else
        return TYPE_INVALID;
}

INTEGER IsOperator(AnsiString S) {
    INTEGER id = GetID(S);

    return ((id > __START_OPERATORS) && (id < __END_OPERATORS)) ? 1 : 0;
}

AnsiString Strip(AnsiString String) {
    char *data;

    switch (String[0]) {
        case '\'':
        case '"':
        case '(':
            data = String.c_str();
            data[String.Length() - 1] = 0;
            return AnsiString(data + 1);

        default:
            return String;
    }
}

INTEGER ValidName(AnsiString& S) {
    INTEGER len = S.Length();

    //INTEGER numeric_switch=0;
    if (!len)
        return 0;

    for (INTEGER i = 0; i < len; i++) {
        /*if ((S[i]>='0') && (S[i]<='9') && (i))
         *  numeric_switch=1;
         * else
         * if ((((S[i]>='a') && (S[i]<='z')) ||
         *  ((S[i]>='A') && (S[i]<='Z')) || (S[i]=='_')) && (!numeric_switch))
         *  continue;
         * else
         *  return 0;*/
        if ((((S[i] >= 'a') && (S[i] <= 'z')) ||
             ((S[i] >= 'A') && (S[i] <= 'Z')) || (S[i] == '_')) ||
            ((S[i] >= '0') && (S[i] <= '9') && (i)))
            continue;
        else
            return 0;
    }
    return 1;
}

INTEGER ValidHexNumber(AnsiString& S) {
    INTEGER len = S.Length();

    if (len < 3)
        return 0;
    for (INTEGER i = 2; i < len; i++) {
        if (((S[i] >= '0') && (S[i] <= '9')) ||
            ((S[i] >= 'a') && (S[i] <= 'f')) ||
            ((S[i] >= 'A') && (S[i] <= 'F')))
            continue;
        else
            return 0;
    }
    return 1;
}

INTEGER ValidNumber(AnsiString& S) {
    INTEGER len        = S.Length();
    INTEGER done_point = 0;
    INTEGER EXP        = 0;
    INTEGER EXP_PREC   = 0;

    if (!len)
        return 0;

    for (INTEGER i = 0; i < len; i++) {
        if (EXP_PREC) {
            EXP_PREC = 0;
            if ((S[i] != '+') && (S[i] != '-'))
                return 0;
        } else
        if (((S[i] == 'e') || (S[i] == 'E')) && (!EXP)) {
            EXP      = 1;
            EXP_PREC = 1;
        } else
        if (S[i] == '.')
            done_point = 1;
        else
        if ((S[i] >= '0') && (S[i] <= '9'))
            continue;
        else
            return 0;
    }
    return !EXP_PREC;
}

INTEGER LexicalCheck(AnsiString& S, INTEGER TYPE) {
    if (TYPE == TYPE_METHOD)
        return ValidName(S);
    else
    if (TYPE == TYPE_NUMBER)
        return ValidNumber(S);
    else
    if (TYPE == TYPE_HEX_NUMBER)
        return ValidHexNumber(S);
    else
    if (TYPE == TYPE_OPERATOR)
        return GetOperatorType(GetID(S));
    return 0;
}

AnsiString StripString(AnsiString *S) {
    long       len     = S->Length();
    int        start   = 0;
    int        escape  = 0;
    int        set_esc = 0;
    AnsiString result  = "";

    char first = 0;
    char last  = 0;

    if (len >= 2) {
        first = (*S)[(unsigned long)0];
        last  = (*S)[(unsigned long)len - 1];
    }

    if ((first == last) && ((first == '\"') || (first == '\''))) {
        len--;
        start++;
    }

    for (unsigned long i = start; i < (unsigned long)len; i++) {
        if (((*S)[i] == '\\') && (!escape))
            set_esc = 1;
        else
            set_esc = 0;

        if (!set_esc) {
            if (!escape)
                result += (*S)[i];
            else {
                if ((*S)[i] == 'n')
                    result += '\n';
                else
                if ((*S)[i] == 'r')
                    result += '\r';
                else
                if ((*S)[i] == 't')
                    result += '\t';
                else
                if ((*S)[i] == 'b')
                    result += '\b';
                else
                if ((*S)[i] == '0')
                    result += '\0';
                else
                    result += (*S)[i];
            }
        }
        escape  = set_esc;
        set_esc = 0;
    }
    return result;
}

INTEGER GetPriority(INTEGER OP_ID) {
    if (OP_ID <= OPERATOR_LEVEL_SET_0)
        return /*OP_ID;//*/ OPERATOR_LEVEL_SET_0;
    if (OP_ID <= OPERATOR_LEVEL_1)
        return OPERATOR_LEVEL_1;
    if (OP_ID <= OPERATOR_LEVEL_2)
        return OPERATOR_LEVEL_2;
    if (OP_ID <= OPERATOR_LEVEL_3)
        return OPERATOR_LEVEL_3;
    if (OP_ID <= OPERATOR_LEVEL_4)
        return OPERATOR_LEVEL_4;
    if (OP_ID <= OPERATOR_LEVEL_5)
        return OPERATOR_LEVEL_5;
    if (OP_ID <= OPERATOR_LEVEL_6)
        return OPERATOR_LEVEL_6;
    if (OP_ID <= OPERATOR_LEVEL_SET_7)
        return OP_ID;
    if (OP_ID <= OPERATOR_LEVEL_8)
        return OPERATOR_LEVEL_8;
    if (OP_ID <= OPERATOR_LEVEL_9)
        return OPERATOR_LEVEL_9;
}

/*#include <fstream>
 * void __DEBUG_PRINT(char *str) {
 *      std::ofstream out;
 *      out.open("debug-output.txt", std::ios_base::out | std::ios_base::app);
 *      out << str << "\n";
 *      out.close();
 * }
 */
