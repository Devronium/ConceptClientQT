#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(audiostream.Concept, 2.0, eduard@radgs.ro)
AM_INIT_AUTOMAKE(audiostream.Concept, 2.0)
AC_CONFIG_SRCDIR([src/main.cpp])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

# Checks for libraries.
pkg_modules="portaudio-2.0 >= 19"
PKG_CHECK_MODULES(COMPONENT, [$pkg_modules])

pkg_modules3="speex >= 1.1"
PKG_CHECK_MODULES(SPEEX, [$pkg_modules3])

pkg_modules3="opus >= 1.0"
PKG_CHECK_MODULES(OPUS, [$pkg_modules3])

AC_SUBST(COMPONENT_CFLAGS)
AC_SUBST(COMPONENT_LIBS)

AC_SUBST(SPEEX_CFLAGS)
AC_SUBST(SPEEX_LIBS)

AC_SUBST(OPUS_CFLAGS)
AC_SUBST(OPUS_LIBS)


# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

# Checks for library functions.

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
