#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([Ad-hoc], [0.1.0])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])

AC_ARG_ENABLE([debug],
[AS_HELP_STRING([--enable-debug],[Turn on debugging])],
[CFLAGS='-ggdb'; AC_DEFINE([AH_DEBUG])])

# Checks for programs.
AC_PROG_YACC
AC_PROG_CC
AC_PROG_LEX

# Checks for libraries.
# FIXME: Replace `main' with a function in `-lfl':
AC_CHECK_LIB([fl], [main])
# FIXME: Replace `main' with a function in `-lm':
AC_CHECK_LIB([m], [main])
# FIXME: Replace `main' with a function in `-lreadline':
AC_CHECK_LIB([readline], [main])

# Checks for header files.
AC_CHECK_HEADERS([locale.h stddef.h stdlib.h string.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL
AC_C_INLINE
AC_TYPE_SIZE_T

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([pow setlocale strdup strndup])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
