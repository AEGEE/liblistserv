AM_LFLAGS	= -s -olex.yy.c
AM_CPPFLAGS	= -DSYSCONFDIR='"${sysconfdir}"' -I${top_builddir}/lib -I${top_srcdir}/lib
AM_CFLAGS	= -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable ${CFLAG_VISIBILITY}

EXTRA_DIST			= sieve.mailtpl m4/gnulib-cache.m4
pkgconfigdir			= $(libdir)/pkgconfig
pkgconfig_DATA			= liblistserv.pc
noinst_PROGRAMS			= src/demo
SUBDIRS				= lib

info_TEXINFOS			= doc/liblistserv.texi
doc_liblistserv_TEXINFOS	= doc/fdl-1.3.texi

lib_LTLIBRARIES			= src/liblistserv.la
include_HEADERS			= src/liblistserv.h

src_liblistserv_la_LIBADD	= lib/libgnu.la
src_liblistserv_la_LDFLAGS	= -no-undefined -version-info 0:4 $(GETADDRINFO_LIB) $(HOSTENT_LIB) $(INET_NTOP_LIB) $(LIBSOCKET) $(LTLIBINTL) $(SERVENT_LIB) $(LTLIBINTL)
src_liblistserv_la_SOURCES	= src/content_filter.l src/lcmdx.c src/liblistserv.c src/list_header.l src/sieve_scripts.c src/stringlist.c src/stringlist.h src/util.h src/util.c

src_demo_LDADD			= src/liblistserv.la

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status -recheck
