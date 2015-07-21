%top{

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

/*
 * YYSTYPE[0] contians the header-field, "Header" or "Text"
 * YYSTYPE[1] contains the string to be searched for, in case of : (contains)
 * match, the text is surrounded by '*'
 * YYSTYPE[2] is NULL or the parameter of Reject/Discard
*/
#define ACTION_REJECT   'R'
#define ACTION_ALLOW    'A'
#define ACTION_MODERATE 'M'
#define ACTION_DISCARD  'D'
#define YYSTYPE         char**
#include "src/util.h"
}

%{
#include <ctype.h>
#define YY_NO_INPUT 1
%}
%option case-insensitive 8bit noyywrap never-interactive
%option noyyget_leng noyyget_text noinput
%option noyyget_lineno noyyset_lineno noyyget_in noyyset_in 
%option noyyget_out noyyset_out noyyget_lloc noyyset_lloc
%option noyyget_debug noyyset_debug
%option bison-bridge nodefault
/* bison-locations stack read */ 
%option reentrant pointer align fast nounput 
%option prefix="listserv_content_filter" backup
%x ACTION

%%
<INITIAL><<EOF>>                        yyterminate ();

<INITIAL>[^:\ \n]+:\                    |
<INITIAL>[^:\ \n]+:\ \n                 |
<INITIAL>[^:\ \n]+:\ [^\n]+             |
<INITIAL>[^:\ \n]+:\ [^\n]+\n           {
            BEGIN (ACTION);
            YYSTYPE val = *yyget_lval (yyscanner);
            char *a = strchr (yytext, ':');
            a[0] = '\0';
            val[0] = strdup (yytext); /* header-field */
            a+=2;
            if (yytext[yyleng - 1] == '\n')
              yytext[yyleng - 1] = '\0';
            int strlen_a = strlen (a);
            val[1] = malloc (strlen_a + 3); /* header-value */
            val[1][0] = '*';
            strcpy (val[1]+1, a);
            val[1][strlen_a+1] = '*';
            val[1][strlen_a+2] = '\0';
        }

<INITIAL>[^:\ \n]+:[^:\ \n].*           |
<INITIAL>[^:\ \n]+:[^:\ \n].*\n         |
<INITIAL>[:\ ].*                        |
<INITIAL>[^:\ \n]+                      |
<INITIAL>[^:\ \n]+:\n                   |
<INITIAL>[^:\ \n]+:                     |
<INITIAL>[^:\ \n]+\n                    |
<INITIAL>[^:\ \n]+[^:\n]                |
<INITIAL>[^:\ \n]+[^:\n]\n              {
            BEGIN (ACTION);
            YYSTYPE ret = *yyget_lval (yyscanner);
            ret[0] = strdup ("Text");
            if (yytext[yyleng - 1] == '\n')
               yytext[yyleng -1] = '\0';
            ret[1] = strdup (yytext);
        }


<INITIAL>[^:\ \n]+::                    |
<INITIAL>[^:\ \n]+::\n                  |
<INITIAL>[^:\ \n]+::\                   |
<INITIAL>[^:\ \n]+::\ \n                { /* exact-match -- empty/no header */
            BEGIN (ACTION);
            YYSTYPE val = *yyget_lval (yyscanner);
            char *a = strchr (yytext, ':');
            a[0] = '\0';
            val[0] = strdup (yytext);
            val[1] = strdup ("");
        }

<INITIAL>[^:\ \n]+::\ [^\n]+            |
<INITIAL>[^:\ \n]+::\ [^\n]+\n          {
            BEGIN(ACTION);
            YYSTYPE val = *yyget_lval (yyscanner);
            char *a = strchr(yytext, ':');
            a[0] = '\0';
            val[0] = strdup (yytext); /* header-field */
            if (yytext[yyleng - 1] == '\n')
              yytext[yyleng - 1] = '\0';
            val[1] = strdup (a+3); /* header-value */
        }

<INITIAL>\n

<ACTION>ACTION:\ ALLO                 |
<ACTION>ACTION:\ ALL                  |
<ACTION>ACTION:\ AL                   |
<ACTION>ACTION:\ REJEC                |
<ACTION>ACTION:\ REJE                 |
<ACTION>ACTION:\ REJ                  |
<ACTION>ACTION:\ RE                   |
<ACTION>ACTION:\ MODERAT              |
<ACTION>ACTION:\ MODERA               |
<ACTION>ACTION:\ MODER                |
<ACTION>ACTION:\ MODE                 |
<ACTION>ACTION:\ MOD                  |
<ACTION>ACTION:\ MO                   |
<ACTION>ACTION:\ DISCAR               |
<ACTION>ACTION:\ DISCA                |
<ACTION>ACTION:\ DISC                 |
<ACTION>ACTION:\ DIS                  |
<ACTION>ACTION:\ DI                   |
<ACTION>ACTION:\ .                    |
<ACTION>ACTION:\ \n                   |
<ACTION>ACTION:.                      |
<ACTION>ACTION.                       |
<ACTION>ACTION.\n                     |
<ACTION>ACTION\n                      |
<ACTION>ACTION                        |
<ACTION>ACTIO                         |
<ACTION>ACTI                          |
<ACTION>ACT                           |
<ACTION>AC                            |
<ACTION>\n                            |
<ACTION><<EOF>>                       |
<ACTION>.                             {
            BEGIN (INITIAL);
            YYSTYPE val = *yyget_lval (yyscanner);
            val[2] = NULL;
            if (yytext[0] != '\0')
              yyless(0);
            return ACTION_REJECT;
         }

<ACTION>ACTION:\ (REJECT|DISCARD|MODERATE|ALLOW)[^\n]*    |
<ACTION>ACTION:\ (REJECT|DISCARD|MODERATE|ALLOW)[^\n]*\n  {
            BEGIN (INITIAL);
            YYSTYPE val = *yyget_lval (yyscanner);
            if (yytext[yyleng -1] == '\n') 
                yytext[yyleng -1] = '\0';
            char *a = strchr (yytext+9, ' ');
            val[2] = (a && a+1) ? strdup (a+1) : NULL;
            switch (tolower (yytext[8])) {
                case 'a': return ACTION_ALLOW;
                case 'r': return ACTION_REJECT;
                case 'm': return ACTION_MODERATE;
                case 'd': return ACTION_DISCARD;
                default : return 0;
            }
        }
%%
struct listserv_content_filter**
listserv_getcontent_filter (struct listserv* const l,
                            const char* const listname)
{
  listserv_free_cached_content_filter (l);
  l->cached_content_filter = NULL;
  char *content_filter_template = listserv_getmail_template (l, listname,
							     "CONTENT_FILTER");
  if (content_filter_template == NULL) {
    l->cached_content_filter = malloc (sizeof (struct
					       listserv_content_filter*));
    l->cached_content_filter[0] = NULL;
    return l->cached_content_filter;
  }
  yyscan_t yyscanner;
  listserv_content_filterlex_init (&yyscanner);
  YY_BUFFER_STATE buf =
    listserv_content_filter_scan_string (content_filter_template, yyscanner);
  char** val = ALLOC_A (3 * sizeof (char*));
  int i, j= 1;
  while ((i = listserv_content_filterlex (&val, yyscanner))) {
    l->cached_content_filter = realloc (l->cached_content_filter,
					++j * sizeof (struct
						      listserv_content_filter*));
    struct listserv_content_filter* c = malloc (sizeof (struct
							listserv_content_filter));
    l->cached_content_filter[j - 2] = c;
    c->header = val[0];
    c->value = val[1];
    c->text = val[2];
    c->action = i;
  }
  l->cached_content_filter[j - 1] = NULL;
  FREE_A (val);
  listserv_content_filter_delete_buffer (buf, yyscanner);
  listserv_content_filterlex_destroy (yyscanner);
  return l->cached_content_filter;
}
