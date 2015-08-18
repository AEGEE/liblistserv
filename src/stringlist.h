struct listserv_stringlist
{
  char **elements;
  char *string_presentation;
  int num_elements;
  int total_length_of_elements;
  int size_for_elements;
};

struct listserv_stringlist *listserv_stringlist_init ();
void listserv_stringlist_add (struct listserv_stringlist *const sl, char *text);
char *listserv_stringlist_string (struct listserv_stringlist *const sl);
void listserv_stringlist_destroy (struct listserv_stringlist *sl);
