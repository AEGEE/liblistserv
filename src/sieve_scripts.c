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
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct listserv_stringlist
{
  int num_elements;
  char **elements;
  char *string_presentation;
  int total_length_of_elements;
  int size_for_elements;
};

struct listserv_stringlist *listserv_stringlist_init ();
void listserv_stringlist_add (struct listserv_stringlist *const sl, char *text);
char *listserv_stringlist_string (struct listserv_stringlist *const sl);
void listserv_stringlist_destroy (struct listserv_stringlist *sl);

static inline char**
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


static inline int
keyword_doesnot_contain (struct listserv *l, const char const * listname, 
			 const char const *keyword,
			 const char const *value)
{
  char **kw = listserv_getlist_keyword(l, listname, keyword);
  int i = 0;
  while (kw[i])
    if (strcasecmp(value, kw[i]) == 0) return 0;
    else i++;
  return 1;
}

static inline void
listserv_content_filter_build_sieve_condition (struct string *str,
					       const char *const header_field,
					       const char *const header_value)
{
  char *ret = ALLOC_A ((header_value ? strlen (header_value) : 0) +
		       strlen (header_field) + 22);
  if (header_value == NULL)
    sprintf (ret, "header \"%s\" \"\"", header_field);
  else if (header_value[0] == '*')
    {
      if (strlen (header_value) > 2
	  && strchr (header_value + 1, '*') == strrchr (header_value + 1,
							'*'))
	{
	  // :contains, header_value has * at the beginning and at the end
	  sprintf (ret, "header :contains \"%s\" \"%s\"",
		   header_field, header_value + 1);
	  int len = strlen (ret);
	  ret[len - 2] = '\"';	//was '*'
	  ret[len - 1] = '\0';
	}
      else			//:matches, further * are contained
	sprintf (ret, "header :matches \"%s\" \"%s\"",
		 header_field, header_value);
    }
  else				//does not start with * and is not empty => :is
    sprintf (ret, "header \"%s\" \"%s\"", header_field, header_value);
  str_concat (str, ret);
  FREE_A (ret);
}

static inline char *
listserv_content_filter_build_script (struct listserv *l,
				      const char *const listname,
				      const char *const reject,
				      const char non_member)
{
  struct listserv_content_filter **a = listserv_getcontent_filter (l,
								   listname);
  if (a[0])
    {				//there is CONTENT_FILTER
      int i = 0, j = 0;
      struct string *list_script = str_init ();
      str_concat (list_script, "\r\n# Generated from >>> CONTENT_FILTER\r\n");
      while (a[i++]);
      char **header_field = ALLOC_A (sizeof (char *) * i);
      char **header_value = ALLOC_A (sizeof (char *) * i);
      char *operation = ALLOC_A (sizeof (char) * i);
      char **text = ALLOC_A (sizeof (char *) * i);
      i = 0;
      while (a[i++])
	if (strcmp (a[i - 1]->header, "Text")
	    && strcmp (a[i - 1]->header, "Header"))
	  {
	    header_field[j] = a[i - 1]->header;
	    header_value[j] = a[i - 1]->value;
	    text[j] = a[i - 1]->text;
	    operation[j++] = a[i - 1]->action;
	  }
	else
	  break;
      operation[j] = '\0';
      for (i = 0; i < j; i++)
	if (operation[i] != 'R')
	  {	// DISCARD, ALLOW, MODERATE
	    int k = 1;
	    while (operation[i + k] != 'R' && operation[i + k++] != '\0');
	    if (operation[i + k - 1] == '\0')
	      k--;
	    str_concat (list_script, "if ");
	    if (k == 1)
	      listserv_content_filter_build_sieve_condition (list_script,
							     header_field[i],
							     header_value[i]);
	    else
	      {
		str_concat (list_script, "anyof (");
		while (k)
		  {
		    listserv_content_filter_build_sieve_condition
		      (list_script, header_field[i], header_value[i]);
		    k--;
		    i++;
		    str_concat (list_script, ",\r\n         ");
		  };
		i--;
		list_script->str[list_script->pos - 12] = ')';
		list_script->str[list_script->pos - 11] = '\0';
		list_script->pos -= 11;
	      }
	    str_concat (list_script, " {\r\n    stop;\r\n}\r\n\r\n");
	  }
	else
	  {			// REJECT !!!
	    str_concat (list_script, "if ");
	    listserv_content_filter_build_sieve_condition (list_script,
							   header_field[i],
							   header_value[i]);
	    str_concat (list_script, " {\r\n    ");
	    str_concat (list_script, reject);
	    str_concat (list_script, " \"Your posting to the ");
	    str_concat (list_script, listname);
	    str_concat (list_script,
			" list has been rejected by the content filter.");
	    if (text[i])
	      {
		str_concat (list_script, "\r\n");
		str_concat (list_script, text[i]);
	      }
	    str_concat (list_script,
			"\r\n${advertisement}\r\nResult of SpamAssassin evaluation follows:${headers.X-Aegee-Spam-Report}");
	    str_concat (list_script, "\";");
	    if (non_member || operation[i+1])
	      str_concat (list_script, "\r\n    stop;");
	    str_concat (list_script, "\r\n}\r\n\r\n");
	  }			//end if Reject, Allow, Moderate, Discard
      //    printf("script %s\n", list_script->str);
      FREE_A (text);
      FREE_A (header_value);
      FREE_A (header_field);
      FREE_A (operation);
      return str_free (list_script);
    }
  else
    return NULL;
}

static inline void
insert_emails_from (struct listserv *l,
		    const char *const listname,
		    const char *const keyword,
		    struct listserv_stringlist *emails,
		    struct listserv_stringlist *extlists,
		    struct listserv_stringlist *loop_protection,
		    const unsigned int extensions)
{
  int j = loop_protection->num_elements;
  char *temp = ALLOC_A (strlen (listname) + strlen (keyword) + 2);
  sprintf (temp, "%s|%s", listname, keyword);
  listserv_stringlist_add (loop_protection, temp);
  FREE_A (temp);
  if (j == loop_protection->num_elements)
    return;
  if (keyword[0] == '\0' || strcasecmp(keyword, "Private") == 0)
    {
      char **sublists = listserv_getlist_keyword (l, listname, "Sub-Lists");
      if (sublists[0]) {
	char **temp = listserv_duplicate_char2(sublists);
	int k = 0;
	while (temp[k]) {
	  int m = 0;
	  while (temp[k][m]) {
	      temp[k][m] = tolower (temp[k][m]);
	      m++;
	  }
	  insert_emails_from (l, temp[k], "", emails, extlists,
			      loop_protection, extensions);
	  k++;
	}
	listserv_free_char2(temp);
      }
				//insert all subscribers from list
      if (extensions & 8)
	{			//provided that extlists is supported
	  temp = ALLOC_A (strlen (listname) + 10);
	  sprintf (temp, "listserv:%s", listname);
	  listserv_stringlist_add (extlists, temp);
	  FREE_A (temp);
	}
      else
	{			//no extlists support
	  struct listserv_subscriber **ls =
	    listserv_getsubscribers (l, listname);
	  j = 0;
	  while (ls[j])
	    {
	      listserv_stringlist_add (emails, ls[j]->email);
	      j++;
	    }
	}
      return;
    }
  char **keywords = listserv_getlist_keyword (l, listname, keyword);
  if (keywords)
    {
      j = 0;
      while (keywords[j])
	{
	  if (strchr (keywords[j++], '@'))  /* we have an email address */
	    listserv_stringlist_add (emails, keywords[j - 1]);
	  else if ((strchr (keywords[j - 1], '(') != NULL)
		   || (strcasecmp (keywords[j - 1], "Private") == 0))
	    {			/* external list */
	      char *kw;
	      if (strcasecmp (keywords[j-1], "Private") == 0) {
		kw = malloc (strlen (listname) + 3);
		sprintf(kw, "(%s)", listname);
	      } else kw = strdup (keywords[j - 1]);
	      char *temp = kw;
	      while (temp[0] != '(')
		temp++;
	      temp[0] = '\0';
	      temp++;
	      char *list = temp;
	      while (temp[0] && temp[0] != ')')
		temp++;
	      if (temp[0])
		temp[0] = '\0';
	      if (extensions & 8)
		{		//supports external lists
		  temp = ALLOC_A (strlen (kw) + strlen (list) + 11);
		  if (kw[0])
		    sprintf (temp, "listserv:%s|%s", list, kw);
		  else
		    sprintf (temp, "listserv:%s", list);
		  listserv_stringlist_add (extlists, temp);
		  FREE_A (temp);
		}
	      else
		{		//does not support external lists
		  if (strcasecmp (kw, "Owners") == 0)
		    kw[5] = '\0';
		  insert_emails_from (l, list, kw, emails,
				      extlists, loop_protection,
				      extensions);
		  keywords = listserv_getlist_keyword (l, listname, keyword);
		}
	      free (kw);
	    }
	  else if (strcasecmp (keywords[j - 1], "quiet:") == 0
		   && ((extensions & 4096) == 0))
	    break;
	}
    }
}

char **
listserv_getemails_fromkeywords (struct listserv *l,
				 const char *const listname,
				 const char** keywords)
{
  int i = 0;
  struct listserv_stringlist *emails = listserv_stringlist_init ();
  struct listserv_stringlist *loop_protection = listserv_stringlist_init ();

  while (keywords[i]) {
    insert_emails_from (l, listname, keywords[i], emails,
			NULL, loop_protection, 4096);
    i++;
  }
  if (emails->num_elements == emails->size_for_elements)
    {
      emails->elements = realloc (emails->elements,
				  (emails->num_elements + 1)
				  * sizeof (char *));
    }
  emails->elements[emails->num_elements] = NULL;
  listserv_free_cached_char2 (l);
  l->cached_char2 = emails->elements;
  free (emails);
  listserv_stringlist_destroy (loop_protection);
  return l->cached_char2;
}

static inline char *
listserv_size_criterion (struct listserv *l,
			 const char *const listname,
			 const unsigned int extensions,
			 const char *const reject)
{
  if ((extensions & 4096) == 0)
    {
      
      if (keyword_doesnot_contain(l, listname, "Misc-Options", "DISCARD_HTML")
	  && keyword_doesnot_contain(l, listname, "Language", "nohtml")
	  && keyword_doesnot_contain(l, listname, "Attachments", "Filter"))
	{
	  char** keyword = listserv_getlist_keyword (l, listname, "Sizelim");
	  int size_ = atol (keyword[0]);
	  char *size = ALLOC_A (15);
	  if (size_ % 1073741824 == 0)	// 2^30
	    sprintf (size, "%dG", size_ / 1073741824);
	  else if (size_ % 1048576 == 0)	// 2^20
	    sprintf (size, "%dM", size_ / 1048576);
	  else if (size_ % 1024 == 0)	// 2^10
	    sprintf (size, "%dK", size_ / 1024);
	  else
	    sprintf (size, "%d", size_);
	  struct string *temp = str_init ();
	  str_concat (temp, "if size :over ");
	  str_concat (temp, size);
	  str_concat (temp, " {\r\n    ");
	  str_concat (temp, reject);
	  str_concat (temp, " \"Your mail to ${envelope.to} exceeds ");
	  str_concat (temp, size);
	  if (size[0] == '1'
	      && (size[1] == 'K' || size[1] == 'M' || size[1] == 'G'))
	    str_concat (temp, "Byte.  ");
	  else
	    str_concat (temp, "Bytes.  ");
	  str_concat (temp,
		      "Upload the attachments\r\nin internet and include a link to them in your email.\r\n${advertisement}\";\r\n    stop;\r\n}\r\n\r\n");
	  FREE_A (size);
	  return str_free (temp);
	}
    }
  return strdup ("");
}

static inline char *
listserv_header_build_script (struct listserv *l,
			      const char *const listname,
			      const int unsigned extensions,
			      const char *const reject,
			      char *envelope, char *extlist, char *reject_)
{
  char private = 1, explicit_private = 0, non_member = 0, hold = 0;
  int j = 0;
  char *size_criterion = listserv_size_criterion (l, listname,
						  extensions, reject);
  char **keyword = listserv_getlist_keyword (l, listname, "Send");
  while (keyword[j++])
    if (strcasecmp (keyword[j - 1], "Public") == 0)
      private = 0;
    else if (strcasecmp (keyword[j - 1], "Non-Member") == 0)
      {
	private = 0;
	non_member = 1;
	break;
      }
    else if (strcasecmp (keyword[j - 1], "Hold") == 0) 
      {
	explicit_private = 1;
	hold = 1;
      }
    else if (strcasecmp (keyword[j - 1], "Private") == 0)
      explicit_private = 1;
  struct listserv_stringlist *posting_subscribers =
    listserv_stringlist_init (),
    //subscribers of which lists can post
    *extlists = listserv_stringlist_init ();
  if (private || non_member)
    {
      if (l->subscribers[0])
	{
	  j = 0;
	  while (l->subscribers[j])
	    if (!(l->subscribers[j++]->options & 0x01))
	      listserv_stringlist_add (posting_subscribers,
				       l->subscribers[j - 1]->email);
	}
    }
  struct listserv_stringlist *loop = listserv_stringlist_init ();
  insert_emails_from (l, listname, "Configuration-Owner",
		      posting_subscribers, extlists,
		      loop, extensions | 4096);
  insert_emails_from (l, listname, "Editor",
		      posting_subscribers, extlists,
		      loop, extensions | 4096);
  insert_emails_from (l, listname, "Moderator",
		      posting_subscribers, extlists,
		      loop, extensions | 4096);
  insert_emails_from (l, listname, "Owner",
		      posting_subscribers, extlists,
		      loop, extensions | 4096);
  insert_emails_from (l, listname, "Send",
		      posting_subscribers, extlists,
		      loop, extensions | 4096);
  if (hold) {
    keyword = listserv_getlist_keyword (l, listname, "Sub-Lists");
    if (keyword[0])
      {
	char **temp = listserv_duplicate_char2(keyword);
	j = 0;
	while (temp[j])
	  {
	    insert_emails_from (l, temp[j], "",
				posting_subscribers, extlists,
				loop, extensions);
	    j++;
	  }
	listserv_free_char2(temp);
      }
  }
  listserv_stringlist_destroy (loop);
  struct string *script = str_init ();
  if (private)
    {
      char *posting_subs = listserv_stringlist_string (posting_subscribers);
      char *posting_extl = listserv_stringlist_string (extlists);
      str_concat (script, "# Generated from List Header\r\n");
      if (extensions & 1)
	{
	  str_concat (script, "if not ");
	  if (posting_subs[0])
	    {
	      if (extlists->num_elements)
		str_concat (script, "anyof ( ");
	      str_concat (script, "envelope \"from\" [\"\", ");
	      *envelope = 1;
	      str_concat (script, posting_subs);
	      str_concat (script, "]");
	      if (extlists->num_elements)
		str_concat (script, ",\r\n              ");
	    }
	  if (extlists->num_elements)
	    {
	      str_concat (script, "envelope :list \"from\" [");
	      *extlist = 1;
	      *envelope = 1;
	      str_concat (script, posting_extl);
	      str_concat (script, "]");
	      if (posting_subs[0])
		str_concat (script, ")");
	    }
	  str_concat (script, " {\r\n    ");
	  str_concat (script, reject);
	  *reject_ = 1;
	  str_concat (script, " \"");
	  str_concat (script,
		      listserv_getmail_template (l, listname,
						 "SIEVE_CANNOT_POST_ENVELOPE_MSG"));
	  str_concat (script, "\";\r\n    stop;\r\n}\r\n");
	}
      str_concat (script, size_criterion);
      str_concat (script, "if not ");
      if (posting_subs[0])
	{
	  if (extlists->num_elements)
	    str_concat (script, "anyof ( ");
	  str_concat (script,
		      "address [\"From\", \"Sender\", \"Resent-From\", \"Resent-Sender\"] [");
	  str_concat (script, posting_subs);
	  str_concat (script, "]");
	  if (extlists->num_elements)
	    str_concat (script, ",\r\n              ");
	}
      if (extlists->num_elements)
	{
	  str_concat (script,
		      "address :list [\"From\", \"Sender\", \"Resent-From\", \"Resent-Sender\"] [");
	  *extlist = 1;
	  str_concat (script, posting_extl);
	  str_concat (script, "]");
	  if (posting_subs[0])
	    str_concat (script, ")");
	}
      str_concat (script, " {\r\n    ");
      str_concat (script, reject);
      *reject_ = 1;
      str_concat (script, " \"");
      str_concat (script,
		  listserv_getmail_template (l, listname,
					     "SIEVE_CANNOT_POST_MIME_MSG"));
      str_concat (script, "\";\r\n    stop;\r\n}\r\n");
    }
  else				//if not private
    str_concat (script, size_criterion);
  free (size_criterion);
  char *content_filter =
    listserv_content_filter_build_script (l, listname, reject, non_member);
  *reject_ = 1;
  if (content_filter)
    str_concat (script, content_filter);
  free (content_filter);
  listserv_stringlist_destroy (extlists);
  //If Send =  Public,Confirm,Non-Member
  if (non_member)
    {
      str_concat (script, "# Non-Member\r\n");
      str_concat (script,
		  "if not address [\"From\", \"Sender\", \"Resent-From\", \"Resent-Sender\"] [");
      str_concat (script, listserv_stringlist_string (posting_subscribers));
      str_concat (script, "] {\r\n");

      struct listserv_stringlist *emails = listserv_stringlist_init ();
      struct listserv_stringlist *elists = listserv_stringlist_init ();
      struct listserv_stringlist *loop = listserv_stringlist_init ();
      insert_emails_from (l, listname, "Editor",
			  emails, elists, loop, extensions);
      insert_emails_from (l, listname, "Moderator",
			  emails, elists, loop, extensions);
      listserv_stringlist_destroy (loop);

      j = 0;
      while (j < emails->num_elements)
	{
	  str_concat (script, "    redirect \"");
	  str_concat (script, emails->elements[j]);
	  str_concat (script, "\";\r\n");
	  j++;
	}
      listserv_stringlist_destroy(emails);

      j = 0;
      while (j < elists->num_elements)
	{
	  str_concat (script, " redirect \"");
	  str_concat (script, elists->elements[j]);
	  str_concat (script, "\";\r\n");
	  j++;
	}
      listserv_stringlist_destroy(elists);
      str_concat (script, "}\r\n");
    }
  listserv_stringlist_destroy (posting_subscribers);
  return str_free (script);
}

static inline char *
listserv_request_script (struct listserv *l,
			 const char *const listname,
			 const int unsigned extensions,
			 const char *const reject)
{
  struct listserv_stringlist *emails = listserv_stringlist_init ();
  struct listserv_stringlist *elists = listserv_stringlist_init ();//external lists
  struct listserv_stringlist *loop = listserv_stringlist_init ();
  insert_emails_from (l, listname, "Owner",
		      emails, elists, loop, extensions | 4096);
  insert_emails_from (l, listname, "Configuration-Owner",
		      emails, elists, loop, extensions | 4096);
  listserv_stringlist_destroy (loop);
  struct string *gstr = str_init ();
  str_concat (gstr, "require [\"vacation\", \"");
  str_concat (gstr, reject);
  str_concat (gstr, "\"");
  if (elists->num_elements)
    str_concat (gstr, ", \"extlists\"");
  str_concat (gstr,
	      "];\r\nif header :contains \"X-Aegee-Spam-Level\" \"+++\" {\r\n    ");
  str_concat (gstr, reject);
  str_concat (gstr, " \"");
  str_concat (gstr,
	      listserv_getmail_template (l, listname, "SPAM_REQUEST_MSG"));
  str_concat (gstr, "\";\r\n\
} else {\r\n");
  int i;
  for (i = 0; i < emails->num_elements; i++)
    {
      str_concat (gstr, "    redirect \"");
      str_concat (gstr, emails->elements[i]);
      str_concat (gstr, "\";\r\n");
    }
  if (elists->num_elements)
    for (i = 0; i < elists->num_elements; i++)
      {
	str_concat (gstr, "    redirect :list \"");
	str_concat (gstr, elists->elements[i]);
	str_concat (gstr, "\";\r\n");
      }

  str_concat (gstr, "\r\n    if not exists \"X-Aegee-Spam-Level\" {\r\n\
        vacation :handle \"request-vacation\" :subject \"How to unsubscribe from ");
  str_concat (gstr, listname);
  str_concat (gstr, "\" \"Hello,\r\n\
\r\n\
The most common question people ask here is how to unsubscribe from the\r\n\
mailing list ");
  str_concat (gstr, listname);
  str_concat (gstr, ".  This is an automatic answer \r\n\
to this question.  There are two options:\r\n\
  -- Send an email from the address you are subscribed in the mailing list to \r\n\
listserv@aegee.org containing (not in the subject, not as HTML, just plain\r\n\
text):\r\n\
      SIGNOFF ");
  str_concat (gstr, listname);
  str_concat (gstr, "\r\n\r\n\
  -- Visit https://lists.aegee.org/cgi-bin/wa?SUBED1=");
  str_concat (gstr, listname);
  str_concat (gstr, "&A=1 ,\r\nfill in your email address and click on \\\"Unsubscribe\\\".\r\n\r\n\
Kind regards\r\n\r\n\
Your AEGEE Mail Team\r\n\
http://mail.aegee.org\r\n\
email: mail@aegee.org\r\n\
sip:   8372@aegee.org\r\n\";\r\n    }\r\n}\r\n");
  listserv_stringlist_destroy (emails);
  listserv_stringlist_destroy (elists);

  return str_free (gstr);
}

static inline char *
listserv_signoff_request (const char *const listname,
			  const char *const subscribers,
			  const int extensions, const char *const reject)
{
  static const char * const ret_text = "BG - Ne ste abonat na %s.\r\n\
CA - No ets suscrit a la llista %s.\r\n\
DE - Sie sind kein Mitglied der Liste %s.\r\n\
EN - You, ${envelope.from} are not subscribed to %s.\r\n\
ES - No estas suscrito a la lista %s.\r\n\
FR - Vous n'etes pas inscrit sur la liste %s.\r\n\
IT - Non sei iscritto alla lista %s.\r\n\
LV - Tu neesi pierakstijies listei %s.\r\n\
PT - Nao esta inscrito na lista %s.\r\n\
SR - Niste pretplaceni na %s.\r\n\
${advertisement}";
  char *ret_text2 = ALLOC_A (strlen (ret_text) + 10 * strlen (listname) - 19);	// +19 = 20 - 1= 10 x 2 ("%s") - '\0'
  sprintf (ret_text2, ret_text, listname /* BG */ , listname /* CA */ ,
	   listname /* DE */ , listname /* EN */ , listname /* ES */ ,
	   listname /* FR */ , listname /* IT */ , listname /* LV */ ,
	   listname /* PT */ , listname /* SR */ );
  struct string *s = str_init ();
  struct string *the_rejection_command = str_init ();
  str_concat (the_rejection_command, reject);
  str_concat (the_rejection_command, " \"");
  str_concat (the_rejection_command, ret_text2);
  FREE_A (ret_text2);
  str_concat (the_rejection_command, "\";\r\n");
  char *rejection_command = str_free (the_rejection_command);
  str_concat (s, "require [");
  if (extensions & 1)
    str_concat (s, "\"envelope\", ");
  str_concat (s, "\"");
  str_concat (s, reject);
  str_concat (s, "\"];\r\n");
  if (*subscribers == '\0')
    str_concat (s, rejection_command);
  else
    {
      str_concat (s, "if not ");
      if (extensions & 1)
	{
	  str_concat (s, "anyof (envelope \"from\" [\"\", ");
	  str_concat (s, subscribers);
	  str_concat (s, "],\r\n              ");
	}
      str_concat (s,
		  "address [\"From\", \"Sender\", \"Resent-From\", \"Resent-Sender\"] [");
      str_concat (s, subscribers);
      str_concat (s, "]");
      if (extensions & 1)
	str_concat (s, ")");
      str_concat (s, " {\r\n    ");
      str_concat (s, rejection_command);
      str_concat (s, "}");
    }
  free (rejection_command);
  return str_free (s);
}

static inline char *
listserv_subscribe_request (struct listserv *l,
			    const char *const listname,
			    const char *const reject)
{
  char **kw = listserv_getlist_keyword (l, listname, "Subscription");
  struct string *ret = str_init ();
  str_concat (ret, "require \"");
  str_concat (ret, reject);
  str_concat (ret, "\";\r\n");
  if (kw[0] && strcasecmp (kw[0], "Closed") == 0)
    {
      str_concat (ret, reject);
      str_concat (ret, " \"${envelope.from} cannot subscribe to ");
      str_concat (ret, listname);
      str_concat (ret, ".  Contact the listowners at ");
      str_concat (ret, listname);
      str_concat (ret,
		  "-request@aegee.org for clarification how to join the list.\r\n${advertisement}\";\r\n");
    }
  else
    {
      str_concat (ret,
		  "if header :contains \"X-Aegee-Spam-Level\" \"+++++\" {\r\n    ");
      str_concat (ret, reject);
      str_concat (ret,
		  " \"Your mail was evaluated as spam (see below for details).\r\n  To join ");
      str_concat (ret, listname);
      str_concat (ret,
		  " visit\r\nhttps://lists.aegee.org/cgi-bin/wa?SUBED1=");
      str_concat (ret, listname);
      str_concat (ret,
		  "&A=1 .\r\n${advertisement}${headers.X-Aegee-Spam-Report}\";\r\n}");
    }
  return str_free (ret);
}

static inline char *
listserv_check_subscription (const char *const all_subscribers,
			     const int extensions, const char *const reject)
{
  if ((extensions & 32) == 0
      && ((extensions & 1) == 0
	  || ((extensions & 2) == 0 && (extensions & 4) == 0)))
    return strdup ("");
  // no ihave & (no envelope || (no reject && no ereject))
  struct string *ret = str_init ();
  str_concat (ret, "require ");
  if (extensions & 32)
    {
      str_concat (ret, "\"ihave\";");
    }
  else
    {				//no ihave 
      str_concat (ret, "[\"envelope\", \"");
      str_concat (ret, reject);
      str_concat (ret, "\"];\r\n\
    if not envelope \"from\" [\"\"");
      if (*all_subscribers)
	{
	  str_concat (ret, ", ");
	  str_concat (ret, all_subscribers);
	}
      str_concat (ret, "] {\r\n	");
      str_concat (ret, reject);
      str_concat (ret, " \"Not subscribed\";\r\n\
} else {\r\n\
	discard;\r\n\
}\r\n");
    }
  return str_free (ret);
}

static inline char *
listserv_owner (const char *const all_subscribers,
		const int unsigned extensions, const char *const reject)
{
  if (extensions & 1)
    return strdup ("");
  struct string *ret = str_init ();
  str_concat (ret, "require [\"");
  str_concat (ret, reject);
  if (*all_subscribers)
    {
      if (extensions & 1)
	{
	  str_concat (ret, "\", \"envelope\"];\r\n");
	  str_concat (ret, "if not envelope \"from\" [\"\", ");
	  str_concat (ret, *all_subscribers ? all_subscribers : "\"\"");
	  str_concat (ret, "] {\r\n    ");
	  str_concat (ret, reject);
	  str_concat (ret,
		      " \"${envelope.from} cannot post here.\r\n${advertisement}\";\r\n}\r\n");
	}
    }
  else
    {
      str_concat (ret, "\"];\r\n");
      str_concat (ret, reject);
      str_concat (ret,
		  "  \"${envelope.from} cannot post here.\r\n${advertisement}\";\r\n");
    }
  return str_free (ret);
}

//static functions END

//extensions: 1 envelope, 2 reject, 4 ereject, 8 extlists
//16 variables, 32 ihave, 4096 check for size
char **
listserv_getsieve_scripts (struct listserv *const l,
			   const char *const listname,
			   const int unsigned extensions)
{
  if ((extensions & 32) == 0 &&	//no ihave
      (extensions & 2) == 0 &&	//no reject
      (extensions & 4) == 0) {	//no ereject 
    listserv_free_cached_char2 (l);
    l->cached_char2 = malloc(sizeof(char*));
    l->cached_char2[0] = NULL;
    return l->cached_char2;
  }
  struct listserv_subscriber **_list_subscribers =
    listserv_getsubscribers (l, listname);
  struct string *all_subscribers = str_init ();
  if (_list_subscribers[0])
    {
      int j = 0, k;
      while (_list_subscribers[j])
	{
	  k = 0;
	  while (_list_subscribers[j]->email[k])
	    {
	      _list_subscribers[j]->email[k] =
		tolower (_list_subscribers[j]->email[k]);
	      k++;
	    }
	  str_concat (all_subscribers, "\"");
	  str_concat (all_subscribers, _list_subscribers[j]->email);
	  str_concat (all_subscribers, "\", ");
	  j++;
	}

      all_subscribers->pos -= 2;
      all_subscribers->str[all_subscribers->pos] = '\0';
    }
  const char *reject;
  if (extensions & 4)
    reject = "ereject";
  else if (extensions & 2)
    reject = "reject";
  else
    reject = "";
  char envelope = 0, extlist = 0, reject_ = 0;
  char *list_header = listserv_header_build_script (l, listname,
						    extensions, reject, 
						    &envelope, &extlist,
						    &reject_);
  struct string *temp = str_init ();
  if (list_header)
    {
      if (extensions & 32)
	str_concat (temp, "require \"ihave\";\r\n\r\n");
      else
	{
	  if (envelope || extlist || reject_) {
	    str_concat (temp, "require ");
	    if (envelope + extlist + reject_ > 1)
	      str_concat(temp, "[");
	    if (envelope) {
	      str_concat(temp, "\"envelope\""); 
	      if (extlist || reject_)
		str_concat(temp, ", ");
	    }
	    if (extlist) {
	      str_concat(temp, "\"extlists\"");
	      if (reject_)
		str_concat(temp, ", ");
	    }
	    if (reject_) {
	      str_concat(temp, "\"");
	      str_concat(temp, reject);
	      str_concat(temp, "\"");
	    }
	    if (envelope + extlist + reject_ > 1)
	      str_concat(temp, "]");
	    if (envelope || extlist || reject_)
	      str_concat(temp, ";\r\n\r\n");
	  }
	}
      str_concat (temp, list_header);
      free (list_header);
    };

  char **ret = malloc (sizeof (char *) * 19);
  //x[ 0]  x[ 1] the list
  ret[0] = strdup (listname);
  ret[1] = str_free (temp);
  //    printf(ret[1]);
  char *script_name = ALLOC_A (strlen (listname) + 21);	//20 is the longest (-unsubscribe-request) + '\0'
  //x[ 2]  x[ 3] -request
  sprintf (script_name, "%s-request", listname);
  ret[2] = strdup (script_name);
  ret[3] = listserv_request_script (l, listname, extensions, reject);
  //x[ 4]  x[ 5] -search-request
  sprintf (script_name, "%s-search-request", listname);
  ret[4] = strdup (script_name);
  ret[5] = strdup ("");
  //x[ 6]  x[ 7] -server
  sprintf (script_name, "%s-server", listname);
  ret[6] = strdup (script_name);
  ret[7] = strdup ("");
  //x[ 8]  x[ 9] -unsubscribe-request
  sprintf (script_name, "%s-unsubscribe-request", listname);
  ret[ 8] = strdup (script_name);
  ret[ 9] = listserv_signoff_request (listname, all_subscribers->str,
				      extensions, reject);
  //x[10]  x[11] -signoff-request
  sprintf (script_name, "%s-signoff-request", listname);
  ret[10] = strdup (script_name);
  ret[11] = strdup (ret[9]);
  //x[12]  x[13] -subscribe-request -- OPTIONAL - if Subscribe != Closed
  sprintf (script_name, "%s-subscribe-request", listname);
  ret[12] = strdup (script_name);
  ret[13] = listserv_subscribe_request (l, listname, reject);
  ret[14] = NULL;
  //x[14]  x[15] -check-subscription
  //  sprintf (script_name, "%s-check-subscription", listname);
  //  ret[14] = strdup (script_name);
  //  ret[15] = listserv_check_subscription (all_subscribers->str,
  //					 extensions, reject);
  //x[16]  x[17] owner-list
  //  sprintf (script_name, "owner-%s", listname);
  //  ret[16] = strdup (script_name);
  //  ret[17] = listserv_owner (all_subscribers->str, extensions, reject);
  //x[16 or 18]  terminating NULL
  ret[16] = NULL;
  FREE_A (script_name);
  free (str_free (all_subscribers));
  listserv_free_cached_char2 (l);
  l->cached_char2 = ret;
  return ret;
}
