

  /*
   * rodzo header
   */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char *rdz_strdup(char *s)
{
  int l = strlen(s);
  char *r = calloc(l + 1, sizeof(char));
  strcpy(r, s);
  return r;
}

typedef struct rdz_result {
  int success;
  int stackc;
  char **stack;
  char *context;
  char *title;
  int itnumber;
  char *fname;
  int lnumber;
} rdz_result;

rdz_result *rdz_malloc_result(
  int success, int sc, char *s[], int itnumber, char *fname, int lnumber
)
{
  char *context = calloc((sc - 1) * 160, sizeof(char));
  char *title = calloc(sc * 160, sizeof(char));
  char *t = title;

  for (int i = 0; i < sc; i++) {
    strcpy(t, s[i]); t += strlen(s[i]);
    if (i == sc - 2) strcpy(context, title);
    if (i < sc - 1) *(t++) = ' ';
  }

  rdz_result *r = malloc(sizeof(rdz_result));
  r->success = success;
  r->stackc = sc;
  r->stack = s;
  r->context = context;
  r->title = title;
  r->itnumber = itnumber;
  r->fname = rdz_strdup(fname);
  r->lnumber = lnumber;

  return r;
}

void rdz_free_result(rdz_result *r)
{
  free(r->context);
  free(r->title);
  free(r->fname);
  free(r);
}

int rdz_count = 0;
int rdz_fail_count = 0;
rdz_result **rdz_results = NULL;

void rdz_red() { printf("[31m"); }
void rdz_green() { printf("[32m"); }
void rdz_cyan() { printf("[36m"); }
void rdz_clear() { printf("[0m"); }

void rdz_print_context(rdz_result *r)
{
  for (int i = 0; i < r->stackc - 1; i++)
  {
    for (int ii = 0; ii < i; ii++) printf("  "); // indent
    printf("%s\n", r->stack[i]);
  }
}

void rdz_print_result(rdz_result *r)
{
  for (int ii = 0; ii < r->stackc - 1; ii++) printf("  "); // indent
  if (r->success) rdz_green(); else rdz_red();
  printf("%s", r->stack[r->stackc - 1]);
  if ( ! r->success) printf(" (FAILED)");
  printf("\n");
  rdz_clear();
}

void rdz_do_record(rdz_result *r)
{
  rdz_result *prev = NULL;
  if (rdz_count > 0) prev = rdz_results[rdz_count - 1];

  if (prev == NULL) printf("\n"); // initial blank line

  if (r != NULL && (prev == NULL || strcmp(prev->context, r->context) != 0))
  {
    rdz_print_context(r);
  }

  if (prev == NULL) return;

  if (r == NULL || prev->itnumber < r->itnumber)
  {
    rdz_print_result(prev);
  }
}

void rdz_record(
  int success, int sc, char *s[], int itnumber, char *fname, int lnumber
)
{
  rdz_result *result =
    rdz_malloc_result(success, sc, s, itnumber, fname, lnumber);

  rdz_do_record(result);

  rdz_results[rdz_count++] = result;
  if ( ! success) rdz_fail_count++;
}

char *rdz_read_line(char *fname, int lnumber)
{
  char *l = calloc(1024, sizeof(char));
  char *ll = l;
  FILE *in = fopen(fname, "r");
  while(1)
  {
    if (lnumber < 1) break;
    char c = fgetc(in);
    if (c == EOF) break;
    if (c == '\n') { lnumber--; continue; }
    if (lnumber == 1) *(ll++) = c;
  }
  fclose(in);
  return l;
}

void rdz_summary(int itcount)
{
  rdz_do_record(NULL);

  printf("\n");

  if (rdz_fail_count > 0)
  {
    printf("Failures:\n\n");
  }

  for (int i = 0, j = 0; i < rdz_count; i++)
  {
    rdz_result *r = rdz_results[i];

    if ( ! r->success)
    {
      char *line = rdz_read_line(r->fname, r->lnumber);
      printf("  %d) %s\n", ++j, r->title);
      printf("     >");
      rdz_red(); printf("%s", line); rdz_clear();
      printf("<\n");
      rdz_cyan(); printf("     # %s:%d\n", r->fname, r->lnumber); rdz_clear();
      free(line);
    }

    rdz_free_result(r);
  }
  free(rdz_results);

  printf("\n");
  if (rdz_fail_count > 0) rdz_red(); else rdz_green();
  printf("%d examples, ", itcount);
  printf("%d tests seen, ", rdz_count);
  printf("%d failures\n", rdz_fail_count);
  rdz_clear();
  printf("\n");

  if (rdz_fail_count > 0)
  {
    printf("Failed examples:\n\n");
    rdz_red(); printf("TODO (maybe)\n\n"); rdz_clear();
  }
}


  /*
   * ../spec/node_spec.c
   */

//
// specifying aabro
//
// Wed Dec 18 16:19:13 JST 2013
//

#include "aabro.h"



int sc_1 = 3;
char *s_1[] = { "abr_node", "abr_malloc_node()", "creates a node" };
char *fn_1 = "../spec/node_spec.c";
int test_1()
    {
  int r0 = (1 == 2);
    rdz_record(r0, sc_1, s_1, 1, fn_1, 17);
    if ( ! r0) return 0;
  return 1;
    }

int sc_2 = 4;
char *s_2[] = { "abr_node", "abr_malloc_node()", "abr_node_to_string(n)", "returns a string representation of the node" };
char *fn_2 = "../spec/node_spec.c";
int test_2()
    {
      //abr_node *n = abr_malloc_node("nada", 0);
      //ensure(strcmp("[ nada, 0 ]", abr_node_to_string(n)) == 0);
  int r1 = (4 == 5);
    rdz_record(r1, sc_2, s_2, 2, fn_2, 26);
    if ( ! r1) return 0;
  return 1;
    }


  /*
   * ../spec/parse_string_spec.c
   */

//
// specifying aabro
//
// Tue Dec 17 22:56:18 JST 2013
//

#include "aabro.h"



int sc_3 = 2;
char *s_3[] = { "abr_parse_string()", "flips burgers" };
char *fn_3 = "../spec/parse_string_spec.c";
int test_3()
  {
  int r0 = (1 == 1);
    rdz_record(r0, sc_3, s_3, 3, fn_3, 15);
    if ( ! r0) return 0;
  int r1 = (3 == 4);
    rdz_record(r1, sc_3, s_3, 3, fn_3, 16);
    if ( ! r1) return 0;
  return 1;
  }


  /*
   * rodzo footer
   */

int main(int argc, char *argv[])
{
  rdz_results = calloc(3, sizeof(rdz_result));

  test_1();
  test_2();
  test_3();

  rdz_summary(3);
}

