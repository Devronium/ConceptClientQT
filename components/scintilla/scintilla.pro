CONFIG += warn_off
QT += widgets
INCLUDEPATH += /usr/local/include
LIBPATH += /usr/local/lib
LIBPATH += /usr/lib
TARGET = scintilla3
TEMPLATE = lib
CONFIG += lib_bundle

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/scintilla/bin/release/ -lScintillaEditBase
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/scintilla/bin/debug/ -lScintillaEditBase
else:mac: LIBS += -F$$PWD/scintilla/bin/ -framework ScintillaEditBase
else:unix: LIBS += -L$$PWD/scintilla/bin/ -lScintillaEditBase

INCLUDEPATH += $$PWD/scintilla/include
INCLUDEPATH += $$PWD/scintilla/Qt/ScintillaEditBase
DEPENDPATH += $$PWD/scintilla/bin

HEADERS += \
    AnsiList.h \
    AnsiParser.h \
    AnsiString.h \
    Codes.h \
    ConceptTypes.h \
    quickparser.h \
    ScintillaEventHandler.h \
    stdlibrary.h

SOURCES += \
    AnsiList.cpp \
    AnsiParser.cpp \
    AnsiString.cpp \
    Codes.cpp \
    main.cpp \
    quickparser.cpp \
    stdlibrary.cpp

