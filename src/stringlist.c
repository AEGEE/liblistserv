#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "src/stringlist.h"

struct listserv_stringlist *
listserv_stringlist_init ()
{
  struct listserv_stringlist *sl =
    malloc (sizeof (struct listserv_stringlist));
  sl->string_presentation = NULL;
  sl->total_length_of_elements = 0;
  sl->num_elements = 0;
  sl->size_for_elements = 1024;
  sl->elements = malloc (sl->size_for_elements * sizeof (char *));
  return sl;
}

void
listserv_stringlist_add (struct listserv_stringlist *const sl, char *text)
{
  int pos, l, text_length;
  /*
     if (text[8] == ':') {
     text_length = 9;
     while (text[text_length] != '\0'
     && text[text_length] != '|') {
     text[text_length] = toupper (text[text_length]);
     text_length++;
     }
     if (text[text_length] == '|')
     text[text_length+1] = toupper (text[text_length+1]);
     } else { */
  text_length = 0;
  while (text[text_length])
    {
      text[text_length] = tolower (text[text_length]);
      text_length++;
    }
  //  }

  if (sl->num_elements != 0)
    {
      l = 0;
      int r = sl->num_elements, diff;
      do
	{
	  pos = (l + r) / 2;
	  diff = strcmp (text, sl->elements[pos]);
	  if (diff == 0)
	    return;
	  else if (diff < 0)
	    r = pos;
	  else
	    {
	      if (l == pos)
		{
		  pos++;
		  break;
		}
	      else
		l = pos;
	    }
	}
      while (r != l);
    }
  else
    pos = 0;
  if (sl->string_presentation)
    {
      free (sl->string_presentation);
      sl->string_presentation = NULL;
    }
  sl->total_length_of_elements += text_length;
  if (sl->num_elements == sl->size_for_elements)
    {
      sl->size_for_elements += 1024;
      sl->elements = realloc (sl->elements,
			      sl->size_for_elements * sizeof (char *));
    }
  /*
  memmove(*sl->elements + sizeof(char*) * pos,
	  *sl->elements + sizeof(char*) * ( pos +1),
	  (sl->num_elements - pos) * sizeof(char*));
  */
  for (l = sl->num_elements; l > pos; l--)
    sl->elements[l] = sl->elements[l - 1];
  sl->elements[pos] = strdup (text);
  sl->num_elements++;
}

char *
listserv_stringlist_string (struct listserv_stringlist *const sl)
{
  if (sl->string_presentation)
    free (sl->string_presentation);
  if (sl->num_elements)
    {
      int ret_pos = 0, i;
      sl->string_presentation =
	malloc (sl->total_length_of_elements + sl->num_elements * 4);
      for (i = 0; i < sl->num_elements; i++)
	{
	  sl->string_presentation[ret_pos] = '\"';
	  strcpy (sl->string_presentation + ret_pos + 1, sl->elements[i]);
	  ret_pos += strlen (sl->elements[i]) + 4;
	  sl->string_presentation[ret_pos - 3] = '\"';
	  sl->string_presentation[ret_pos - 2] = ',';
	  sl->string_presentation[ret_pos - 1] = ' ';
	}
      sl->string_presentation[ret_pos - 2] = '\0';
    }
  else
    sl->string_presentation = strdup ("");
  return sl->string_presentation;
}

void
listserv_stringlist_destroy (struct listserv_stringlist *sl)
{
  if (sl->string_presentation)
    free (sl->string_presentation);
  int j;
  for (j = 0; j < sl->num_elements; j++)
    free (sl->elements[j]);
  free (sl->elements);
  free (sl);
}
