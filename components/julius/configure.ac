#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(julius.Concept, 1.0, eduard@radgs.ro)
AM_INIT_AUTOMAKE(julius.Concept, 1.0)
AC_CONFIG_SRCDIR([src/main.cpp])
AC_CONFIG_HEADER([config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

# Checks for libraries.
COMPONENT_CFLAGS=`libjulius-config --cflags`
COMPONENT_LIBS=`libjulius-config --libs`

LIBSENT_CFLAGS=`libsent-config --cflags`
LIBSENT_LIBS=`libsent-config --libs`

COMPONENT_CFLAGS="$COMPONENT_CFLAGS $LIBSENT_CFLAGS"
COMPONENT_LIBS="$COMPONENT_LIBS $LIBSENT_LIBS -lpthread"

AC_SUBST(COMPONENT_CFLAGS)
AC_SUBST(COMPONENT_LIBS)


# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST

# Checks for library functions.

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
