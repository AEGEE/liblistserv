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
struct listserv_list_keyword {
  char *keyword;
  char **values;
};

struct listserv_header_extra_type {
  int elements;
  int has_list_already_title;
};

#define YYSTYPE         struct listserv_list_keyword *
#define KEYWORD         300
#include "src/util.h"
}

%{
#include <ctype.h>
#define YY_EXTRA_TYPE	struct listserv_header_extra_type *
#define YY_NO_INPUT	1
%}

%option case-insensitive 8bit noyywrap 
%option noyyget_leng noyyget_text noinput
%option noyyget_lineno noyyset_lineno noyyget_in noyyset_in 
%option noyyget_out noyyset_out noyyget_lloc noyyset_lloc
%option noyyget_debug noyyset_debug
%option bison-bridge nodefault
%option reentrant pointer align full nounput
%option prefix="listserv_list_header" backup
%x VALUES KEYWORDLINE COMMENTLINE

%%
<INITIAL><<EOF>>                             yyterminate();

<INITIAL>\n                                  |
<INITIAL>.                                   ;

<COMMENTLINE>.*                              | /* cannot happen */
<COMMENTLINE>.*\n                            {
  YY_EXTRA_TYPE yy_extra = yyget_extra (yyscanner);
  if (yy_extra->has_list_already_title == 0 && yytext[1]) {
      int i = 1;
      while (yytext[i] && yytext[i] != '\n' 
             && (yytext[i] == ' ' || yytext[i] == 9)) i++;//9 is TAB
      if (i != yyleng) {//found non-blank character
          char *list_title = yytext + i;
          i = yyleng - 1;
          while (i > 0 
                 && (yytext[i] == ' ' || yytext[i] == 9) ) i--;//9 is TAB
          yytext[i] = '\0';
          yy_extra->has_list_already_title = 1;
          YYSTYPE val = *yyget_lval (yyscanner);
          val->keyword = strdup ("Title"); /* the keyword */
          val->values  = malloc (2 * sizeof(char*)); /* the values */
          val->values[0] = strdup (list_title);
          val->values[1] = NULL;
          BEGIN (INITIAL);
          return KEYWORD;
      }
  } else
      BEGIN (INITIAL);
                                             }

<INITIAL>.[\ \t]*\n                        |
<INITIAL>.[\ \t]*[-_A-Z]+[\ \t]*\n         |
<INITIAL>.[\ \t]*[^-_A-Z\n]                |
<INITIAL>.[\ \t]*[-_A-Z]+                  |
<INITIAL>.[\ \t]*[-_A-Z]+[\ \t]*[^=]       {
            YY_EXTRA_TYPE yy_extra = yyget_extra (yyscanner);
            if (yy_extra->has_list_already_title == 0)
                yyless (1);
            if (yy_extra->has_list_already_title == 0 ||
                yytext[yyleng-1] != '\n')
                BEGIN (COMMENTLINE);
                                             }

<INITIAL>.[\ \t]*[-_A-Z]+[\ \t]*=          {
            BEGIN (KEYWORDLINE);
            yyless (1);
        }

<KEYWORDLINE>[-A-Z_]+[\t\ ]*                 |
<KEYWORDLINE>[-A-Z_]+[\t\ ]*\n               |
<KEYWORDLINE>[^-A-Z_\n]+                     ; /* inline comment */

<KEYWORDLINE>\n                              BEGIN (INITIAL);

<KEYWORDLINE>[-A-Z_]+[\t\ ]*=                {
            BEGIN (VALUES);
            YYSTYPE val = *yyget_lval (yyscanner);
            int i = 1;
            while (yytext[i] != ' ' &&
                   yytext[i] != '=' &&
                   yytext[i] != '\t') i++; 
            yytext[i] = '\0';
            val->keyword = strdup (yytext); /* the keyword */
            val->values  = malloc (sizeof (char*)); /* the values */
            val->values[0] = NULL;
            YY_EXTRA_TYPE yy_extra = yyget_extra (yyscanner);
            yy_extra->elements = 1; /* the keyword has 0 values so far */
          }

<VALUES>\n                                    {
            BEGIN(INITIAL);
            return KEYWORD;
          }


<VALUES>[\ \t]*                               ;
<VALUES>,                                     ;  /* shall not happen */

<VALUES>\"[^\"\n]*\n                            | /* shall not happen */
<VALUES>\"[^\"\n]*                              | /* shall not happen */
<VALUES>[^\"\t\n, ]+[\t\ ]*\n                    |
<VALUES>[^\"\t\n, ]+[\t\ ]*,                     |
<VALUES>[^\"\t\n, ]+[\t\ ]*                      |
<VALUES>\"[^\"\n]*\"[\t\ ]*\n                   |
<VALUES>\"[^\"\n]*\"[\t\ ]*,                    |
<VALUES>\"[^\"\n]*\"[\t\ ]*                     { /* extract value */
            YYSTYPE val = *yyget_lval (yyscanner);
            int i = 0;
            if ((yytext[0] == '\"')) {
                do i++; while (yytext[i] != '"');
                i++;
            } else {
                while (yytext[i] != ' ' &&
                       yytext[i] != '\t' &&
                       yytext[i] != ',' &&
                       yytext[i] != '\n' &&
                       yytext[i] != '\0') i++; 
            }
            YY_EXTRA_TYPE yy_extra = yyget_extra (yyscanner);
            yy_extra->elements++; /* the keyword has 0 values so far */
            val->values = realloc (val->values,
                                   yy_extra->elements * sizeof(char*));
            if (yytext[i] == ' ' || yytext[i] == '\t') {
                    yytext[i] = '\0';
            };
            val->values[yy_extra->elements-1] = (void*)0;
            if (yytext[yyleng - 1 ] == ',') {
               yytext[i] = '\0';
               val->values[yy_extra->elements-2] = strdup (yytext);
            } else if (yytext[yyleng - 1] == '\n') {
                yytext[i] = '\0';
                val->values[yy_extra->elements-2] = strdup (yytext);
                BEGIN (INITIAL);
                return KEYWORD;
            } else if (yytext[yyleng - 1] != ',') {
                val->values[yy_extra->elements-2] = strdup (yytext);
                BEGIN (KEYWORDLINE);
                return KEYWORD;
            };
          }
%%

inline static int
cs (const char* const k)
{
  int i = ((tolower (k[0]) - 65) << 10) + 
          ((tolower (k[1]) - 65) <<  5) + 
           (tolower (k[1]) - 65);
  if (strlen (k) > 9) //cause of  Default-, Notify-, Editor-, Notebook-
    i = (i << 5) + tolower (k[8])-65;
  if (strlen (k) > 5) //cause of  Sender/Send
    i = (i << 5) + tolower (k[5])-65;
  //static j=0;  printf("case %9i: return %2i;//%s\n", i, ++j, k);
  return i;
  /*
  cs ("Ack");            cs("Attachments");              cs("Auto-Delete");
  cs ("Categories");     cs("Change-Log");               cs("Confidential");
  cs ("Configuration-Owner");  cs("Confirm-Defay");      cs("Daily-Threshold");
  cs ("DBMS");           cs("Default-Options");          cs("Default-Topics");
  cs ("Digest");         cs("Editor");                   cs("Editor-Header");
  cs ("Errors-To");      cs("Exit");                     cs("Files");
  cs ("Filter");         cs("Indent");                   cs("Internet-Via");
  cs ("Language");       cs("Limits");                   cs("List-Address");
  cs ("List-Id");	cs("Local");                    cs("Long-Lines");
  cs ("Loopcheck");	cs("Mail-Merge");               cs("Mail-Via");
  cs ("Misc-Options");   cs("Moderator");                cs("New-List");
  cs ("Newsgroups");     cs("NJE-Via");                  cs("Notebook");
  cs ("Notebook-Header");cs("Notify");                   cs("Owner");
  cs ("Peers");          cs("Prime");			cs("PW");
  cs ("Renewal");	cs("Reply-To");			cs("Review");
  cs ("RSS-Abstract-Words");  cs("Safe");		cs("Send");
  cs ("Sender");		cs("Service");			cs("SizeLim");
  cs ("Subject-Tag");	cs("Stats");			cs("Sub-lists");
  cs ("Subscription");	cs("Topics");			cs("Translate");
  cs ("Validate");       cs("X-Tags");
   */
}

static inline char
listserv_num_keyword (const int i)
{
  switch (i) { //cs(char *j)
  case     33890: return  1;//Ack
  case  35279303: return  2;//Attachments
  case  35312803: return  3;//Auto-Delete
  case  36734126: return  4;//Categories
  case  36970980: return  5;//Change-Log
  case  37207683: return  6;//Confidential
  case  37207078: return  7;//Configuration-Owner
  case  37207185: return  8;//Confirm-Delay
  case  37783052: return  9;//Daily-Threshold
  case     36929: return 10;//DBMS
  case  37918187: return 11;//Default-Options
  case  37918347: return 12;//Default-Topics
  case   1189171: return 13;//Digest
  case   1216657: return 14;//Editor
  case  38932657: return 15;//Editor-Header
  case   1231442: return 16;//Errors-To
  case     38679: return 17;//Exit
  case     39208: return 18;//Files
  case   1254705: return 19;//Filter
  case   1358291: return 20;//Indent
  case  43463085: return 21;//Internet-Via
  case   1442848: return 22;//Language
  case   1451314: return 23;//Limits
  case  46442048: return 24;//List-Address
  case   1451304: return 25;//List-Id
  case     45550: return 26;//Local
  case  46644395: return 27;//Long-Lines
  case   1457639: return 28;//Loopcheck
  case  47219948: return 29;//Mail-Merge
  case   1475637: return 30;//Mail-Via
  case  47490350: return 31;//Misc-Options
  case   1490400: return 32;//Moderator
  case   1512616: return 33;//New-List
  case  48403985: return 34;//Newsgroups
  case   1516872: return 35;//NJE-Via
  case   1523182: return 36;//Notebook
  case  48739758: return 37;//Notebook-Header
  case   1523192: return 38;//Notify
  case     48886: return 39;//Owner
  case     49316: return 40;//Peers
  case     49745: return 41;//Prime
  case     49878: return 42;//PW
  case   1643680: return 43;//Renewal
  case   1643628: return 44;//Reply-To
  case   1643702: return 45;//Review
  case  53038657: return 46;//RSS-Abstract-Words
  case     52256: return 47;//Safe
  case     52388: return 48;//Send
  case   1676465: return 49;//Sender
  case   1676450: return 50;//Service
  case   1680680: return 51;//SizeLim
  case  54187650: return 52;//Subject-Tag
  case     52883: return 53;//Stats
  case   1693352: return 54;//Sub-lists
  case  54187665: return 55;//Subscription
  case     53544: return 56;//Title, of the list
  case   1719794: return 57;//Topics
  case   1722955: return 58;//Translate
  case   1770528: return 59;//Validate
  case   1781170: return 60;//X-Tags
  default:
    return 0;
  }
}

#define set_default_value(x, y)  {			\
  l->keywords[i-1] = malloc ((x+1) * sizeof (char*));	\
  if (y[0] != '\0')					\
    l->keywords[i-1][  0] = strdup (y);			\
  l->keywords[i-1][x] = NULL;             	     	\
  };

static inline char**
listserv_keyword_default_value (struct listserv *l,
                                const char* const listname,
                                int i)
{
  switch (i) {
  case  0:
    set_default_value(1, "Unknown keyword");
    break;
  case  1: //Ack
  case  5: //Change-Log
  case  6: //Confidential
  case 10: //DBMS
  case 18: //Files
  case 29: //Mail-Merge
  case 59: //Validate
    set_default_value (1, "No");
    break;
  case  2: //Attachments
  case 15: //Editor-Header
  case 27: //Long-Lines
  case 38: //Notify
  case 41: //Prime
  case 47: //Safe
  case 58: //Translate
  case 60: //X-Tags
    set_default_value(1, "Yes");
    break;
  case  3: //Auto-Delete, depends on 59 = Validate
    if (l->keywords[58] != NULL
        && l->keywords[58][0] != NULL
	&& (strcasecmp (l->keywords[58][0], "Yes") == 0)) {
      set_default_value (1, "No");
    } else {
      set_default_value (4, "Yes");
      l->keywords[i-1][1] = strdup ("Semi-Auto");
      l->keywords[i-1][2] = strdup ("Delay(4)");
      l->keywords[i-1][3] = strdup ("Max(100)");
    }
    break;
  case  4: //Categories
  case 11: //Default-Options
  case 12: //Default-Topics
  case 14: //Editor
  case 16: //Errors-To
  case 17: //Exit
  case 19: //Filter
  case 21: //Internet-Via
  case 23: //Limits
  case 24: //List-Address
  case 25: //List-Id
  case 26: //Local
  case 31: //Misc-Options
  case 32: //Moderator
  case 33: //New-List
  case 34: //Newsgroups
  case 35: //NJE-Via
//  case 39: //Owner //cannot happen
  case 40: //Peers
  case 42: //PW
  case 43: //Renewal
  case 54: //Sub-lists
  case 56: //Title
  case 57: //Topics
    set_default_value (0, "");
    break;
  case  7: //Configuration-Owner
    ; int j = 0;
    while (l->keywords[38][j]) j++;
    set_default_value(j, "");
    j--;
    for (; j >= 0; j--)
      l->keywords[6][j] = strdup(l->keywords[38][j]);
    break;
  case  8: //Confirm-Delay
    set_default_value (1, "48");
    break;
  case  9://Daily-Threshold
    set_default_value (1, "50");
    break;
  case 13: //Digest
    if (l->keywords[35] != NULL /* Notebook */
        && l->keywords[35][0] 
	&& (strcasecmp (l->keywords[35][0], "Yes") == 0)) {
      set_default_value (4, "Yes");
      l->keywords[i-1][1] = strdup ("Same");
      l->keywords[i-1][2] = strdup ("Daily");
      l->keywords[i-1][3] = strdup ("Size(10000)");
    } else
      set_default_value (1, "No");
    break;
  case 20: //Indent
    set_default_value (1, "40");
    break;
  case 22: //Language
    set_default_value (1, "English");
    break;
  case 28: //Loopcheck
    set_default_value (1, "Normal");
    break;
  case 30: //Mail-Via
    set_default_value (1, "DISTRIBUTE");
    break;
  case 36: //Notebook
    set_default_value (4, "No");
    l->keywords[i-1][1] = strdup ("A");
    l->keywords[i-1][2] = strdup ("Single");
    l->keywords[i-1][3] = strdup ("Private");
  break;
  case 37: //Notebook-Header
    set_default_value (1, "Short");
    break;
  case 44: //Reply-To
    set_default_value (2, "List");
    l->keywords[i-1][1] = strdup ("Respect");
    break;
  case 45: //Review
    set_default_value (1, "Private");
    break;
  case 46: //RSS-Abstract-Words
    set_default_value (2, "100");
    l->keywords[i-1][1] = strdup ("50");
    break;
  case 48: //Send
    set_default_value (1, "Public");
    break;
  case 49: //Sender
    set_default_value (1, "List");
    break;
  case 51: //SizeLim
    set_default_value (1, "40000000");//500k lines * 80 chars
    break;
  case 50: //Service
    set_default_value (1, "*");
    break;
  case 52: //Subject-Tag
    set_default_value (1, listname);
    break;
  case 53: //Stats
    set_default_value (2, "Normal");
    l->keywords[i-1][1] = strdup ("Private");
    break;
  case 55: //Subscription
    set_default_value (1, "By_Owner");
    break;
  };
  return l->keywords[i-1];
}
#undef set_default_value

char**
listserv_getlist_keyword (struct listserv* const l,
                          const char* const listname, 
                          const char* const keyword)
{
  //get it from cache, if feasible, otherwise free the cache
  if (l->keywords_initialized_for_list == NULL 
      || (listname
          && strcasecmp (l->keywords_initialized_for_list, listname) != 0)) {
      listserv_free_keywords (l);
      l->keywords_initialized_for_list = strdup (listname);
      char* header = listserv_get (l, listname, "HDR");
      yyscan_t yyscanner;
      YY_EXTRA_TYPE yy_extra = ALLOC_A (sizeof (struct
                                               listserv_header_extra_type));
      yy_extra->has_list_already_title = 0;
      listserv_list_headerlex_init_extra (yy_extra, &yyscanner);
      YY_BUFFER_STATE buf = listserv_list_header_scan_string (header,
                                                              yyscanner);
      YYSTYPE val = ALLOC_A(sizeof(struct listserv_list_keyword));
      int i = 0;
      while((i = listserv_list_headerlex (&val, yyscanner))) {
        int num = listserv_num_keyword (cs (val->keyword));
        if (num == 0) {
            fprintf (stderr, "unrecognized keyword %s in list %s\n",
		     val->keyword, listname);
            free (val->values);
  	    continue;
        }
        num--;
        if (l->keywords[num] == NULL)
	    l->keywords[num] = val->values;
        else {
	  int x = 0, y = 0;
          while (l->keywords[num][x]) x++;
	  while (val->values[y]) y++;
	  l->keywords[num] = realloc (l->keywords[num],
				      (x + y + 1) * sizeof(char**));
	  y = 0;
	  while (val->values[y++])
	      l->keywords[num][x+y-1] = val->values[y-1];
	  l->keywords[num][x+y-1] = NULL;
	  free (val->values);
        }
        free (val->keyword);
      }//while
      FREE_A (val);
      listserv_list_header_delete_buffer (buf, yyscanner);
      listserv_list_headerlex_destroy (yyscanner);
      FREE_A (yy_extra);
      char *bracket_listname_bracket = ALLOC_A (3 + strlen(listname));
      char* owner_listname = ALLOC_A (8 + strlen(listname));
      sprintf (bracket_listname_bracket, "(%s)", listname);
      sprintf (owner_listname, "Owner(%s)", listname);
      //replace (list) with Private
      //replace Owner(list) with Owner
      for (i = 0; i < LIBLISTSERV_NUM_KEYWORDS; i++)
        if (l->keywords[i]) {
          int j = 0;
	  while (l->keywords[i][j])
	    if (strcasecmp (l->keywords[i][j++], owner_listname) == 0) {
  	        free(l->keywords[i][j-1]);
	        l->keywords[i][j-1] = strdup ("Owner");
	    } else
              if (strcasecmp (l->keywords[i][j-1],
			      bracket_listname_bracket) == 0) {
	        free(l->keywords[i][j-1]);
	        l->keywords[i][j-1] = strdup ("Private");
	      }
            }
      FREE_A (bracket_listname_bracket);
      FREE_A (owner_listname);
      if (l->keywords[50]) { // SizeLim was set
          int factor, sizelim_value_len = strlen(l->keywords[50][0]);
          switch (l->keywords[50][0][sizelim_value_len - 1]) {
              case 'K':
	      case 'k':
	          factor = 1024;
                  l->keywords[50][0][sizelim_value_len -1] = '\0';
	          break;
	      case 'M':
	      case 'm':
	          factor = 1048576;
                  l->keywords[50][0][sizelim_value_len -1] = '\0';
	          break;
              default:
	          factor = 80;
          }
          int new_size = atol (l->keywords[50][0]) * factor;
          free (l->keywords[50][0]);
          l->keywords[50][0] = malloc(15);
          sprintf (l->keywords[50][0], "%i", new_size);
      }//if Sizelim
  };
  int num = listserv_num_keyword (cs (keyword));
  if (num == 0)
      return l->keywords[LIBLISTSERV_NUM_KEYWORDS];
  if (l->keywords[num-1])
      return l->keywords[num-1];
  else
      return listserv_keyword_default_value (l, listname, num);
}
