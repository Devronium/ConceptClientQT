#include "AnsiParser.h"
#include <string.h>
//using namespace Concept;

#define IS_QUOTE(c)    (c == '"') || (c == '\'')

AnsiParser::AnsiParser(AnsiString *TRG, AnsiList *Constants) {
    position      = 0;
    Target        = TRG;
    linia_curenta = 1;
    ConstantList  = Constants;
    //var_cnt=0;
    has_virtual_result = 0;
    put_a_quote        = 0;
    in_formatted       = 0;
    in_expr            = 0;
    is_end             = 0;
    put_prefix         = 0;
}

AnsiParser::~AnsiParser(void) {
}

void AnsiParser::Atomize() {
    position      = 0;
    linia_curenta = 1;
}

AnsiString AnsiParser::GetConstant() {
    AnsiString result = NULL_STRING;
    long       len    = Target->Length();
    int        i;
    int        empty = 1;

    for (i = position; i < len; i++) {
        char c = (*Target)[i];

        if (c == '\r')
            continue;
        if (c == '\n')
            break;

        /*if ((c=='\n') || (c=='\r'))
         *  break;*/
        else
        if (empty) {
            if ((c != ' ') && (c != '\t') && (c != '\b') && (c != '\r')) {
                result += c;
                empty   = 0;
            }
        } else
            result += c;
    }
    position = i;
    return result;
}

void AnsiParser::NextAtom(AnsiString& result, int no_constants) {
    unsigned long len = Target->Length();

    /*AnsiString*/ result = NULL_STRING;
    int           separator    = 0;
    int           escape       = 0;
    int           oper         = 0;
    char          quote        = 0;
    char          next_close   = 0;
    int           line_comment = 0;
    int           comment      = 0;
    int           only_numbers = 1;
    unsigned long i;
    //var_cnt=0;
    char in_var = 0;
    char c      = 0;

    if (!Target)
        return;//return(result);

    if (has_virtual_result) {
        if (put_prefix) {
            put_prefix = 0;
            if (is_end) {
                result = (char *)")";
                //return(")");
                return;
            } else {
                result         = virtual_result;
                virtual_result = "(";
                return;//return(result);
            }
        }
        has_virtual_result = 0;
        result             = virtual_result;
        return;
        //return(virtual_result);
    }

    if (put_a_quote) {
        quote       = '"';
        result      = '"';
        put_a_quote = 0;
    }

    char *str_ptr = Target->c_str();
    for (i = position; i < len; i++) {
        separator = 0;
        //char c=(*Target)[i];
        c = str_ptr[i];
        if ((!escape) && (c == '@'))
            continue;

        if (c == NEW_LINE) {
            linia_curenta++;
            line_comment = 0;
        }
        if (((c == '\n') || (c == '\r')) && (quote && (!comment))) {
            i++;
            break;
        }

        if ((!quote) && (!comment) && (!line_comment)) {
            if ((c == '$') && (in_formatted)) {
                virtual_result     = "+";
                has_virtual_result = 1;
                is_end             = 1;
                put_a_quote        = 1;
                in_formatted       = 0;
                in_var             = 1;
                is_end             = 1;
                put_prefix         = 1;
                continue;
            } else
            if ((c == '{') && (in_var)) {
                in_expr            = 1;
                in_var             = 1;
                has_virtual_result = 0;
                put_a_quote        = 0;
                in_formatted       = 0;
                in_var             = 0;
                is_end             = 0;
                put_prefix         = 1;
                continue;
            } else
            if ((c == '}') && (in_expr)) {
                in_expr = 0;
                i++;
                virtual_result     = "+";
                has_virtual_result = 1;
                put_a_quote        = 1;
                in_formatted       = 0;
                in_var             = 1;
                is_end             = 1;
                if (!result.Length()) {
                    position = i;
                    //return(NextAtom());
                    NextAtom(result);
                    return;
                }
                break;
            }

            if (c == '.') {
                if (!only_numbers)
                    break;
                else
                if (i == position) {
                    result = c;
                    i++;
                    break;
                }
            }

            if ((only_numbers) && ((c < '0') || (c > '9')))
                only_numbers = 0;
        }


        AnsiString prediction = result;
        prediction += c;

        if (!quote) {
            if (prediction == LINE_COMMENT) {
                if (!comment) {
                    line_comment = 1;
                    result       = (char *)"";
                    oper         = 0;
                    separator    = 0;
                }
            } else
            if (prediction == (char *)COMMENT_START) {
                comment   = 1;
                result    = (char *)"";
                oper      = 0;
                separator = 0;
            } else
            if (prediction == (char *)COMMENT_END) {
                comment   = 0;
                result    = (char *)"";
                oper      = 0;
                separator = 0;
            } else
            if (comment) {
                if (c == COMMENT_END[0])
                    result = c;
                else
                    result = (char *)"";
            }
        }

        if ((!line_comment) && (!comment) && (prediction != (char *)COMMENT_END)) {
            if ((!quote) && ((c == ST_BEGIN) || (c == ST_END))) {
                if (result == (char *)"") {
                    result = c;
                    i++;
                }
                break;
            }

            if ((!quote) && (Contains2(OPERATORS, c))) {
                if ((oper) || ((result == (char *)"") && (!oper)))
                    //result+=c;
                    oper = 1;
                else
                    break;
            } else if (oper)
                break;

            if ((!quote) && ((c == STATAMENT_SEPARATOR))) {
                if (result == (char *)"") {
                    result = c;
                    i++;
                }
                break;
            }
            // verific daca am un escape character in ghilimele
            if ((c == ESCAPE_CHARACTER) && (quote) && (!escape)) {
                escape  = 1;
                result += c;
            } else {
                // conditia cu !oper e pusa de curand ...
                if ((!oper) && (!escape) && (/*Contains2(QUOTES,c)*/ IS_QUOTE(c))) {
                    if ((!quote) && (result.Length()))
                        break;
                    result += c;                   // de curand
                    if (!quote)
                        quote = c;
                    else if (quote == c) {
                        quote = 0;
                        //result+=c;
                        i++;
                        break;
                    }
                } else                 // adaugat de curand !!!
                if (!quote) {
                    // este separator ... spatziu, etc ...
                    // !oper adaugat de curand
                    if ((!oper) && (Contains2(SEPARATORS, c))) {
                        if (result != (char *)"")
                            break;
                        else {
                            only_numbers = 1;
                            separator    = 1;
                        }
                    }
                    if (!separator) {
                        // !oper adaugat de curand
                        if ((!oper) && (Contains2(ATOM_MEMBER, c))) {
                            if (!oper)
                                result += c;
                            else
                                break;
                        } else {
                            if (!oper)
                                break;
                            else {
                                // if (oper) && (!IsOperator(result+AnsiString(c)));
                                if (oper) {
                                    AnsiString tmp(result);
                                    tmp += c;

                                    INTEGER id = GetID(tmp);
                                    if ((id <= __START_OPERATORS) || (id >= __END_OPERATORS))
                                        break;
                                    else
                                        result += c;
                                } else
                                    result += c;
                            }
                        }
                    }
                } else {
                    // am o variabila in ghilimele ... anuntz asta ...

                    /*if ((quote=='"') && (c=='$') && (!escape) && (var_cnt<MAX_STR_VAR)) {
                     *  // retin pozitzia de la care incepe variabila ...
                     *  var_pos[var_cnt++]=result.Length();
                     * }*/
                    if ((quote == '"') && (c == '$') && (!escape)) {
                        virtual_result     = "+";
                        has_virtual_result = 1;
                        is_end             = 0;
                        put_prefix         = 1;
                        result            += quote;
                        in_formatted       = 1;
                        break;
                    }

                    // daca sunt in ghilimele ... incarc tot ...
                    result += c;
                    if (escape)
                        escape = 0;
                }
            }
        }
    }
    long old_position = position;
    position = i + (c == NEW_LINE);

    // verific daca nu cumva este o constanta !

    /*if ((ConstantList) && (!no_constants)) {
     *  INTEGER Count=ConstantList->Count();
     *  INTEGER POS;
     *  do {
     *      POS=0;
     *      for (INTEGER i=0;i<Count;i++) {
     *          VariableDESCRIPTOR *VD=(VariableDESCRIPTOR *)(*ConstantList)[i];
     *          if (VD->name==result) {
     *              (*Target)=VD->value+" "+(Target->c_str()+position);
     *              position=0;
     *              return NextAtom();
     *
     *              break;
     *          }
     *      }
     *  } while (POS);
     * }*/
    //return(result);
}

/*int AnsiParser::StrVarCount() {
 *  return this->var_cnt;
 * }
 *
 * long *AnsiParser::StrVar() {
 *  return this->var_pos;
 * }
 */

/*AnsiString AnsiParser::GetVariableName(int start, AnsiString sval, int *end) {
 *  int len=sval.Length();
 *  AnsiString res="";
 *  int i;
 *  for (i=start;i<len;i++) {
 *      char c=sval[i];
 *      if (Contains2(ATOM_MEMBER,c))
 *          res+=c;
 *      else
 *          break;
 *  }
 * *end=i;
 *  return res;
 * }
 *
 * AnsiString AnsiParser::GetSubStr(AnsiString sval, int start, int end) {
 *  AnsiString res;
 *  if (end-start<=0)
 *      return res;
 *  res.LoadBuffer(sval.c_str()+start,end-start);
 *  return res;
 * }*/

long AnsiParser::LastLine() {
    return linia_curenta;
}
