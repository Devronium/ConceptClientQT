# what flags you want to pass to the C compiler & linker
AM_CFLAGS = -O2 @COMPONENT_CFLAGS@
AM_CXXFLAGS = -O2 @COMPONENT_CFLAGS@
AM_LDFLAGS = -shared @COMPONENT_LIBS@

# this lists the binaries to produce, the (non-PHONY, binary) targets in
# the previous manual Makefile
libexecdir=@libexecdir@/ConceptControls
libexec_PROGRAMS = julius.cl
julius_cl_SOURCES = stdlibrary.cpp stdlibrary.h main.cpp AnsiString.cpp AnsiString.h semhh.h
