/*
   Copyright 2009, 2010 Free Software Foundation, Inc.


   This file is part of liblistserv.

   liblistserv is free software: you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

   liblistserv is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see
   `http://www.gnu.org/licenses/'.
 */
#ifndef LIBLISTSERV_H
#define LIBLISTSERV_H 1

#ifdef __cplusplus
extern "C"
{
#endif

#ifndef LIBLISTSERV_API
#  if defined BUILDING_LIBLISTSERV && defined HAVE_VISIBILITY && HAVE_VISIBILITY
#    define LIBLISTSERV_API __attribute__((__visibility__("default")))
#  elif defined BUILDING_LIBLISTSERV && defined _MSC_VER && ! defined LIBLISTSERV_STATIC
#    define LIBLISTSERV_API __declspec(dllexport)
#  elif defined _MSC_VER && ! defined LIBLISTSERV_STATIC
#    define LIBLISTSERV_API __declspec(dllimport)
#  else
#    define LIBLISTSERV_API
#  endif
#endif

enum listserv_content_filter_actions { REJECT = 'R', ALLOW = 'A',
				       MODERATE = 'M', DISCARD = 'D'};

  struct listserv_content_filter
  {
    char *header;		//could be Text, Header (for all headers) or a concrete header-filed
    char *value;		//the value of the header, NULL if header shall be empty/not presented
    char *text;			//parameter of REJECT or DISCARD, optionally ""
    enum listserv_content_filter_actions action;
  };

  struct listserv_subscriber
  {
    char *email;
    // xxxx xxx0 POST
    // xxxx xxx1 NOPOST
    // xxxx x00x NOMAIL
    // xxxx x01x MAIL
    // xxxx x10x DIGEST
    // xxxx x11x INDEX
    // xxxx 0xxx NOREPRO 
    // xxxx 1xxx REPRO
    // xxx0 xxxx
    // xxx1 xxxx
    // xx0x xxxx NOMIME
    // xx1x xxxx MIME
    // x0xx xxxx NOHTML
    // x1xx xxxx HTML
    // 0xxx xxxx NOCONCEAL
    // 1xxx xxxx CONCEAL
    char *date;//as returned by Listserv, currently DD MMM YYYY
    char *list;
    char **topics;
    int options;
  };

  struct listserv
  {
    char *email;
    char *password;
    char filesystem_hack;
    struct addrinfo *addrres;
    int port;
    struct listserv_subscriber **subscribers;
    char *mailtpl_default;
    char *mailtpl_site;
    char *cached_file_name;
    char **cached_files;
    struct listserv_content_filter **cached_content_filter;
    char *cached_command;
    char **cached_owned_lists;
    char **cached_char2;
    char ***keywords;
    char *keywords_initialized_for_list;
  };

  extern LIBLISTSERV_API struct listserv *listserv_init (const char *email,
							 const char *password,
							 const char *host);
  extern LIBLISTSERV_API char *listserv_command (struct listserv *,
						 const char *command);
  extern LIBLISTSERV_API char *listserv_getmail_template (struct listserv *,
							  const char
							  *listname,
							  const char
							  *template);
  extern LIBLISTSERV_API struct listserv_subscriber
    *listserv_getsubscriber (struct listserv *, const char *listname,
			     const char *email);
  extern LIBLISTSERV_API struct listserv_subscriber
    **listserv_getsubscribers (struct listserv *, const char *const listname);
  extern LIBLISTSERV_API struct listserv_subscriber
    **listserv_getsubscriptions (struct listserv *, const char *email);
  extern LIBLISTSERV_API char **listserv_getowned_lists (struct listserv *);
  extern LIBLISTSERV_API char* listserv_get (struct listserv *, const char*,
					     const char*);
  extern LIBLISTSERV_API char **listserv_list_filelist (struct listserv *,
							const char*);
  extern LIBLISTSERV_API char *listserv_putheader (struct listserv *,
						   const char *filename,
						   const char *data);
  extern LIBLISTSERV_API void listserv_destroy (struct listserv *);
  extern LIBLISTSERV_API void listserv_shutdown (struct listserv *);
  extern LIBLISTSERV_API struct listserv_content_filter
    **listserv_getcontent_filter (struct listserv *, const char *listname);
  extern LIBLISTSERV_API char **listserv_getlist_keyword (struct listserv *,
							  const char *,
							  const char *);
  extern LIBLISTSERV_API char** listserv_getemails_fromkeywords(struct
								listserv*,
								const char *,
								const char **);

/* Options, last parameter of listserv_sieve is ORed of
 *    0000 0000 0000 0001 |    1  extension envelope 
 *    0000 0000 0000 0010 |    2  extension reject
 *    0000 0000 0000 0100 |    4  extension ereject
 *    0000 0000 0000 1000 |    8  extension extlists
 *    0000 0000 0001 0000 |   16  extension variables 
 *    0000 0000 0010 0000 |   32  extension ihave
 *    0100 0000 0000 0000 | 4096  without size
 */
  extern LIBLISTSERV_API char **listserv_getsieve_scripts (struct listserv *,
							   const char *,
							   int unsigned
							   extensions);
#ifdef __cplusplus
}
#endif

#endif
