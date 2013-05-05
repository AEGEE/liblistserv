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

#include "src/util.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


inline void
listserv_free_char2(char **x)
{
  int i = 0;
  while (x[i]) {
    free(x[i]);
    i++;
  }
  free(x);
}

inline char**
listserv_duplicate_char2(char** x)
{
  int i = 0;
  while (x[i]) i++;
  char **ret = malloc(sizeof(char*) * (i+1));
  i = 0;
  while (x[i]) {
    ret[i] = strdup(x[i]);
    i++;
  }
  ret[i] = NULL;
  return ret;
}

inline struct string *
str_init ()
{
  struct string *s = malloc (sizeof (struct string));
  s->len = 1023;
  s->str = malloc (s->len);
  s->str[0] = '\0';
  s->pos = 0;
  return s;
}

inline void
str_concat (struct string *s, const char *str)
{
  unsigned int len = strlen (str);
  if (len + s->pos + 1 > s->len)
    {
      unsigned int factor = (1 + len / 1024) * 1024;
      s->str = realloc (s->str, s->len + factor);
      s->len += factor;
    }
  strcpy (s->str + s->pos, str);
  s->pos += len;
}

inline char *
str_free (struct string *s)
{
  char *x = s->str;
  free (s);
  s = NULL;
  return x;
}

inline void
listserv_free_cached_char2 (struct listserv *const l)
{
  if (l->cached_char2) {
    listserv_free_char2(l->cached_char2);
    l->cached_char2 = NULL;
  }
}


inline void
listserv_free_cached_content_filter (struct listserv *const l)
{
  if (l->cached_content_filter)
    {
      int i = 0;
      while (l->cached_content_filter[i])
	{
	  free (l->cached_content_filter[i]->header);
	  if (l->cached_content_filter[i]->value)
	    free (l->cached_content_filter[i]->value);
	  if (l->cached_content_filter[i]->text)
	    free (l->cached_content_filter[i]->text);
	  free (l->cached_content_filter[i++]);
	}
      free (l->cached_content_filter);
    }
}

inline void
listserv_free_cached_subscribers (struct listserv *const l)
{
  if (l->subscribers)
    {
      unsigned int i = 0, j;
      while (l->subscribers[i])
	{
	  free (l->subscribers[i]->email);
	  if (l->subscribers[i]->date)
	    free (l->subscribers[i]->date);
	  free (l->subscribers[i]->list);
	  if (l->subscribers[i]->topics)
	    {
	      j = 0;
	      while (l->subscribers[i]->topics[j])
		free (l->subscribers[i]->topics[j++]);
	    }
	  free (l->subscribers[i]);
	  i++;
	}
      free (l->subscribers);
      l->subscribers = NULL;
    }
}

inline void
listserv_free_keywords (struct listserv *const l)
{
  if (l->keywords_initialized_for_list)
    {
      int i;
      for (i = 0; i < LIBLISTSERV_NUM_KEYWORDS; i++)
	if (l->keywords[i]) {
	  listserv_free_char2(l->keywords[i]);
	  l->keywords[i] = NULL;
	}
      free (l->keywords_initialized_for_list);
    }
}
