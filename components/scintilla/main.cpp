//------------ standard header -----------------------------------//
#include "stdlibrary.h"
//------------ end of standard header ----------------------------//
#include <stdarg.h>
#include "AnsiString.h"
#include <strings.h>
#include <vector>

#include "quickparser.h"
#include "ConceptTypes.h"
#include "ScintillaEventHandler.h"
#include <QWidget>
#include <QLabel>
#include <ScintillaEditBase.h>
//#include <QThread>
//#include <qtconcurrentrun.h>

#define CUSTOM_CONTROL_ID             1000
#define INCLUDE_FILE                  100000
#define EVENT_REQUEST_FILE            350
#define EVENT_UPDATE_CLASS_LIST       351

#define MAX_CTRLS                     0xFF
#define _CUSTOM_MADE_SCLEX_CONCEPT    0xFFFF

class DScintillaEditBase : public ScintillaEditBase {
public:
    QLabel *hintLabel             = NULL;
    ScintillaEventHandler *events = NULL;
    AnsiString            sugest_list;
    AnsiString            hint;
    int param_no   = 0;
    int sugest_len = 0;

    DScintillaEditBase(QWidget *parent) : ScintillaEditBase(parent) {
    }

    virtual void focusOutEvent(QFocusEvent *event) {
        if ((hintLabel) && (hintLabel->isVisible()))
            hintLabel->hide();
    }
};

/*typedef struct {
    //ScintillaEditBase  *sci_obj;
    AnsiString sugest_list;
    int        sugest_len;
    AnsiString hint;
    int        param_no;
   } _SCINTILLAS;

   //_SCINTILLAS Controls[1024];
   //std::vector<_SCINTILLAS> Controls;
   //int         ControlsCount = 0;

   _SCINTILLAS *GetInternalSCI(void *sci_obj) {
    for (int i = 0; i < ControlsCount; i++)
        if (Controls[i].sci_obj == sci_obj)
            return(&Controls[i]);
    return(0);
   }
 */

INVOKER    RemoteInvoker = 0;
static int sci_id        = 0;
AnsiList   IncludedList;
static int session_id = 0;
AnsiString buffered_text;
char       clean_flag = 0;
//QFuture<void> background_run;

typedef struct {
    char event_request_file;
    char event_update_class_list;
} ScintilaParams;

void SciRequestFileDef(void *owner, char *filename);
void SciRequestFileDef2(void *owner, char *filename);

#define SCINTILLA_MESSAGE_GET(sci_message)                                        \
    char *post_string = 0;                                                        \
    int len_post_string;                                                          \
    int sci_message = 0;                                                          \
    RemoteInvoker(context, INVOKE_GETPOSTSTRING, &post_string, &len_post_string); \
    sci_message = AnsiString(post_string).ToInt();

#define MESSAGES_WITH_RETURN(msg1, msg2)                                                                               \
    if (msg == msg1) {                                                                                                 \
        AnsiString S((char *)ret);                                                                                     \
        RemoteInvoker(context, INVOKE_SENDMESSAGE, (int)msg1, (char *)"", (int)0, (char *)S.c_str(), (int)S.Length()); \
    } else                                                                                                             \
    if (msg == msg2) {                                                                                                 \
        AnsiString S((long)ret);                                                                                       \
        RemoteInvoker(context, INVOKE_SENDMESSAGE, (int)msg2, (char *)"", (int)0, (char *)S.c_str(), (int)S.Length()); \
    }

sptr_t scintilla_send_message(ScintillaEditBase *sci, unsigned int iMessage, uptr_t wParam = 0, sptr_t lParam = 0) {
    return sci->send(iMessage, wParam, lParam);
}

void RegisterIcons(ScintillaEditBase *sci) {
/* XPM */
    static char *EVENTSP_xpm[] = {
        /*"30 16 50 1",
           "      c None",
           "0     c #FBF4F9",
           "1     c #F5F5F5",
           "2     c #EBEBEB",
           "3     c #F5D7ED",
           "4     c #E891D2",
           "5     c #D67BBD",
           "6     c #D276B9",
           "7     c #CF74B6",
           "8     c #BE89AF",
           "9     c #A2A2A2",
           "a     c #2E2E2E",
           "b     c #444444",
           "c     c #EDCBE3",
           "d     c #DD7DC4",
           "e     c #C05EA5",
           "f     c #BE5CA3",
           "g     c #B25297",
           "h     c #8E6681",
           "i     c #AAAAAA",
           "j     c #353535",
           "k     c #0C0C0C",
           "l     c #636363",
           "m     c #D8D8D8",
           "n     c #D06EB5",
           "o     c #75135A",
           "p     c #6E0C53",
           "q     c #4A0E39",
           "r     c #E3E3E3",
           "s     c #B3B3B3",
           "t     c #3B3B3B",
           "u     c #020202",
           "v     c #191919",
           "w     c #CF6DB4",
           "x     c #B5539A",
           "y     c #69074E",
           "z     c #620047",
           "A     c #420832",
           "B     c #242424",
           "C     c #BEBEBE",
           "D     c #858585",
           "E     c #484848",
           "F     c #9B9B9B",
           "G     c #C263A8",
           "H     c #B69AAD",
           "I     c #913F79",
           "J     c #590041",
           "K     c #3D082E",
           "L     c #BDB6BA",
           "M     c #948A91",
           "             000000000        ",
           "       12   345666666781      ",
           "     19ab   cddeffffffgh1     ",
           "   1ijklm   cnfopppppppqr     ",
           "  stuv92111 cwxyzzzzzzzAr     ",
           " 9uuukBBBBBrcwxyzzzzzzzAr     ",
           " rCCCCDkuuE2cwxyzzzzzzzAr     ",
           "     sjujs  cwxyzzzzzzzAr     ",
           "   rbuai1   cwxyzzzzzzzAr     ",
           "   1lF1     cGxyzzzzzzzAr     ",
           "             HIzJJJJJJJKr     ",
           "              LMMMMMMMMM1     ",
           "                              ",
           "                              ",
           "                              ",
           "                              \0"
           };*/
        "12 12 3 1",
        "   c None",
        ".	c #FF6666",
        "+	c #FF0000",
        ".+++.       ",
        "+   +       ",
        ".+++.       ",
        "  +         ",
        "  +         ",
        "  +   .++.  ",
        " ++  .++++. ",
        "  +  ++++++ ",
        "+++  ++++++ ",
        "     .++++. ",
        "      .++.  ",
        "            "
    };
/* XPM */
    static char *FUNCP_xpm[] = {
        /*
           "30 16 54 1",
           "      c None",
           "0     c #CACACA",
           "1     c #9B9B9B",
           "2     c #EBEBEB",
           "3     c #727272",
           "4     c #4C4C4C",
           "5     c #616161",
           "6     c #B9B9B9",
           "7     c #F6D6EE",
           "8     c #EECBE4",
           "9     c #EFE2EB",
           "a     c #DBDBDB",
           "b     c #2C2C2C",
           "c     c #F2F2F2",
           "d     c #ACACAC",
           "e     c #5F5F5F",
           "f     c #FAF1F8",
           "g     c #E389CC",
           "h     c #E080C7",
           "i     c #D06EB5",
           "j     c #CF6DB4",
           "k     c #B5629D",
           "l     c #D1C4CC",
           "m     c #A3A3A3",
           "n     c #515151",
           "o     c #D5D5D5",
           "p     c #D275B9",
           "q     c #A44289",
           "r     c #98367D",
           "s     c #96347B",
           "t     c #692B56",
           "u     c #E5E5E5",
           "v     c #6F6F6F",
           "w     c #7A7A7A",
           "x     c #1F1F1F",
           "y     c #D073B7",
           "z     c #C563AA",
           "A     c #75135A",
           "B     c #620047",
           "C     c #450534",
           "D     c #828282",
           "E     c #212121",
           "F     c #B2B2B2",
           "G     c #323232",
           "H     c #C76DAF",
           "I     c #C7C7C7",
           "J     c #C9AEC0",
           "K     c #9F4D87",
           "L     c #6F1155",
           "M     c #5E0044",
           "N     c #949494",
           "O     c #C9C2C6",
           "P     c #81707C",
           "Q     c #7E737B",
           "                              ",
           "      0123                    ",
           "      40 56  7888888889       ",
           "     abc de fghijjjjjjkl      ",
           "     mn  obcfppqrrrrrrstu     ",
           "     vw  2xafyzABBBBBBBCa     ",
           "     eD  cxofpzABBBBBBBCa     ",
           "     5D  cEofpzABBBBBBBCa     ",
           "     w3  2EufpzABBBBBBBCa     ",
           "     F4  oGcfpzABBBBBBBCa     ",
           "     uGc mv fHzABBBBBBBCa     ",
           "      eI e0  JKLMMMMMMMCa     ",
           "      oFcN    OPPPPPPPPQ2     ",
           "                              ",
           "                              ",
           "                              \0"
           };*/
        "12 12 2 1",
        "   c None",
        ".	c #FF0000",
        "  .......   ",
        "  ..   ..   ",
        "  .......   ",
        "  .......   ",
        "     .      ",
        "     .      ",
        "     .      ",
        "     .      ",
        "     ..     ",
        "     .      ",
        "     ...    ",
        "            "
    };
/* XPM */
    static char *PROPP_xpm[] = {
        /*"30 16 36 1",
           "      c None",
           "0     c #F4C8E9",
           "1     c #E495D0",
           "2     c #D685C1",
           "3     c #C88FB8",
           "4     c #F5F5F5",
           "5     c #E3ACD4",
           "6     c #E283CA",
           "7     c #D372B9",
           "8     c #C361A8",
           "9     c #BA599F",
           "a     c #966787",
           "b     c #DADADA",
           "c     c #D4D3D4",
           "d     c #D06EB5",
           "e     c #A24087",
           "f     c #731158",
           "g     c #570E42",
           "h     c #CDCBCC",
           "i     c #323232",
           "j     c #0D0D0D",
           "k     c #CF6CB4",
           "l     c #96357C",
           "m     c #630148",
           "n     c #4B0237",
           "o     c #E4E4E4",
           "p     c #9F9F9F",
           "q     c #8E8E8E",
           "r     c #464646",
           "s     c #252525",
           "t     c #DAB3CF",
           "u     c #4E043B",
           "v     c #B094A6",
           "w     c #71275C",
           "x     c #50043C",
           "y     c #3F072F",
           "                              ",
           "                              ",
           "             01222222234      ",
           "             5678888889a4     ",
           "       bcccb 5defffffffgh     ",
           "      4ijjji45klmmmmmmmnh     ",
           "       ooooo 5klmmmmmmmnh     ",
           "       pqqqp 5klmmmmmmmnh     ",
           "      4rsssr45klmmmmmmmnh     ",
           "       44444 5klmmmmmmmnh     ",
           "             t8lmmmmmmmuh     ",
           "              vwxuuuuuuyh     ",
           "               chhhhhhhh4     ",
           "                              ",
           "                              ",
           "                              \0"
           };*/
        "12 12 4 1",
        "   c None",
        ".	c #666666",
        "+	c #000000",
        "@	c #FFAE00",
        "            ",
        " .++.@@@    ",
        " + @+@@@@   ",
        " +@@+@@@@@  ",
        " @@@+@@@@@@ ",
        " @@@+@@@@@@ ",
        " ++++@@@@@@ ",
        " +..+@@@@@  ",
        " ++++@@@@   ",
        "    @@@@    ",
        "            ",
        "            "
    };



    static char *VAR_xpm[] = {
        "12 12 2 1",
        "   c None",
        ".	c #0000FF",
        "   .........",
        "  .   .   ..",
        " .   .   . .",
        ".........  .",
        ".   .   .  .",
        ".   .   . ..",
        ".   .   .. .",
        ".........  .",
        ".   .   .  .",
        ".   .   . . ",
        ".   .   ..  ",
        ".........   "
    };
/* XPM */

/*    static char *VAR_xpm[] = {
        "30 16 28 1",
        "      c None",
        "0     c #EEF8E7",
        "1     c #ECF6E5",
        "2     c #E5F5DA",
        "3     c #ACE384",
        "4     c #9BD573",
        "5     c #95CF6D",
        "6     c #9CB88D",
        "7     c #E1F1D6",
        "8     c #7CB554",
        "9     c #5B9533",
        "a     c #4F842A",
        "b     c #A5ADA2",
        "c     c #286200",
        "d     c #265B00",
        "e     c #90988B",
        "f     c #58922F",
        "g     c #8F9789",
        "h     c #579230",
        "i     c #276100",
        "j     c #57912F",
        "k     c #EDF3E9",
        "l     c #82B066",
        "m     c #548E2C",
        "n     c #E1E4DF",
        "o     c #6B7E5F",
        "p     c #526744",
        "q     c #ABAEA9",
        "                              ",
        "       0111111111111111       ",
        "      234555555555555556      ",
        "      74899999999999999ab     ",
        "      759ccccccccccccccde     ",
        "      75fccccccccccccccdg     ",
        "      75hccccccccccccccdg     ",
        "      75hiiiiiiiiiiiiiidg     ",
        "      75jccccccccccccccdg     ",
        "      klmiiiiiiiiiiiiiidg     ",
        "       nopppppppppppppppq     ",
        "                              ",
        "                              ",
        "                              ",
        "                              ",
        "                              \0"
    };
 */
/* XPM */

/*    static char *EVENTSP_xpm[] = {
        "30 16 50 1",
        "      c None",
        "0     c #FBF4F9",
        "1     c #F5F5F5",
        "2     c #EBEBEB",
        "3     c #F5D7ED",
        "4     c #DD7DC4",
        "5     c #D377BA",
        "6     c #CE6DB3",
        "7     c #B69AA9",
        "8     c #A9A7A5",
        "9     c #272726",
        "a     c #484848",
        "b     c #EDCBE3",
        "c     c #C05EA5",
        "d     c #BE5CA3",
        "e     c #B5539A",
        "f     c #8E6681",
        "g     c #373737",
        "h     c #0C0C0C",
        "i     c #716B6F",
        "j     c #DDD8D7",
        "k     c #75135A",
        "l     c #6E0C53",
        "m     c #600948",
        "n     c #E2E2E2",
        "o     c #BAB9BA",
        "p     c #020202",
        "q     c #1A1A1A",
        "r     c #69074E",
        "s     c #620047",
        "t     c #8F5274",
        "u     c #938B91",
        "v     c #555555",
        "w     c #6A0D4F",
        "x     c #913F79",
        "y     c #916F7A",
        "z     c #F3EFEE",
        "A     c #EBC99A",
        "B     c #32200B",
        "C     c #B8975E",
        "D     c #F8D568",
        "E     c #5D331D",
        "F     c #F49629",
        "G     c #EB730B",
        "H     c #A9490C",
        "I     c #AA8B8A",
        "J     c #FB7A0A",
        "K     c #D1690E",
        "L     c #FFF9E8",
        "M     c #953711",
        "            000000000         ",
        "      12   345555555671       ",
        "    189a   b44cddddddef1      ",
        "  18ghij   b6dklllllllmn      ",
        " ogpq82111 b6ersssssskijnnn   ",
        "8ppph99999nb6ersssssstiaaaai  ",
        "noooouhppa2b6ersssssstvqoiag  ",
        "    ogpgo  b6ersssswxyah8ug9z ",
        "  nap981   b6ersssstAAvBCDEBCn",
        "  1i81     bcersssstDDFGFFFGHI",
        "            7xsmmmmtDFJKBEGJHI",
        "             ouuuuu7DFJKBEGJHI",
        "                   LDFJGBEJJHI",
        "                   LDFJGMHGGHI",
        "                    AHMMMMMMEI",
        "                     1zzzzzzz "
    };
 */
    static char *EVENTS_xpm[] = {
        "12 12 3 1",
        "   c None",
        ".	c #8080FF",
        "+	c #0000FF",
        "            ",
        "    .+.     ",
        "    .+.     ",
        "   +++++    ",
        "  +++.+++   ",
        "  + +.+ +   ",
        "  + +.+ +   ",
        "  + +.+ +   ",
        "    + +     ",
        "    + +     ",
        "    + +     ",
        "            "
    };


/* XPM */

/*    static char *FUNCP_xpm[] = {
        "30 16 49 1",
        "      c None",
        "0     c #CAC9CA",
        "1     c #949390",
        "2     c #ECE9EB",
        "3     c #767676",
        "4     c #4A4A4A",
        "5     c #686868",
        "6     c #AFAFAF",
        "7     c #F6D6EE",
        "8     c #EECBE4",
        "9     c #D7D7D7",
        "a     c #2D2D2C",
        "b     c #F2F2F2",
        "c     c #5B5B5B",
        "d     c #FAF1F8",
        "e     c #D275B9",
        "f     c #D06EB5",
        "g     c #CF6DB4",
        "h     c #C563AA",
        "i     c #D1C4CC",
        "j     c #FAF0F7",
        "k     c #A8508F",
        "l     c #98367D",
        "m     c #96347B",
        "n     c #75135A",
        "o     c #E3E3E3",
        "p     c #271D11",
        "q     c #D073B7",
        "r     c #620047",
        "s     c #7D5873",
        "t     c #84707C",
        "u     c #8F5274",
        "v     c #6C0F53",
        "w     c #F3EFEE",
        "x     c #EEE0DC",
        "y     c #F8D568",
        "z     c #B8975E",
        "A     c #613A19",
        "B     c #C76DAF",
        "C     c #F49629",
        "D     c #EB730B",
        "E     c #A9490C",
        "F     c #AA8E8B",
        "G     c #FB7A0A",
        "H     c #D1690E",
        "I     c #AE3C00",
        "J     c #FFF9E8",
        "K     c #8D3517",
        "L     c #E4B9A5",
        "     0123                     ",
        "     40 56  7888888882        ",
        "    9ab 6c deefgggggghi       ",
        "    6c  9abjeekllllllmno      ",
        "    53  2p9jqhnrrrrrrns0ooo   ",
        "    ct  bp9jehnrrrrrru344443  ",
        "    5t  ba9jehnrrrrrrscp634a  ",
        "    33  2aojehnrrrrvut4p614aw ",
        "    64  9abjehnrrrruxy4pzyApzx",
        "    oab 65 jBhnrrrruyyCDCCCDEF",
        "     c0 c0  ikvrrrruyCGHpADGIF",
        "     96b1    0tttttFyCGHpADGIF",
        "                   JyCGDpAGGIF",
        "                   JyCGDKEDDIF",
        "                    LEKKKKKKAF",
        "                     bwwwwwww \0"
    };
 */

    static char *FUNCTS_xpm[] = {
        "12 12 3 1",
        "   c None",
        ".	c #6666FF",
        "+	c #0000FF",
        "            ",
        "  .++++++.  ",
        "  +      +  ",
        "  + ++++ +  ",
        "  + +    +  ",
        "  + +    +  ",
        "  + +++  +  ",
        "  + +    +  ",
        "  + +    +  ",
        "  +      +  ",
        "  .++++++.  ",
        "            "
    };
/* XPM */

/*    static char *PROPP_xpm[] = {
        "30 16 49 1",
        "      c None",
        "0     c #F1D4E3",
        "1     c #E099CE",
        "2     c #D685C1",
        "3     c #F5F4F5",
        "4     c #E3ACD4",
        "5     c #D270B7",
        "6     c #C361A8",
        "7     c #C061A5",
        "8     c #897075",
        "9     c #E2E2E2",
        "a     c #D6D6D6",
        "b     c #CF6CB4",
        "c     c #A6558F",
        "d     c #731158",
        "e     c #722F5F",
        "f     c #CDCACC",
        "g     c #2D2D2C",
        "h     c #101010",
        "i     c #96357C",
        "j     c #630148",
        "k     c #796A75",
        "l     c #4F4F4F",
        "m     c #454545",
        "n     c #925270",
        "o     c #535353",
        "p     c #B4A9AB",
        "q     c #959595",
        "r     c #6A0D50",
        "s     c #F3EFEE",
        "t     c #DED8C8",
        "u     c #E5C894",
        "v     c #635530",
        "w     c #32200B",
        "x     c #B8975E",
        "y     c #F8D468",
        "z     c #F49629",
        "A     c #EB730B",
        "B     c #A9490C",
        "C     c #A88887",
        "D     c #FB7A0A",
        "E     c #D1690E",
        "F     c #602D0E",
        "G     c #AE3C00",
        "H     c #B29797",
        "I     c #4F043B",
        "J     c #FFF8E8",
        "K     c #8F3718",
        "L     c #8D3517",
        "                              ",
        "                              ",
        "           01222222223        ",
        "           425666666783       ",
        "     9aaa9 4bcdddddddef9999   ",
        "    3ghhhg34bijjjjjjjk8llmmk  ",
        "     99999 4bijjjjjjjnohpkmg  ",
        "     qqqqq 4bijjjjjrn8ohpqmgs ",
        "    3mgggm34bijjjjjntuvwxyvwx9",
        "     33333 4bijjjjjnyyzAzzzABC",
        "           46ijjjjjnyzDEwFADGC",
        "            HeIIIIInyzDEwFADGC",
        "             affffftyzDAwFDDGC",
        "                   JyzDAKBAAGC",
        "                    uBLLLLLLFH",
        "                     3sssssss "
    };
 */

    static char *PROP_xpm[] = {
        "12 12 2 1",
        "   c None",
        ".	c #FFAE00",
        "            ",
        "    ....    ",
        "   ......   ",
        "  ........  ",
        " .......... ",
        " .......... ",
        " .......... ",
        "  ........  ",
        "   ......   ",
        "    ....    ",
        "            ",
        "            "
    };

/* XPM */
    static char *VARP_xpm[] = {
/*        "30 16 49 1",
        "      c None",
        "0     c #ECF6E5",
        "1     c #E5F5DA",
        "2     c #9BD573",
        "3     c #95CF6D",
        "4     c #A1AF9A",
        "5     c #E1F1D6",
        "6     c #7DA35B",
        "7     c #5B9533",
        "8     c #558D2D",
        "9     c #286200",
        "a     c #6A8854",
        "b     c #E8E2DE",
        "c     c #579230",
        "d     c #787977",
        "e     c #474747",
        "f     c #525352",
        "g     c #271B0B",
        "h     c #323230",
        "i     c #276100",
        "j     c #32690D",
        "k     c #618B3D",
        "l     c #8A9771",
        "m     c #F3EFEE",
        "n     c #6F9444",
        "o     c #D7D88F",
        "p     c #5B612E",
        "q     c #C9994D",
        "r     c #ECCB67",
        "s     c #EEF6E8",
        "t     c #FCE576",
        "u     c #EB8D25",
        "v     c #F18115",
        "w     c #A9490C",
        "x     c #A88887",
        "y     c #6B7E5F",
        "z     c #526744",
        "A     c #FFAB39",
        "B     c #FD7C0B",
        "C     c #D1690E",
        "D     c #602D0E",
        "E     c #EB730B",
        "F     c #AE3C00",
        "G     c #F97708",
        "H     c #F4F0F0",
        "I     c #FCA232",
        "J     c #8F3718",
        "K     c #8D3517",
        "L     c #B29797",
        "                              ",
        "       0000000000000000       ",
        "      122333333333333334      ",
        "      5267777777777777784     ",
        "      5379999999999998aa4bb   ",
        "      53c999999999999adeeeed  ",
        "      53c999999999999afgddeh  ",
        "      53ciiiiiiiiiijk6fgdlhhm ",
        "      53c9999999999noopgqrpgqb",
        "      s68iiiiiiiiiintruuuuuvwx",
        "       byzzzzzzzzzzltABCgDEBFx",
        "                   stABCgDGBFx",
        "                   HtABEgDBGFx",
        "                   HrIGEJwEGFx",
        "                    rwKKKKKKDL",
        "                     Hmmmmmmm "
    };*/
        "12 12 2 1",
        "   c None",
        ".	c #FF0000",
        "   .........",
        "  .   .   ..",
        " .   .   . .",
        ".........  .",
        ".   .   .  .",
        ".   .   . ..",
        ".   .   .. .",
        ".........  .",
        ".   .   .  .",
        ".   .   . . ",
        ".   .   ..  ",
        ".........   "
    };


    scintilla_send_message(sci, SCI_REGISTERIMAGE, (uptr_t)1, (sptr_t)VAR_xpm);
    scintilla_send_message(sci, SCI_REGISTERIMAGE, (uptr_t)2, (sptr_t)FUNCTS_xpm);
    scintilla_send_message(sci, SCI_REGISTERIMAGE, (uptr_t)3, (sptr_t)PROP_xpm);
    scintilla_send_message(sci, SCI_REGISTERIMAGE, (uptr_t)4, (sptr_t)EVENTS_xpm);

    scintilla_send_message(sci, SCI_REGISTERIMAGE, (uptr_t)11, (sptr_t)VARP_xpm);
    scintilla_send_message(sci, SCI_REGISTERIMAGE, (uptr_t)12, (sptr_t)FUNCP_xpm);
    scintilla_send_message(sci, SCI_REGISTERIMAGE, (uptr_t)13, (sptr_t)PROPP_xpm);
    scintilla_send_message(sci, SCI_REGISTERIMAGE, (uptr_t)14, (sptr_t)EVENTSP_xpm);
}

int GetLine(ScintillaEditBase *sci) {
    int caret = scintilla_send_message(sci, SCI_GETCURRENTPOS, (uptr_t)0, (sptr_t)0);
    int line  = scintilla_send_message(sci, SCI_LINEFROMPOSITION, (uptr_t)caret, (sptr_t)0);

    /*int lineStart = scintilla_send_message(sci, SCI_POSITIONFROMLINE, (uptr_t)line, (sptr_t)0);
     * return caret - lineStart;*/
    return line;
}

AnsiString GetSciText(ScintillaEditBase *sci) {
    int len = scintilla_send_message(sci, SCI_GETTEXTLENGTH, (uptr_t)0, (sptr_t)0);

    if (len > 0) {
        char *text = new char[len + 1];
        if (scintilla_send_message(sci, SCI_GETTEXT, (uptr_t)len + 1, (sptr_t)text)) {
            AnsiString ret = text;
            delete[] text;
            return ret;
        }
        delete[] text;
        return "";
    }
    return "";
}

int CheckDone() {
    int cnt = IncludedList.Count();

    for (int i = 0; i < cnt; i++) {
        Included *C__INC = (Included *)IncludedList.Item(i);
        if (!C__INC->last_mod)
            return 0;
    }
    return 1;
}

void PreParseAll(ScintillaEditBase *sci, ScintillaEditBase *first_scintilla) {
    int        cnt  = IncludedList.Count();
    AnsiString text = GetSciText(sci);

    for (int i = 0; i < cnt; i++) {
        Included *C__INC = (Included *)IncludedList.Item(i);
        if (C__INC->ses_requested != session_id) {
            C__INC->ses_requested = session_id;
            PreParse(first_scintilla, &C__INC->Content, SciRequestFileDef);
        }
    }
    PreParse(first_scintilla, &text, SciRequestFileDef);
}

void InitClassList(AnsiString *text, ScintillaEditBase *first_scintilla) {
    Parse(first_scintilla, text, SciRequestFileDef, 0, clean_flag);
    clean_flag = 0;
    if ((RemoteInvoker) && (first_scintilla)) {
        ScintilaParams *ptr = 0;
        RemoteInvoker(first_scintilla, INVOKE_GETEXTRAPTR, &ptr);
        if ((ptr) && (ptr->event_update_class_list))
            RemoteInvoker(first_scintilla, INVOKE_FIREEVENT, (int)EVENT_UPDATE_CLASS_LIST, GetProjectList().c_str());
    }
}

void ParseAll(ScintillaEditBase *sci, ScintillaEditBase *first_scintilla) {
    AnsiString text = GetSciText(sci);

    if (CheckDone()) {
        Parse(first_scintilla, &text, SciRequestFileDef2, 0, clean_flag);
        clean_flag = 0;
        if ((RemoteInvoker) && (first_scintilla)) {
            ScintilaParams *ptr = 0;
            RemoteInvoker(first_scintilla, INVOKE_GETEXTRAPTR, &ptr);
            if ((ptr) && (ptr->event_update_class_list))
                if (ProjectListChanged())
                    RemoteInvoker(first_scintilla, INVOKE_FIREEVENT, (int)EVENT_UPDATE_CLASS_LIST, GetCheckedProjectList().c_str());
        }
    }
}

void AddParse(char *filename, char *content, ScintillaEditBase *first_scintilla) {
    //std::cout << content;
    int cnt = IncludedList.Count();

    for (int i = 0; i < cnt; i++) {
        Included *C__INC = (Included *)IncludedList.Item(i);
        if (C__INC->Name == filename) {
            C__INC->Content  = content;
            C__INC->last_mod = 1;

            C__INC->ses_requested = session_id;

            //clean_flag = 1;

            if (first_scintilla) {
                PreParse(first_scintilla, &C__INC->Content, SciRequestFileDef);
            }
        }
    }
    //session_id++;
}

void SciRequestFileDef2(void *first_scintilla, char *filename) {
    if ((RemoteInvoker) && (first_scintilla)) {
        ScintilaParams *ptr = 0;
        RemoteInvoker((ScintillaEditBase *)first_scintilla, INVOKE_GETEXTRAPTR, &ptr);
        if ((ptr) && (ptr->event_request_file)) {
            int cnt = IncludedList.Count();
            for (int i = 0; i < cnt; i++) {
                Included *C__INC = (Included *)IncludedList.Item(i);
                if (C__INC->Name == filename) {
                    if (C__INC->ses_requested_parse != session_id) {
                        C__INC->ses_requested_parse = session_id;
                        Parse((ScintillaEditBase *)first_scintilla, &C__INC->Content, SciRequestFileDef2, 1, 0);
                    }
                    return;
                }
            }
            AnsiString fname(filename);
            if ((!fname.Length()) || (fname[(int)fname.Length() - 1] == '.'))
                return;
            Included *C__INC = new Included;
            C__INC->Name                = filename;
            C__INC->last_mod            = 0;
            C__INC->ses_requested       = -1;
            C__INC->ses_requested_parse = -1;
            IncludedList.Insert(C__INC, 0, DATA_INCLUDED);
            // nu-l am, il adaug ...
            RemoteInvoker((ScintillaEditBase *)first_scintilla, INVOKE_FIREEVENT, (int)EVENT_REQUEST_FILE, filename);
        }
    }
}

void SciRequestFileDef(void *first_scintilla, char *filename) {
    if ((RemoteInvoker) && (first_scintilla)) {
        ScintilaParams *ptr = 0;
        RemoteInvoker((ScintillaEditBase *)first_scintilla, INVOKE_GETEXTRAPTR, &ptr);
        if ((ptr) && (ptr->event_request_file)) {
            int cnt = IncludedList.Count();
            for (int i = 0; i < cnt; i++) {
                Included *C__INC = (Included *)IncludedList.Item(i);
                if (C__INC->Name == filename) {
                    if (!C__INC->last_mod)
                        C__INC->last_mod = 1;
                    if (C__INC->ses_requested != session_id) {
                        C__INC->ses_requested = session_id;
                        PreParse((ScintillaEditBase *)first_scintilla, &C__INC->Content, SciRequestFileDef);
                    }
                    return;
                }
            }
            AnsiString fname(filename);
            if ((!fname.Length()) || (fname[(int)fname.Length() - 1] == '.'))
                return;
            Included *C__INC = new Included;
            C__INC->Name                = filename;
            C__INC->last_mod            = 0;
            C__INC->ses_requested       = -1;
            C__INC->ses_requested_parse = -1;
            IncludedList.Insert(C__INC, 0, DATA_INCLUDED);
            // nu-l am, il adaug ...
            RemoteInvoker((ScintillaEditBase *)first_scintilla, INVOKE_FIREEVENT, (int)EVENT_REQUEST_FILE, filename);
        }
    }
}

//#include <iostream>
AnsiString GetLinePrefix(ScintillaEditBase *sci, int caret) {
    int line      = scintilla_send_message(sci, SCI_LINEFROMPOSITION, (uptr_t)caret, (sptr_t)0);
    int lineStart = scintilla_send_message(sci, SCI_POSITIONFROMLINE, (uptr_t)line, (sptr_t)0);

    int        end   = caret;
    AnsiString text  = GetSciText(sci);
    char       *ptxt = text.c_str();

    for (int k = lineStart; k < caret; k++)
        if ((ptxt[k] != ' ') &&
            (ptxt[k] != '\t')) {
            end = k;
            break;
        }
    ptxt[end] = 0;

    AnsiString result = text.c_str() + lineStart;
    return result;
}

int ReplaceLinePrefix(ScintillaEditBase *sci, int caret, AnsiString new_prefix) {
    int line      = scintilla_send_message(sci, SCI_LINEFROMPOSITION, (uptr_t)caret, (sptr_t)0);
    int lineStart = scintilla_send_message(sci, SCI_POSITIONFROMLINE, (uptr_t)line, (sptr_t)0);

    int        end   = caret;
    AnsiString text  = GetSciText(sci);
    char       *ptxt = text.c_str();

    for (int k = lineStart; k < caret; k++)
        if ((ptxt[k] != ' ') &&
            (ptxt[k] != '\t')) {
            end = k;
            break;
        }
    scintilla_send_message(sci, SCI_SETSEL, (uptr_t)lineStart, (sptr_t)end);
    scintilla_send_message(sci, SCI_REPLACESEL, (uptr_t)0, (sptr_t)new_prefix.c_str());
    return new_prefix.Length() - (end - lineStart);
}

AnsiString GetPrecLinePrefix(ScintillaEditBase *sci, int spos = -1) {
    int caret = spos;

    if (spos == -1)
        caret = scintilla_send_message(sci, SCI_GETCURRENTPOS, (uptr_t)0, (sptr_t)0) - 1;

    int        line      = scintilla_send_message(sci, SCI_LINEFROMPOSITION, (uptr_t)caret, (sptr_t)0);
    int        lineStart = scintilla_send_message(sci, SCI_POSITIONFROMLINE, (uptr_t)line, (sptr_t)0);
    AnsiString txt       = GetSciText(sci);
    int        len       = txt.Length();
    char       *ptxt     = txt.c_str();

    AnsiString extra;
    int        first_char = caret;
    for (int k = caret; k >= 0; k--)
        if ((ptxt[k] != ' ') &&
            (ptxt[k] != '\r') &&
            (ptxt[k] != '\n') &&
            (ptxt[k] != '\t')) {
            first_char = k;
            break;
        }
    if ((txt[first_char] == '{') || (txt[first_char] == '(') || (txt[first_char] == ']')) {
        long use_tabs = scintilla_send_message(sci, SCI_GETUSETABS, (uptr_t)0, (sptr_t)0);
        if (use_tabs)
            extra = (char *)"/t";
        else {
            long tab_size = scintilla_send_message(sci, SCI_GETTABWIDTH, (uptr_t)0, (sptr_t)0);
            for (int j = 0; j < tab_size; j++)
                extra += (char *)" ";
        }
    }

    int i;
    for (i = lineStart; i < len; i++)
        if ((ptxt[i] != ' ') && (ptxt[i] != '\t'))
            break;
    ptxt[i] = 0;
    AnsiString result = ptxt + lineStart;
    return result + extra;
}

//#include <iostream>
AnsiString GetLeft(ScintillaEditBase *sci, int& pos) {
    AnsiString text   = GetSciText(sci);
    char       *ptext = text.c_str();
    int        caret  = pos;

    if (pos == -1)
        caret = scintilla_send_message(sci, SCI_GETCURRENTPOS, (uptr_t)0, (sptr_t)0);
    int  i       = 0;
    char trimmed = 0;
    for (i = caret - 2; i >= 0; i--) {
        if (trimmed) {
            if (!Contains2(IDENTIFIER, ptext[i]))
                break;
        } else
        if ((ptext[i] != ' ') &&
            (ptext[i] != '\r') &&
            (ptext[i] != '\n') &&
            (ptext[i] != '\t')) {
            trimmed = 1;
            caret   = i + 2;
            continue;
        }
    }
    pos = i + 1;
    ptext[caret - 1] = 0;
    AnsiString result = ptext + i + 1;
    return result;
}

char GetChar(ScintillaEditBase *sci, int& caret) {
    AnsiString text   = GetSciText(sci);
    char       *ptext = text.c_str();

    for (int i = caret; i >= 0; i--)
        if ((ptext[i] != ' ') &&
            (ptext[i] != '\r') &&
            (ptext[i] != '\n') &&
            (ptext[i] != '\t')) {
            caret = i + 1;
            return text[i];
        }
    return 0;
}

#define MAX_LEFT    255
AnsiString GetFinalType(ScintillaEditBase *sci, AnsiString *hint = 0, int *lpos = 0) {
    int        pos   = -1;
    int        lefts = 0;
    AnsiString left[MAX_LEFT];

    left[lefts++] = GetLeft(sci, pos);
    AnsiString text = GetSciText(sci);
    AnsiString tref("this");

    pos--;
    while (lefts < MAX_LEFT) {
        int last = left[lefts - 1].Length() - 1;
        if ((left[lefts - 1][last] == ')') || (left[lefts - 1][last] == ']')) {
            pos = scintilla_send_message(sci, SCI_BRACEMATCH, (uptr_t)/*lStart-1*/ pos + last + 1, (sptr_t)0);
            if (pos < 0)
                //std::cout << "\nERROR\n\n";
                return "";
            pos++;
            left[lefts - 1] = GetLeft(sci, pos);
            pos--;
            //std::cout << "LEFT:" << left[lefts-1] << "\n";
            continue;
        }
        char ch = GetChar(sci, pos);
        if (ch != '.')
            break;
        left[lefts++] = GetLeft(sci, pos);
        pos--;
    }

    //std::cout << "Have " << lefts << " lefts\n";
    AnsiString type;
    AnsiString prec_type;
    char       first = 1;
    for (int i = lefts - 1; i >= 0; i--) {
        if (first) {
            type = GetElementType(left[i], GetLine(sci));
            //std::cout << "Type: " << type.c_str();
            if (type == (char *)"") {
                AnsiString tmp = GetElementType(tref, GetLine(sci));
                type = GetMemberType(tmp, left[i]);
                //std::cout << "Type: " << type.c_str();
                if (type == (char *)"")
                    type = left[i];


                /*else
                 *   i++;*/
                //break;
            }
            first = 0;
        } else {
            //std::cout << "TYPE:" << type << " LEFT:" << left[i] << "\n";
            type = GetMemberType(type, left[i]);
            if (type == (char *)"")
                break;
        }
        prec_type = type;
    }
    if (hint) {
        if (lefts == 1) {
            AnsiString el_type = GetElementType(tref, GetLine(sci));
            *hint = GetHint(el_type, left[0]);
        } else
            *hint = GetHint(prec_type, left[0]);
    }
    if (lpos)
        *lpos = pos;
    return type;
}

void scitilla_notify(ScintillaEditBase *widget, SCNotification *scn) {
    static char       last_rec_char    = 0;
    ScintillaEditBase *first_scintilla = widget;
    //_SCINTILLAS           *internal        = GetInternalSCI(widget);
    DScintillaEditBase *sci = (DScintillaEditBase *)widget;

    switch (scn->nmhdr.code) {
        case SCN_MARGINCLICK:
            {
                if (scn->margin == 2) {
                    long lLine = scintilla_send_message(sci, SCI_LINEFROMPOSITION, scn->position, 0);
                    scintilla_send_message(sci, SCI_TOGGLEFOLD, (uptr_t)lLine, (sptr_t)0);
                }
            }
            break;

        case SCN_CHARADDED:
            if (scn->ch == '.') {
                int        pos  = -1;
                AnsiString left = GetFinalType(sci);
                if (left.Length()) {
                    AnsiString list = GetSciList(left);

                    /*if (left==(char *)"this") {
                        AnsiString text=GetSciText(sci);
                        Parse(first_scintilla, &text, NULL);
                       } else*/
                    ParseAll(sci, first_scintilla);
                    if (!list.Length())
                        list = GetSciList(left);

                    if (list.Length())
                        scintilla_send_message(sci, SCI_AUTOCSHOW, (uptr_t)0, (sptr_t)list.c_str());
                }
            } else
            if (scn->ch == '(') {
                AnsiString hint;
                //AnsiString text=GetSciText(sci);
                //Parse(&text,SciRequestFileDef);
                ParseAll(sci, first_scintilla);
                GetFinalType(sci, &hint);
                if (hint != (char *)"") {
                    if (!sci->hintLabel) {
                        sci->hintLabel = new QLabel(/*widget->window()*/);
                        //hintLabel->setAttribute(Qt::WA_ShowWithoutActivating);
                        sci->hintLabel->setWindowFlags(Qt::ToolTip);
                        sci->hintLabel->setStyleSheet("QLabel { background: white; color: black; border: 1px solid; }");
                    }

                    int pos = scintilla_send_message(sci, SCI_GETCURRENTPOS, (uptr_t)0, (sptr_t)0);

                    int start_bold = hint.Pos("(");
                    int end_bold   = hint.Pos(",");
                    if (end_bold <= 0)
                        end_bold = hint.Pos(")");

#ifndef USE_SCINTILLA_CALLTIP
                    int height = scintilla_send_message(widget, SCI_STYLEGETSIZE, (uptr_t)0, (sptr_t)0);
                    int x      = scintilla_send_message(sci, SCI_POINTXFROMPOSITION, (uptr_t)0, (sptr_t)pos);
                    int y      = scintilla_send_message(sci, SCI_POINTYFROMPOSITION, (uptr_t)0, (sptr_t)pos);
                    x = widget->mapToGlobal(QPoint(x, y)).x();
                    y = widget->mapToGlobal(QPoint(x, y)).y() + height + 5;

                    QString shint(hint.c_str());
                    if ((start_bold > 0) && (end_bold > 0)) {
                        end_bold--;
                        shint  = shint.insert(end_bold, "</font></b>");
                        shint  = shint.insert(start_bold, "<b><font color='green'>");
                        shint  = shint.replace("\n\n", "\n\n<font color='gray'>");
                        shint  = shint.replace("\n", "<br/>\n");
                        shint += "</font>";
                    }

                    sci->hintLabel->move(x, y);
                    sci->hintLabel->setText(shint);
                    sci->hintLabel->adjustSize();
                    sci->hintLabel->show();
#else
                    scintilla_send_message(sci, SCI_CALLTIPSHOW, (uptr_t)pos - 1, (sptr_t)hint.c_str());
                    if ((start_bold > 0) && (end_bold > 0)) {
                        end_bold--;
                        scintilla_send_message(sci, SCI_CALLTIPSETHLT, (uptr_t)start_bold, (sptr_t)end_bold);
                    }
#endif
                    sci->hint     = hint;
                    sci->param_no = 0;
                }
            } else
            if (scn->ch == ',') {
#ifndef USE_SCINTILLA_CALLTIP
                if ((sci->hintLabel) && (sci->hintLabel->isVisible()) && (sci->hint.Length())) {
#else
                if ((scintilla_send_message(sci, SCI_CALLTIPACTIVE, 0, 0)) && (sci->hint.Length())) {
#endif
                    sci->param_no++;

                    char *ptr = sci->hint.c_str();

                    char *ptr2 = ptr;
                    for (int i = 0; i < sci->param_no; i++)
                        if ((ptr2) && (ptr2[0]))
                            ptr2 = strchr(ptr2 + 1, ',');

                    if (ptr2) {
                        ptr2++;

                        char *ptr3 = strchr(ptr2, ',');
                        char *ptr4 = strchr(ptr2, ')');

                        if (ptr4 < ptr3)
                            ptr3 = ptr4;

                        int start_pos = (long)ptr2 - (long)ptr;
                        int end_pos   = (long)ptr3 - (long)ptr;
                        if (end_pos > start_pos) {
#ifndef USE_SCINTILLA_CALLTIP
                            if ((start_pos > 0) && (end_pos > 0)) {
                                QString shint(sci->hint.c_str());
                                shint  = shint.insert(end_pos, "</font></b>");
                                shint  = shint.insert(start_pos, "<b><font color='green'>");
                                shint  = shint.replace("\n\n", "\n\n<font color='gray'>");
                                shint  = shint.replace("\n", "<br/>\n");
                                shint += "</font>";
                                sci->hintLabel->setText(shint);
                                sci->hintLabel->adjustSize();
                            }
#else
                            scintilla_send_message(sci, SCI_CALLTIPSETHLT, (uptr_t)start_pos, (sptr_t)end_pos);
#endif
                        }
                    }
                }
            } else
            if (scn->ch == ')') {
#ifndef USE_SCINTILLA_CALLTIP
                if ((sci->hintLabel) && (sci->hintLabel->isVisible()))
                    sci->hintLabel->hide();
#endif
                scintilla_send_message(sci, SCI_CALLTIPCANCEL, (uptr_t)0, (sptr_t)0);
            } else
            if (scn->ch == '\n') {
                AnsiString prefix = GetPrecLinePrefix(sci);
                //std::cout << "Prefix : "<< prefix.c_str();
                int pos = scintilla_send_message(sci, SCI_GETCURRENTPOS, (uptr_t)0, (sptr_t)0);
                scintilla_send_message(sci, SCI_INSERTTEXT, (uptr_t)pos, (sptr_t)prefix.c_str());
                scintilla_send_message(sci, SCI_GOTOPOS, (uptr_t)pos + prefix.Length(), (sptr_t)0);
            }     /* else
                   * if (scn->ch=='}') {
                   * long lStart = scintilla_send_message(sci, SCI_GETCURRENTPOS, (uptr_t)0, (sptr_t)0);
                   * long lEnd = scintilla_send_message(sci, SCI_BRACEMATCH, (uptr_t)lStart-1, (sptr_t)0);
                   * if (lEnd>=0) {
                   * AnsiString prefix=GetPrecLinePrefix(sci,lEnd);
                   * std::cout << "Prefix : [" << prefix.c_str() << "]\n";
                   * } else
                   * std::cout << "error : [" << GetSciText(sci)[(int)lStart-1] << "]";
                   * }*/
            else if (!scintilla_send_message(sci, SCI_AUTOCACTIVE, 0, 0)) {
                int        pos  = -1;
                AnsiString left = GetLeft(sci, pos);
                if ((sci->sugest_len == left.Length()) && (sci->sugest_list.Length())) {
                    AnsiString left2(" ");
                    left2 += left;
                    if (sci->sugest_list.Pos(left2) > 0) {
                        char *ptr = sci->sugest_list.c_str();
                        if ((ptr) && (ptr[0] == ' '))
                            ptr++;
                        scintilla_send_message(sci, SCI_AUTOCSHOW, (uptr_t)sci->sugest_len + 1, (sptr_t)ptr);
                    }
                }
            }
            last_rec_char = scn->ch;
            break;

        case SCN_UPDATEUI:
            {
                long lStart = scintilla_send_message(sci, SCI_GETCURRENTPOS, (uptr_t)0, (sptr_t)0);
                long lEnd   = scintilla_send_message(sci, SCI_BRACEMATCH, (uptr_t)lStart - 1, (sptr_t)0);
                // if there is a matching brace highlight it
                if (lEnd >= 0) {
                    if ((last_rec_char == '}') ||
                        (last_rec_char == ']') ||
                        (last_rec_char == ')')) {
                        AnsiString prefix = GetLinePrefix(sci, lEnd);
                        //std::cout << "Prefix : [" << prefix.c_str() << "]\n";
                        int delta = ReplaceLinePrefix(sci, lStart - 1, prefix.c_str());
                        //int pos = scintilla_send_message(sci, SCI_GETCURRENTPOS, (uptr_t)0, (sptr_t)0);
                        scintilla_send_message(sci, SCI_GOTOPOS, (uptr_t)/*pos+*/ lStart + delta, (sptr_t)0);
                        last_rec_char = 0;
                        lStart       += delta;
                    }
                    scintilla_send_message(sci, SCI_BRACEHIGHLIGHT, (uptr_t)lStart - 1, (sptr_t)lEnd);
                }
                // if there is NO matching brace erase old highlight
                else
                    scintilla_send_message(sci, SCI_BRACEHIGHLIGHT, (uptr_t)-1, (sptr_t)-1);
            }
            break;
    }
}

void scitilla_general_notify(ScintillaEditBase *widget, SCNotification *scn) {
    static char        last_rec_char = 0;
    DScintillaEditBase *sci          = (DScintillaEditBase *)widget;

    switch (scn->nmhdr.code) {
        case SCN_MARGINCLICK:
            {
                if (scn->margin == 2) {
                    long lLine = scintilla_send_message(sci, SCI_LINEFROMPOSITION, scn->position, 0);
                    scintilla_send_message(sci, SCI_TOGGLEFOLD, (uptr_t)lLine, (sptr_t)0);
                }
            }
            break;

        case SCN_CHARADDED:
            if (scn->ch == '\n') {
                AnsiString prefix = GetPrecLinePrefix(sci);
                //std::cout << "Prefix : "<< prefix.c_str();
                int pos = scintilla_send_message(sci, SCI_GETCURRENTPOS, (uptr_t)0, (sptr_t)0);
                scintilla_send_message(sci, SCI_INSERTTEXT, (uptr_t)pos, (sptr_t)prefix.c_str());
                scintilla_send_message(sci, SCI_GOTOPOS, (uptr_t)pos + prefix.Length(), (sptr_t)0);
            } else if (!scintilla_send_message(sci, SCI_AUTOCACTIVE, 0, 0)) {
                //_SCINTILLAS *internal = GetInternalSCI(widget);
                int        pos  = -1;
                AnsiString left = GetLeft(sci, pos);
                if ((sci->sugest_len == left.Length()) && (sci->sugest_list.Length())) {
                    AnsiString left2(" ");
                    left2 += left;
                    if (sci->sugest_list.Pos(left2) > 0) {
                        char *ptr = sci->sugest_list.c_str();
                        if ((ptr) && (ptr[0] == ' '))
                            ptr++;
                        scintilla_send_message(sci, SCI_AUTOCSHOW, (uptr_t)sci->sugest_len + 1, (sptr_t)ptr);
                    }
                }
            }
            last_rec_char = scn->ch;
            break;

        case SCN_UPDATEUI:
            {
                long lStart = scintilla_send_message(sci, SCI_GETCURRENTPOS, (uptr_t)0, (sptr_t)0);
                long lEnd   = scintilla_send_message(sci, SCI_BRACEMATCH, (uptr_t)lStart - 1, (sptr_t)0);
                // if there is a matching brace highlight it
                if (lEnd >= 0) {
                    if ((last_rec_char == '}') ||
                        (last_rec_char == ']') ||
                        (last_rec_char == ')')) {
                        AnsiString prefix = GetLinePrefix(sci, lEnd);
                        //std::cout << "Prefix : [" << prefix.c_str() << "]\n";
                        int delta = ReplaceLinePrefix(sci, lStart - 1, prefix.c_str());
                        //int pos = scintilla_send_message(sci, SCI_GETCURRENTPOS, (uptr_t)0, (sptr_t)0);
                        scintilla_send_message(sci, SCI_GOTOPOS, (uptr_t)/*pos+*/ lStart + delta, (sptr_t)0);
                        last_rec_char = 0;
                        lStart       += delta;
                    }
                    scintilla_send_message(sci, SCI_BRACEHIGHLIGHT, (uptr_t)lStart - 1, (sptr_t)lEnd);
                }
                // if there is NO matching brace erase old highlight
                else
                    scintilla_send_message(sci, SCI_BRACEHIGHLIGHT, (uptr_t)-1, (sptr_t)-1);
            }
            break;
    }
}

void BackgroundParse(AnsiString sci_text, void *context) {
    if (!sci_text.Length())
        return;
    clean_flag = 1;
    InitClassList(&sci_text, (ScintillaEditBase *)context);
    clean_flag = 0;
    PreParseAll((ScintillaEditBase *)context, (ScintillaEditBase *)context);
}

// Control invoker function
PREFIX int Invoke(void *context, int INVOKE_TYPE, ...) {
    va_list arg;

    switch (INVOKE_TYPE) {
        case INVOKE_QUERYTEST:
            {
                va_start(arg, INVOKE_TYPE);
                int cls_id = va_arg(arg, int);
                va_end(arg);

                if (cls_id == CUSTOM_CONTROL_ID)
                    return E_ACCEPTED;
            }
            break;

        case INVOKE_SETEVENT:
            {
                if (!RemoteInvoker)
                    break;
                va_start(arg, INVOKE_TYPE);
                int  cls_id      = va_arg(arg, int);
                int  event_id    = va_arg(arg, int);
                char *str_buffer = va_arg(arg, char *);
                int  buf_len     = va_arg(arg, int);
                va_end(arg);

                if ((!context) || (cls_id != CUSTOM_CONTROL_ID))
                    break;

                ScintilaParams *sp;
                RemoteInvoker(context, INVOKE_GETEXTRAPTR, &sp);
                if (!sp) {
                    sp = new ScintilaParams;
                    sp->event_request_file      = 0;
                    sp->event_update_class_list = 0;
                    RemoteInvoker(context, INVOKE_SETEXTRAPTR, (void *)sp);
                }

                switch (event_id) {
                    case EVENT_REQUEST_FILE:
                        sp->event_request_file = 1;
                        return E_ACCEPTED;

                    case EVENT_UPDATE_CLASS_LIST:
                        sp->event_update_class_list = 1;
                        return E_ACCEPTED;
                }
            }
            break;

        case INVOKE_SETPROPERTY:
            {
                va_start(arg, INVOKE_TYPE);
                int  cls_id      = va_arg(arg, int);
                int  prop_id     = va_arg(arg, int);
                char *str_buffer = va_arg(arg, char *);
                int  buf_len     = va_arg(arg, int);
                va_end(arg);

                if ((!context) || (cls_id != CUSTOM_CONTROL_ID))
                    break;
                AnsiString sci_text = str_buffer;

                switch (prop_id) {
                    case P_CAPTION:
                        scintilla_send_message((ScintillaEditBase *)context, SCI_SETTEXT, 0, (sptr_t)str_buffer);
                        session_id++;
                        BackgroundParse(sci_text, context);
                        //background_run = QtConcurrent::run(BackgroundParse, sci_text, context);
                        //InitClassList(&sci_text, (ScintillaEditBase *)context);
                        //PreParseAll((ScintillaEditBase *)context, (ScintillaEditBase *)context);
                        break;

                    default:
                        return E_NOTIMPLEMENTED;
                }

                return E_NOERROR;
            }
            break;

        case INVOKE_GETPROPERTY:
            {
                va_start(arg, INVOKE_TYPE);
                int  cls_id       = va_arg(arg, int);
                int  prop_id      = va_arg(arg, int);
                char **str_buffer = va_arg(arg, char **);
                int  *buf_len     = va_arg(arg, int *);
                va_end(arg);

                if ((!context) || (cls_id != CUSTOM_CONTROL_ID))
                    break;

                switch (prop_id) {
                    case P_CAPTION:
                        {
                            buffered_text = GetSciText((ScintillaEditBase *)context);
                            *str_buffer   = buffered_text.c_str();
                            *buf_len      = buffered_text.Length();
                        }
                        return E_ACCEPTED;

                    default:
                        return E_NOTIMPLEMENTED;
                }

                return E_NOERROR;
            }
            break;

        case INVOKE_SETSTREAM:
            // not needed
            break;

        case INVOKE_SETSECONDARYSTREAM:
            // not needed
            break;

        case INVOKE_CUSTOMMESSAGE:
            {
                va_start(arg, INVOKE_TYPE);
                int  msg     = va_arg(arg, int);
                int  cls_id  = va_arg(arg, int);
                char *target = va_arg(arg, char *);
                int  t_len   = va_arg(arg, int);
                char *value  = va_arg(arg, char *);
                int  v_len   = va_arg(arg, int);
                va_end(arg);

                if ((!context) || (cls_id != CUSTOM_CONTROL_ID))
                    break;

                DScintillaEditBase *sci = (DScintillaEditBase *)context;

                switch (msg) {
                    case MSG_CUSTOM_MESSAGE1:
                        if ((t_len == 0) && (v_len == 0))
                            IncludedList.Clear();
                        else
                            AddParse(target, value, (ScintillaEditBase *)context);
                        return E_ACCEPTED;

                    case MSG_CUSTOM_MESSAGE2: // int, int
                    case MSG_CUSTOM_MESSAGE6:
                    case MSG_CUSTOM_MESSAGE10:
                        {
                            // macro !
                            //---------------------------------
                            SCINTILLA_MESSAGE_GET(sci_message);
                            //---------------------------------
                            long lexer = AnsiString(target).ToInt();
                            if (sci_message == SCI_SETLEXER) {
                                if (lexer == _CUSTOM_MADE_SCLEX_CONCEPT) {
                                    if (!sci->events)
                                        sci->events = new ScintillaEventHandler(sci, scitilla_notify);
                                    lexer = 3;//SCLEX_CPP;
                                    QObject::connect(sci, SIGNAL(notify(SCNotification *)), sci->events, SLOT(on_notify(SCNotification *)));
                                    RegisterIcons(sci);
                                } else {
                                    if (!sci->events)
                                        sci->events = new ScintillaEventHandler(sci, scitilla_general_notify);
                                    QObject::connect(sci, SIGNAL(notify(SCNotification *)), sci->events, SLOT(on_notify(SCNotification *)));
                                }
                            }
                            sptr_t ret = scintilla_send_message(sci, sci_message, (uptr_t)lexer, (sptr_t)AnsiString(value).ToInt());
                            // macro !!
                            //---------------------------------
                            MESSAGES_WITH_RETURN(MSG_CUSTOM_MESSAGE6, MSG_CUSTOM_MESSAGE10)
                            //---------------------------------
                        }
                        return E_ACCEPTED;

                    case MSG_CUSTOM_MESSAGE3: // int, char*
                    case MSG_CUSTOM_MESSAGE7:
                    case MSG_CUSTOM_MESSAGE11:
                        {
                            // macro !
                            //---------------------------------
                            SCINTILLA_MESSAGE_GET(sci_message);
                            //---------------------------------
                            sptr_t ret = scintilla_send_message(sci, sci_message, (uptr_t)AnsiString(target).ToInt(), (sptr_t)AnsiString(value).c_str());
                            // macro !!
                            //---------------------------------
                            MESSAGES_WITH_RETURN(MSG_CUSTOM_MESSAGE7, MSG_CUSTOM_MESSAGE11)
                            //---------------------------------
                        }
                        return E_ACCEPTED;

                    case MSG_CUSTOM_MESSAGE4: // char *, int
                    case MSG_CUSTOM_MESSAGE8:
                    case MSG_CUSTOM_MESSAGE12:
                        {
                            // macro !
                            //---------------------------------
                            SCINTILLA_MESSAGE_GET(sci_message);
                            //---------------------------------
                            sptr_t ret = scintilla_send_message(sci, sci_message, (uptr_t)AnsiString(target).c_str(), (sptr_t)AnsiString(value).ToInt());
                            // macro !!
                            //---------------------------------
                            MESSAGES_WITH_RETURN(MSG_CUSTOM_MESSAGE8, MSG_CUSTOM_MESSAGE12)
                            //---------------------------------
                        }
                        return E_ACCEPTED;

                    case MSG_CUSTOM_MESSAGE5: // char *, char *
                    case MSG_CUSTOM_MESSAGE9:
                    case MSG_CUSTOM_MESSAGE13:
                        {
                            // macro !
                            //---------------------------------
                            SCINTILLA_MESSAGE_GET(sci_message);
                            //---------------------------------
                            sptr_t ret = scintilla_send_message(sci, sci_message, (uptr_t)AnsiString(target).c_str(), (sptr_t)AnsiString(value).c_str());
                            // macro !!
                            //---------------------------------
                            MESSAGES_WITH_RETURN(MSG_CUSTOM_MESSAGE9, MSG_CUSTOM_MESSAGE13)
                            //---------------------------------
                        }
                        return E_ACCEPTED;

                    case MSG_CUSTOM_MESSAGE14:
                        {
                            sci->sugest_list = AnsiString(value);
                            sci->sugest_len  = AnsiString(target).ToInt();
                        }
                        return E_ACCEPTED;
                }
            }
            break;

        case INVOKE_CREATE:
            {
                va_start(arg, INVOKE_TYPE);
                int  cls_id     = va_arg(arg, int);
                void **handler  = va_arg(arg, void **);
                int  *pack_type = va_arg(arg, int *);
                va_end(arg);
                *pack_type = (int)0;

                if (cls_id != CUSTOM_CONTROL_ID)
                    break;

                DScintillaEditBase *widget = new DScintillaEditBase((QWidget *)context);
                widget->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
                widget->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

                for (int i = 0; i < 50; i++) {
                    scintilla_send_message(widget, SCI_STYLESETFONT, (uptr_t)i, (sptr_t)"Lucida Console");
                    scintilla_send_message(widget, SCI_STYLESETSIZE, (uptr_t)i, (sptr_t)10);
                }

                *handler = widget;
                return E_ACCEPTED;
            }
            break;

        case INVOKE_DESTROY:
            {
                va_start(arg, INVOKE_TYPE);
                int  cls_id   = va_arg(arg, int);
                void *handler = va_arg(arg, void *);
                va_end(arg);

                if (cls_id != CUSTOM_CONTROL_ID)
                    break;

                ScintilaParams *sp;
                RemoteInvoker(context, INVOKE_GETEXTRAPTR, &sp);
                if (sp)
                    delete sp;

                return E_ACCEPTED;
            }
            break;

        case INVOKE_INITLIBRARY:
            va_start(arg, INVOKE_TYPE);
            RemoteInvoker = va_arg(arg, INVOKER);
            va_end(arg);
            return E_NOERROR;

        case INVOKE_DONELIBRARY:
            return E_NOERROR;

        default:
            return E_NOTIMPLEMENTED;
    }
    return E_NOTIMPLEMENTED;
}
