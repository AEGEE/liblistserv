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
#include <sys/stat.h>
#include <ctype.h>
#include <netdb.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


struct listserv *
listserv_init (const char *const email, const char *const password,
	       const char *const host)
{
  //if password == NULL, read config file -- ~/.liblistserv, /etc/liblistserv.conf
  struct listserv *ret = malloc (sizeof (struct listserv));
  if (ret == NULL) return NULL;
  memset (ret, 0, sizeof (struct listserv));
  ret->filesystem_hack = 1;
  char *_host = strdup (host ? strdup(host) : "localhost");
  if (_host == NULL) {
    free (ret);
    return NULL;
  }
  if (password)
    {
      ret->password = strdup (password);
      ret->email = strdup (email ? email : "@");
    }
  else
    {
      struct passwd *pw = getpwuid (getuid ());
      char *filename = ALLOC_A (strlen (pw->pw_dir) + 14);
      sprintf (filename, "%s/.liblistserv", pw->pw_dir);
      FILE *f = fopen (filename, "r");
      if (f == NULL)
	{
	  char *filename2 = ALLOC_A (strlen (SYSCONFDIR) + 18);
	  sprintf (filename2, "%s/liblistserv.conf", SYSCONFDIR);
	  f = fopen (filename2, "r");
	  if (f == NULL)
	    {
	      fprintf (stderr,
		       "No configuration file for liblistserv (%s or %s) found.  You cannot use 'NULL' as second paramenter of listserv_init in this case.\nPlease create one of the above files containing on the first line: the email address, the password, the host and the port to connect.  Host and port are separated by semicolon(:). The port is optional and defaults to 2306.  The host is optional and defaults to localhost:2306.\n",
		       filename, filename2);
	      free (_host);
	      FREE_A (filename2);
	      FREE_A (filename);
	      free (ret);
	      return NULL;
	    }
	  FREE_A (filename);
	}
      fscanf (f, "%ms", &ret->email);
      fscanf (f, "%ms", &ret->password);
      char *x;
      fscanf (f, "%ms", &x);
      if (x)
	{
	  free (_host);
	  _host = x;
	}
      fclose (f);
    }
  ret->keywords = malloc ((LIBLISTSERV_NUM_KEYWORDS+1) * sizeof (char **));
  memset (ret->keywords, 0, LIBLISTSERV_NUM_KEYWORDS * sizeof (char **));
  ret->keywords[LIBLISTSERV_NUM_KEYWORDS] = malloc (2 * sizeof(char*));
  ret->keywords[LIBLISTSERV_NUM_KEYWORDS][0] = strdup ("Unknown keyword");
  ret->keywords[LIBLISTSERV_NUM_KEYWORDS][1] = NULL;
  ret->keywords_initialized_for_list = NULL;
  ret->cached_command = NULL;
  ret->cached_owned_lists = NULL;
  char *a = strchr (_host, ':');
  if (a != NULL)
    ret->port = atoi (a + 1);
  else
    ret->port = 2306;
  //  ret->addrres->ai_socktype = SOCK_STREAM;
  getaddrinfo (_host, NULL, NULL, &(ret->addrres));
  free (_host);
  return ret;
}

void listserv_destroy(struct listserv *l)
{
  listserv_shutdown (l);
}

void
listserv_shutdown (struct listserv *l)
{
  free (l->cached_command);
  if (l->cached_file_name ) {
    free(l->cached_file_name);
    int i = 0;
    while (l->cached_files[i])
      free(l->cached_files[i++]);
    free(l->cached_files);
  }
  free (l->email);
  listserv_free_cached_subscribers (l);
  listserv_free_cached_content_filter (l);
  listserv_free_cached_char2 (l);
  listserv_free_keywords (l);
  free (l->keywords[LIBLISTSERV_NUM_KEYWORDS][0]);
  free (l->keywords[LIBLISTSERV_NUM_KEYWORDS]);
  free (l->keywords);
  if (l->cached_owned_lists) {
    int i = 0;
    while (l->cached_owned_lists[i])
      free(l->cached_owned_lists[i++]);
    free(l->cached_owned_lists);
  }
  if (l->password)
    free (l->password);
  if (l->mailtpl_default)
    free (l->mailtpl_default);
  if (l->mailtpl_site)
    free (l->mailtpl_site);
  freeaddrinfo (l->addrres);
  free (l);
}

char *
listserv_putheader (struct listserv *const l,
		    const char *const filename, const char *const data)
{
  //free cache - files
  if (l->cached_files)
    if (strcasecmp (filename, l->cached_file_name)) {
      int i = 0;
      while (l->cached_files[i])
	free (l->cached_files[i++]);
      free (l->cached_files);
      free (l->cached_file_name);
    }
  //free cache - keywords
  if (l->keywords_initialized_for_list == NULL
      || strcasecmp (l->keywords_initialized_for_list, filename) != 0) {
    listserv_free_keywords (l);
    l->keywords_initialized_for_list = NULL;
  }
  struct string *str = str_init ();
  str_concat (str, "X-STL ");
  str_concat (str, filename);
  str_concat (str, " ");
  const char *start_of_line = data;
  int i = 1;
  char *next_CR, *next_LF;
  char line_length_str[100];
  int line_length;
  while (i)
    {
      next_CR = strchr (start_of_line, '\r');
      next_LF = strchr (start_of_line, '\n');
      if (next_CR == NULL && next_LF == NULL)
	{
	  i = 0;
	  line_length = strlen (start_of_line);
	}
      else if ((next_LF != NULL && next_CR >= next_LF) || next_CR == NULL)
	line_length = next_LF - start_of_line;
      else
	line_length = next_CR - start_of_line;
      sprintf (line_length_str, "%i_", line_length);
      str_concat (str, line_length_str);
      if (line_length + str->pos + 1 > str->len)
	{
	  unsigned int factor = (1 + line_length / 1024) * 1024;
	  str->str = realloc (str->str, str->len + factor);
	  str->len += factor;
	}
      strncpy (str->str + str->pos, start_of_line, line_length);
      str->pos += line_length;

      start_of_line += line_length + 1;
      if (start_of_line[0] == '\r' || start_of_line[0] == '\n')
	start_of_line++;
      //    if (start_of_line[0] == '\0') break;
    }
  str->str[str->pos] = '\0';
  char *string = str_free (str);
  char *ret = listserv_command (l, string);
  free (string);

  return ret;
}

static inline char *
listserv_parse_template (char* template)
 
{
  return template;
  if (template[0] == '.' 
      && template[1] == 'C'
      && template[2] == 'S') {
    char *t = strdup(strchr(template, '\n') + 1);
    free(template);
    return t;
  } else
    return template;
}

static char* load_file (const char *filename)
{
  char *path = malloc(strlen(filename)+21);
  sprintf(path, "/home/listserv/%s", filename);
  FILE* f = fopen(path, "r");
  free(path);
  if (!f) return NULL;
  struct stat sb;
  int filenum = fileno (f);
  fstat (filenum, &sb);
  char *ret = malloc(sb.st_size + 1);
  read (filenum, ret, sb.st_size);
  ret[sb.st_size] = '\0';
  fclose (f);
  return ret;
}

char *
listserv_getmail_template (struct listserv *const l,
			   const char *const listname,
			   const char *const template)
{
  char *mailtpl = listserv_get (l, listname, "MAILTPL");
  char *query = NULL;//Put in query the mailtpl file with the searched template.
  char *searched_string = ALLOC_A (5 + strlen (template));
  sprintf (searched_string, ">>> %s", template);
  if ((query = (char *) strcasestr (mailtpl, searched_string)) == 0)
    {
      if (query == NULL)
	{
	  if (l->mailtpl_site == NULL)
	    {
	      if (l->filesystem_hack == 1) {
		l->mailtpl_site = load_file("home/site.mailtpl");
	      }
	      if (l->mailtpl_site == NULL)
		l->mailtpl_site = listserv_command (l, "GET site.mailtpl (MSG");
	      l->cached_command = NULL;
	    }
	  if ((query = (char *) strcasestr (l->mailtpl_site,
					    searched_string)) == 0)
	    {
	      if (l->mailtpl_default == NULL)
		{
		  if (l->filesystem_hack == 1) {
		    l->mailtpl_default = load_file("home/default.mailtpl");
		  }
		  if (l->mailtpl_default == NULL)
		    l->mailtpl_default =
		      listserv_command (l, "GET default.mailtpl (MSG");
		  l->cached_command = NULL;
		}
	      if ((query = (char *) strcasestr (l->mailtpl_default,
						searched_string)) == 0)
		{
		  FREE_A (searched_string);
		  l->cached_command = strdup("");
		  return l->cached_command;
		}
	    }
	}
    }
  //query points now to the correct >>> - line
  while (query[0] != '\r' && query[0] != '\n')
    query++;
  if (query[0] < 14)
    query++;
  //query points to the line after >>>
  FREE_A (searched_string);
  searched_string = strstr (query, ">>> ");
  char *ret;
  if (searched_string == NULL)
    ret = strdup (query);
  else
    {
      ret = malloc (searched_string - query);
      memcpy (ret, query, searched_string - query - 1);
      ret[searched_string - query - 1] = '\0';
    };
  free (l->cached_command);
  l->cached_command = listserv_parse_template (ret);
  //  printf("LIST %s TEMPLATE %s=%s=\n", listname, template, ret);
  return strstr(ret, ".CS UTF-8\n") == ret ? ret + 10 : ret;
}

#define option_number(x)   (((tolower(x[0]) - 65) << 10) + ((tolower(x[2]) - 65) << 5) + (tolower(x[3]) - 65))

#define pri(x)  printf ("case %i: //%s\n", option_number(x), x);
/*
static inline void listserv_setoptions(char *x, int *j) {
  switch (option_number(x)) {
  case 39307: *j &&= 0xFFFFF8FF; break; //FULL
  case 42629: *j ||= 0x00000; break; //IETF
  case 52721: *j &&= 0xFFFFFAFF; break; //SHORT
  case 36907: *j &&= 0xFFFFFBFF; break; //DUAL
  case 52297: *j &&= 0xFFFFFCFF; break; //SUBJ
  case 46304: *j &&= 0xFFFFreturn 20; break; //MSGa
  case 47138: return 21; break; //NOACK
  case 34047: return 22; break; //ACK
  }
 }
*/

static inline struct listserv_subscriber *
listserv_parse_query_gui (char **string)
{
  /*
 ***HDR*** headoffice@AEGEE.ORG
 ***NAME*** AEGEE Europe
 ***LIST*** CD-L

 ***OPT*** NOMAIL
 ***OPT*** SUBJECTHDR
 ***OPT*** REPRO
 ***OPT*** NOACK
 ***HDR*** manos.valasis@AEGEE.ORG
 ***NAME*** Manos Valasis
 ***LIST*** CD-L

 ***OPT*** MAIL
 ***OPT*** SUBJECTHDR
 ***OPT*** REPRO
 ***OPT*** NOACK

 ***SUBDATE*** 27 Sep 2010
 ***TOPICS*** topic
 ***TOPLIST*** toplist
   */
  /*  pri("POST");
     pri("NOPOST");
     pri("REPRO");
     pri("NORPRO"); */
  if (*string[0] != '*') //(*string[0] != 'T' || *string[0] == 'Y' )
    return NULL;		//There is no subscription for 
  char *start = *string + 10;
  struct listserv_subscriber *l =
    malloc (sizeof (struct listserv_subscriber));
  char *eol = strchr (start, '\n');
  *eol='\0';
  l->email = strdup (start);
  //  printf("email:'%s'\n", l->email);
  start = eol+12;// '\0***NAME*** '
  eol = strchr (start, '\n');
  *eol = '\0';
  //  char *name = strdup (start);
  start = eol + 12;// '\0\n***LIST*** '
  eol = strchr (start, '\n');
  *eol = '\0';
  l->list = strdup (start);
  start = eol + 2;// '\0\n'
  //  free (name);
  l->options = 0;
  l->topics = NULL;
  while ((start[3] == 'O'))
    {
      start += 10;//'***OPT*** '
      eol = strchr (start, '\n');
      *eol = '\0';
      //printf("option %s: %i\n", option, option_number(option));
      //      printf("%s:%s:%s\n", l->email, l->list, option);
      switch (option_number (start))
	{
	case 49779:
	  l->options = l->options & 0xFE;
	  break;		//NOPOST, bit 0
	case 47520:
	  l->options = l->options & 0xF9;
	  break;		//NOMAIL
	case 46379:
	  l->options = (l->options | 0x02) & 0xFB;
	  break;		//MAIL
	case 37092:
	  l->options = (l->options | 0x04) & 0xFD;
	  break;		//DIGEST
	case 42116:
	  l->options = l->options | 0x06;
	  break;		//INDEX
	case 47695:
	  l->options = l->options & 0xF7;
	  break;		//NOREPRO
	case 51729:
	  l->options = l->options | 0x08;
	  break;		//REPRO
	case 47528:
	  l->options = l->options & 0xDF;
	  break;		//NOMIME
	case 46500:
	  l->options = l->options | 0x20;
	  break;		//MIME
	case 47379:
	  l->options = l->options & 0xBF;
	  break;		//NOHTML
	case 47372:
	  l->options = l->options | 0x40;
	  break;		//HTML
	case 47214:
	  l->options = l->options & 0x7F;
	  break;		//NOCONCEAL
	case 36290:
	  l->options = l->options | 0x80;
	  break;		//CONCEAL
	default:;
	}
      start = eol + 1;
    }
  if (start[4] == 'S')
    {
      start += 15;
      char *end = strchr (start, '\n');
      if (end)
	*end = '\0';
      l->date = strdup (start);
      if (end)
	{
	  end++;
	  *string = end;
	}
      else
	*string = NULL;
    }
  else
    {
      l->date = NULL;
      *string = start;
    }
  if (start[4] == 'T') { //here follow possible ***TOPICS***
      char *end = strchr (start, '\n');
      if (end) {
	*string = end+1;
      }
  }
  if (start[4] == 'T') { //here follow possible ***TOPLIST***
      char *end = strchr (start, '\n');
      if (end) {
	*string = end+1;
      }
  }

  return l;
}

struct listserv_subscriber *
listserv_getsubscriber (struct listserv *const l,
			const char *const listname, const char *const email)
{
  listserv_free_cached_subscribers (l);
  char *query = ALLOC_A (strlen (listname) + strlen (email) + 22);
  sprintf (query, "QUERY ***GUI*** %s FOR %s", listname, email);
  char *cmd = listserv_command (l, query);
  FREE_A (query);
  //if cmd starts with "You are not" or "There is no " then return -1
  struct listserv_subscriber *s = listserv_parse_query_gui (&cmd);
  if (l)
    {
      l->subscribers = malloc (2 * sizeof (struct listserv_subscriber *));
      l->subscribers[0] = s;
      l->subscribers[1] = NULL;
      return l->subscribers[0];
    }
  else				//  if (cmd[0] == 'Y' || cmd[0] == 'T') {
    return NULL;
}

struct listserv_subscriber **
listserv_getsubscriptions (struct listserv *const l, const char *const email)
{
  listserv_free_cached_subscribers (l);
  char *query = ALLOC_A (strlen (email) + 23);
  sprintf (query, "QUERY ***GUI*** * FOR %s", email);
  char *subscribers = listserv_command (l, query);
  FREE_A (query);
  query = subscribers;
  int amount = 0;
  l->subscribers = NULL;
  struct listserv_subscriber *s;
  while ((s = listserv_parse_query_gui (&subscribers)))
    {
      if (amount % 1024 == 0)
	l->subscribers =
	  realloc (l->subscribers,
		   (amount + 1024) * sizeof (struct listserv_subsriber *));
      l->subscribers[amount++] = s;
    }
  if (amount % 1024 == 0)
    l->subscribers =
      realloc (l->subscribers,
	       (amount + 1) * sizeof (struct listserv_subsriber *));
  l->subscribers[amount] = NULL;
  return l->subscribers;
}

struct listserv_subscriber **
listserv_getsubscribers (struct listserv *const l, const char *const listname)
{
  listserv_free_cached_subscribers (l);
  char *query = ALLOC_A (23 + strlen (listname));
  sprintf (query, "QUERY ***GUI*** %s FOR *", listname);
  char *subscribers = listserv_command (l, query);
  FREE_A (query);
  query = subscribers;
  l->subscribers = NULL;
  int amount = 0;
  //  printf("query = %s\n", query);
  struct listserv_subscriber *s;
  while ((s = listserv_parse_query_gui (&subscribers)))
    {
      if (amount % 1024 == 0)
	l->subscribers =
	  realloc (l->subscribers,
		   (amount + 1024) * sizeof (struct listserv_subsriber *));
      l->subscribers[amount++] = s;
    }
  if (amount % 1024 == 0)
    l->subscribers = realloc (l->subscribers,
			      (amount + 1) 
			      * sizeof (struct listserv_subsriber *));
  l->subscribers[amount] = NULL;
  return l->subscribers;
}

char **
listserv_getowned_lists (struct listserv *const l)
{
  if (l->cached_owned_lists)
    return l->cached_owned_lists;
  char *lists_owned = listserv_command (l, "LISTS OWNED");
  char *lists = lists_owned;
  int num_lists = 0;
  do
    {
      if (!lists[1])
	break;
      lists++;
      num_lists++;
    }
  while ((lists = strchr (lists, '\n')));
  l->cached_owned_lists = malloc (num_lists * sizeof (char *));
  lists = lists_owned;
  num_lists = 0;
  do
    {
      if (!lists[1])
	break;
      sscanf (lists, "%ms", &lists_owned);
      l->cached_owned_lists[num_lists++] = lists_owned;
      lists++;
    }
  while ((lists = strchr (lists, '\n')));
  free (l->cached_owned_lists[num_lists - 1]);
  l->cached_owned_lists[num_lists - 1] = NULL;
  return l->cached_owned_lists;
}				//listserv_getowned_lists

char** listserv_list_filelist (struct listserv *const l,
			       const char *const listname)
{
  listserv_free_cached_char2 (l);
  char *filelist = listserv_get (l, listname, "FILELIST");
  int char2_len = 100;
  int char2_pos = 0;
  l->cached_char2 = malloc (100 * sizeof(char**));
  while (filelist) {
    if (filelist[0] == '*' || (filelist[0] == '\n' && filelist[1] == '\n')) {
      filelist = strchr (filelist + 1, '\n');
      if (filelist) filelist++;
      if (filelist && filelist[0] == '\n' ) filelist++;
      continue;
    }
    if (filelist == NULL) break;
    char t1[100], t2[100];
    sscanf(filelist, "%s %s", t1, t2);
    if (char2_pos + 1 == char2_len) {
      char2_len+=100;
      l->cached_char2 = realloc (l->cached_char2,
				 char2_len * sizeof(char**));
    };
    l->cached_char2[char2_pos++] = strdup (t2);
    filelist = strchr(filelist+1, '\n');
  }
  l->cached_char2[char2_pos] = NULL;
  return l->cached_char2;
}

char*
listserv_get (struct listserv *l, const char * const listname,
	      const char * const extension)
{
  int i = 0;
  if (l->cached_files) {
    if (strcasecmp (listname, l->cached_file_name)) {
      while (l->cached_files[i])
	free (l->cached_files[i++]);
      i = 0;
      free (l->cached_files);
      free (l->cached_file_name);
      l->cached_files = malloc (3 * sizeof(char*));
      l->cached_file_name = strdup (listname);
    } else {
      while (l->cached_files[i])
	if ((extension == NULL && l->cached_files[i][0] == '\0')
	    || (extension != NULL 
		&& strcasecmp (extension, l->cached_files[i]) == 0))
	  return l->cached_files[i+1];
	else i += 2;
      l->cached_files = realloc (l->cached_files, (i+3) * sizeof (char*));
    }
  } else {
    l->cached_files = malloc (3 * sizeof(char*));
    l->cached_file_name = strdup (listname);
  }
  l->cached_files[i] = strdup ( extension ? extension : "");
  l->cached_files[i+2] = NULL;
  char *cmd;
  if (l->cached_files[i][0] != '\0' 
      && strcmp(l->cached_files[i], "HDR") != 0) {
    cmd = ALLOC_A (11 + strlen (listname) + strlen (extension));
    sprintf (cmd, "GET %s %s (MSG", listname, extension);
  } else if (strcmp(l->cached_files[i], "HDR") == 0) {
    cmd = ALLOC_A (21 + strlen (listname));
    sprintf (cmd, "GET %s (MSG NOLOCK HDR", listname);
  } else {
    cmd = ALLOC_A (17 + strlen (listname));
    sprintf (cmd, "GET %s (MSG NOLOCK", listname);
  }
  l->cached_files[i+1] = listserv_command (l, cmd);
  l->cached_command = NULL;
  FREE_A (cmd);
  return l->cached_files[i+1];
}
