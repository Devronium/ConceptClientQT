#ifndef __TOWRITETEXT_H
#define __TOWRITETEXT_H

#include "AnsiString.h"

class ToWriteText {
public:
    AnsiString text;

    int spacing;
    int wrapmode;
    int indent;
    int justify;

    ToWriteText() {
        spacing  = -1;
        wrapmode = -1;
        indent   = -1;
        justify  = -1;
    }
};
#endif //__TOWRITETEXT_H
