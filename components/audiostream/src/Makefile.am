# what flags you want to pass to the C compiler & linker
AM_CFLAGS = -O2 @COMPONENT_CFLAGS@ @SPEEX_CFLAGS@ @OPUS_CFLAGS@
AM_CXXFLAGS = -O2 @COMPONENT_CFLAGS@ @SPEEX_CFLAGS@ @OPUS_CFLAGS@
AM_LDFLAGS = -shared @COMPONENT_LIBS@ @SPEEX_LIBS@ @OPUS_LIBS@

# this lists the binaries to produce, the (non-PHONY, binary) targets in
# the previous manual Makefile
libexecdir=@libexecdir@/ConceptControls
libexec_PROGRAMS = audiostream.cl
audiostream_cl_SOURCES = stdlibrary.cpp stdlibrary.h main.cpp AnsiString.cpp AnsiString.h semhh.h AES/aes.cpp AES/aes.h
