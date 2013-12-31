
//
// Copyright (c) 2013, John Mettraux, jmettraux@gmail.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
// Made in Japan.
//

  /*
   * rodzo header
   */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// avoiding strdup and the posix_source requirement...
char *rdz_strdup(char *s)
{
  int l = strlen(s);
  char *r = calloc(l + 1, sizeof(char));
  strcpy(r, s);
  return r;
}

typedef int rdz_func();

typedef struct rdz_node {
  int parentnumber;
  int nodenumber;
  char type;
  int lstart;
  int ltstart;
  int llength;
  char **stack;
  rdz_func *func;
} rdz_node;

typedef struct rdz_result {
  int success;
  char *message;
  int stackc;
  char **stack;
  char *context;
  char *title;
  int itnumber;
  char *fname;
  int lnumber;
  int ltnumber;
} rdz_result;

rdz_result *rdz_result_malloc(
  int success, char *msg,
  char *s[], int itnumber,
  char *fname, int lnumber, int ltnumber
)
{
  int sc; for (sc = 0; ; sc++) { if (s[sc] == NULL) break; }
  char **ss = calloc(sc + 1, sizeof(char *));
  for (int i = 0; i < sc; i++) ss[i] = rdz_strdup(s[i]);

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
  r->message = msg;
  r->stackc = sc;
  r->stack = ss;
  r->context = context;
  r->title = title;
  r->itnumber = itnumber;
  r->fname = rdz_strdup(fname);
  r->lnumber = lnumber;
  r->ltnumber = ltnumber;

  return r;
}

void rdz_result_free(rdz_result *r)
{
  free(r->context);
  free(r->title);
  free(r->fname);
  for (size_t i = 0; i < r->stackc; i++) free(r->stack[i]);
  free(r->stack);

  free(r);
}

rdz_node **rdz_nodes = NULL;

int rdz_count = 0;
int rdz_fail_count = 0;
rdz_result **rdz_results = NULL;

void rdz_red() { printf("[31m"); }
void rdz_green() { printf("[32m"); }
//void rdz_yellow() { printf("[33m"); }
//void rdz_blue() { printf("[34m"); }
//void rdz_magenta() { printf("[35m"); }
void rdz_cyan() { printf("[36m"); }
//void rdz_white() { printf("[37m"); }
void rdz_clear() { printf("[0m"); }

void rdz_print_context(rdz_result *p, rdz_result *r)
{
  if (r == NULL) return;
  if (p != NULL && r->itnumber == p->itnumber) return;

  for (int i = 0; i < r->stackc - 1; i++)
  {
    char *pt = "";
    if (p != NULL && p->stackc > i) pt = p->stack[i];
    char *rt = r->stack[i];
    if (strcmp(pt, rt) == 0) continue;
    for (int ii = 0; ii < i; ii++) printf("  "); // indent
    printf("%s\n", r->stack[i]);
  }
}

void rdz_do_print_result(rdz_result *r)
{
  if (r == NULL) return;

  for (int ii = 0; ii < r->stackc - 1; ii++) printf("  "); // indent
  if (r->success) rdz_green(); else rdz_red();
  printf("%s", r->stack[r->stackc - 1]);
  if ( ! r->success) printf(" (FAILED)");
  printf("\n");
  rdz_clear();
}

void rdz_print_result(rdz_result *p, rdz_result *r)
{
  if (r != NULL && (p == NULL || p->itnumber == r->itnumber)) return;

  rdz_do_print_result(p);
}

void rdz_do_record(rdz_result *r)
{
  rdz_result *prev = NULL;
  if (rdz_count > 0) prev = rdz_results[rdz_count - 1];

  if (prev == NULL) printf("\n"); // initial blank line

  rdz_print_result(prev, r);
  rdz_print_context(prev, r);
}

void rdz_record(
  int success, char *msg,
  char *s[], int itnumber,
  char *fname, int lnumber, int ltnumber
)
{
  rdz_result *result =
    rdz_result_malloc(success, msg, s, itnumber, fname, lnumber, ltnumber);

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
      rdz_cyan(); printf("     # %s:%d", r->fname, r->lnumber); rdz_clear();
      printf(" (%d)\n", r->ltnumber);
      free(line);
    }

    rdz_result_free(r);
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

