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

#define LIBLISTSERV_NUM_KEYWORDS 60
#include "config.h"
#include "src/liblistserv.h"
#include <alloca.h>

#if HAVE_VISIBILITY
#define INTERNAL __attribute__ ((visibility ("internal")))
#else
#define INTERNAL
#endif

#if HAVE_ALLOCA
#define ALLOC_A(y)	alloca (y);
#define FREE_A(x)
#else
#define ALLOC_A(y)	malloc (y);
#define FREE_A(x)	free (x);
#endif

struct string
{
  char *str;
  unsigned int len, pos;
};

struct string *str_init () INTERNAL;
void str_concat (struct string *s, const char *const str) INTERNAL;
char *str_free (struct string *s) INTERNAL;
void listserv_free_cached_content_filter (struct listserv *const l) INTERNAL;
void listserv_free_cached_char2 (struct listserv *const l) INTERNAL;
void listserv_free_keywords (struct listserv *const l) INTERNAL;
void listserv_free_cached_subscribers (struct listserv *const l) INTERNAL;
void listserv_free_char2 (char** x) INTERNAL;
char** listserv_duplicate_char2(char**) INTERNAL;
