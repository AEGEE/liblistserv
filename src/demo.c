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

#include "src/liblistserv.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>


int
main ()
{
  struct listserv *l = listserv_init (NULL /*"1@aegee.org"*/, NULL, NULL);
  char **lists = listserv_getowned_lists(l);
  char* ret = listserv_command(l, "FREE NETCOM-REBEKKA-L");
  printf ("%s\n", ret);
  return 0;
  int i = 0;
  while (lists[i]) {
    struct listserv_subscriber **subscribers = listserv_getsubscribers (l, lists[i++]);
    int j = 0;
    while (subscribers[j]) {
      if (strstr(subscribers[j]->email, "@AEGEE-ANKARA.ORG"))
	  printf ("%s:%s\n", lists[i-1], subscribers[j]->email);
      j++;
    }
  }
  printf("\n");
  listserv_shutdown (l);
  return 0;
}
