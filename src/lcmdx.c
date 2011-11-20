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


/******************************************************************************
 *                                                                            *
 * LISTSERV V2 - send command to LISTSERV on remote node via TCPGUI interface *
 *                                                                            *
 *       Copyright L-Soft international 1996-97 - All rights reserved         *
 *                                                                            *
 * Syntax:                                                                    *
 *                                                                            *
 *  lcmdx hostname[:port] address password command                            *
 *                                                                            *
 * Connects to 'hostname' on port 'port' (default=2306) using the LISTSERV    *
 * TCPGUI protocol, then executes the LISTSERV command 'command' from the     *
 * origin 'address'. 'password' is the personal LISTSERV password associated  *
 * with the command origin ('address') - see the description of the PW ADD    *
 * command for more information on LISTSERV passwords. The reply from         *
 * LISTSERV is echoed to standard output (the command is executed             *
 * synchronously).                                                            *
 *                                                                            *
 ******************************************************************************/
#include "src/util.h"
#include <ctype.h>
#include <errno.h>
#ifdef INTUX
#include <net/errno.h>
#endif
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#ifdef ultrix
	/* Use read() rather than recv() to bypass a bug in Ultrix */
#define recv(a, b, c, d) read (a, b, c)
#endif

static inline int
receive (int ss, char *buf, int len)
{
  char *w, *e;
  int l;

  for (w = buf, e = buf + len; w < e;)
    {
      l = recv (ss, w, e - w, 0);
      if (l <= 0)
	return (l);
      w += l;
    }
  return len;
}

char *
listserv_command (struct listserv *const l, const char *const command)
{
  char buf[256], *reply = 0, *cmd, *w, *r, *e;
  unsigned char *wb;
  int rc, ss = 0, len, orglen, n;
  unsigned int ibuf[2];
  char *ret = NULL;

  /* Initialize */
  len = strlen(command) + strlen(l->password) + 4;
  cmd = ALLOC_A (len + 1);
  sprintf (cmd, "%s PW=%s", command, l->password);
  orglen = strlen (l->email);

  /* DNS resolution and socket connect */
  struct addrinfo *pointer = l->addrres;
  while (pointer)
    {
      if ((ss = socket (pointer->ai_family, SOCK_STREAM, 0)) >= 0)
	{
	  switch (pointer->ai_family)
	    {
	    case AF_INET:
	      ((struct sockaddr_in *) pointer->ai_addr)->sin_port =
		htons (l->port);
	      break;
	    case AF_INET6:
	      ((struct sockaddr_in6 *) pointer->ai_addr)->sin6_port =
		htons (l->port);
	    }
	  if (0 == connect (ss, (struct sockaddr *) (pointer->ai_addr),
			    pointer->ai_addrlen))
	    break;
	}
      pointer = pointer->ai_next;
    }
  if (pointer == NULL)
    goto Socket_Error;

  /* Send the protocol level request and the command header */
  wb = (unsigned char *) buf;
  n = len + orglen + 1;		/* Byte length                  */
  *wb++ = '1';			/* Protocol level: 1            */
  *wb++ = 'B';			/* Mode: binary                 */
  *wb++ = '\r';
  *wb++ = '\n';
  *wb++ = n / 256;		/* Request length byte 1        */
  *wb++ = n & 255;		/* Request length byte 2        */
  *wb++ = orglen;		/* Origin/Email length: 1               */
  for (r = l->email; *r;)
    *wb++ = (unsigned char) *r++;
  if (send (ss, buf, (char *) wb - buf, 0) < 0)
    goto Socket_Error;

  /* Await confirmation */
  for (w = buf;;)
    {
      n = recv (ss, w, buf + sizeof (buf) - w, 0);
      if (n <= 0)
	goto Socket_Error;
      w += n;
      for (r = buf; r < w && *r != '\n'; r++);
      if (r != w)
	break;
    }
  /* Anything other than 250 is an error */
  if (buf[0] != '2' || buf[1] != '5' || buf[2] != '0')
    goto Protocol_Error;

  /* Finish sending the command text */
  if (send (ss, cmd, len, 0) < 0)
    goto Socket_Error;

  /* Read the return code and reply length */
  if (receive (ss, (char *) ibuf, 8) <= 0)
    goto Socket_Error;
  /* Exit if the return code is not 0 */
  if (ntohl (ibuf[0]))
    goto Protocol_Error;
  /* Read the reply */
  len = ntohl (ibuf[1]);
  reply = ALLOC_A (len + 1);
  if (receive (ss, (char *) reply, len) <= 0)
    goto Socket_Error;
  /* Count the lines */
  int lines = 0;
  r = reply;
  int z;
  for (z = 0; z < len; z++)
    if (r[z] == '\r')
      lines++;
  ret = malloc (len + lines + 1);
  ret[0] = '\0';
  char *x = ret;
  /* Cut it into individual lines, and output it */
  for (r = reply, e = reply + len; r < e;)
    {
      for (w = r; w < e && *w != '\r'; w++);
      *w++ = '\0';
      x = stpcpy (x, r);
      x[0] = '\n';
      x++;
      r = w;
      if (r < e && *r == '\n')
	r++;
    }
  x--;
  x[0] = '\0';
  /* Close the socket and return */
  rc = 0;
  goto Done;

Protocol_Error:
  fprintf (stderr, "\
>>> Protocol error while communicating with LISTSERV.");
  rc = 1000;
  goto Done;

Socket_Error:
  fprintf (stderr, "\
>>> Error - unable to initiate communication with LISTSERV (errno=%d).\n", rc = errno);
  goto Done;

Done:
  FREE_A (cmd);
  if (reply)
    {
      FREE_A (reply);
    }
  if (ss >= 0)
    {
      shutdown (ss, 2);
      close (ss);
    }
  //      return(rc);
  if (l->cached_command)
    free (l->cached_command);
  if (rc)
    {
      free (ret);
      l->cached_command = malloc (15);
      sprintf (l->cached_command, "0%i", rc);
    }
  else
    l->cached_command = ret;
  return l->cached_command;
}
