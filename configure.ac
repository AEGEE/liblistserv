#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.68])
AC_INIT([liblistserv],[1.0.4],[liblistserv@aegee.org])
AM_INIT_AUTOMAKE([dist-bzip2 -Wall subdir-objects])
AM_SILENT_RULES([yes])
AC_CONFIG_SRCDIR([src/liblistserv.c])
AC_CONFIG_MACRO_DIRS([m4])
AC_DEFINE([BUILDING_LIBLISTSERV], [1], [Used to hide symbols from liblistserv.h])
AC_USE_SYSTEM_EXTENSIONS
AM_PROG_AR
LT_INIT([disable-static])
dnl AC_REQUIRE(AC_CANONICAL_HOST)
AC_ARG_WITH(filesystem, AS_HELP_STRING([--with-filesystem=/home/listserv],
  [bypass Listserv for some actions and access directly the files from the filesystem.  Default is disabled. dir is where Listserv is installed]),
	[ if test "$withval" == "yes"; then withval="/home/listserv"; fi
	  AC_DEFINE_UNQUOTED([LISTSERV_PATH], "$withval", [Whether liblistserv shall prefer the filesystem over TCPGUI])])
# Checks for programs.
AC_PROG_CC_C99
gl_EARLY
AM_PROG_LEX
AC_PROG_CC
gl_VISIBILITY
AC_CHECK_HEADERS_ONCE(pwd.h, sys/types.h, net/errno.h)
AC_TYPE_SIZE_T
PKG_PROG_PKG_CONFIG([0.28])
AC_FUNC_MALLOC
AC_HEADER_STDC
AC_CONFIG_HEADERS([config.h])
gl_INIT
AC_CONFIG_FILES([Makefile liblistserv.pc lib/Makefile])
AC_OUTPUT
