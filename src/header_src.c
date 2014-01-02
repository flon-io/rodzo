
//
// Copyright (c) 2013-2014, John Mettraux, jmettraux@gmail.com
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
  int dorun;
  int parentnumber;
  int nodenumber;
  char type;
  char *fname;
  int lstart;
  int ltstart;
  int llength;
  char **stack;
  rdz_func *func;
} rdz_node;

int rdz_node_count = 0;
rdz_node **rdz_nodes = NULL;

typedef struct rdz_result {
  int success;
  char *message;
  char *context;
  char *title;
  int itnumber;
  size_t stackc;
  int lnumber;
  int ltnumber;
} rdz_result;

rdz_result *rdz_result_malloc(
  int success, char *msg, int itnumber, int lnumber, int ltnumber
)
{
  rdz_node *n = rdz_nodes[itnumber];

  size_t sc; for (sc = 0; ; sc++) { if (n->stack[sc] == NULL) break; }
  char **ss = calloc(sc + 1, sizeof(char *));
  for (size_t i = 0; i < sc; i++) ss[i] = rdz_strdup(n->stack[i]);

  char *context = calloc((sc - 1) * 160, sizeof(char));
  char *title = calloc(sc * 160, sizeof(char));
  char *t = title;

  for (int i = 0; i < sc; i++) {
    strcpy(t, n->stack[i]); t += strlen(n->stack[i]);
    if (i == sc - 2) strcpy(context, title);
    if (i < sc - 1) *(t++) = ' ';
  }

  rdz_result *r = malloc(sizeof(rdz_result));
  r->success = success;
  r->message = msg;
  r->context = context;
  r->title = title;
  r->itnumber = itnumber;
  r->stackc = sc;
  r->lnumber = lnumber;
  r->ltnumber = ltnumber;

  return r;
}

void rdz_result_free(rdz_result *r)
{
  free(r->context);
  free(r->title);

  free(r);
}

int *rdz_lines = NULL;
char *rdz_example = NULL;

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

void rdz_print_level(int nodenumber, int min)
{
  if (nodenumber < 0) return;
  if (nodenumber < min) return;

  rdz_node *n = rdz_nodes[nodenumber];
  if (n->stack == NULL) return;

  rdz_print_level(n->parentnumber, min);

  size_t i; for (i = 0; n->stack[i] != NULL; i++)
  for (size_t ii = 0; ii < i; ii++) printf("  ");

  printf("%s", n->stack[i - 1]);
  printf(" (%d)", n->ltstart);
  printf("\n");
}
void rdz_print_context(rdz_result *p, rdz_result *r)
{
  if (r == NULL) return;

  rdz_node *rit = rdz_nodes[r->itnumber];
  rdz_print_level(rit->parentnumber, p != NULL ? p->itnumber : -1);
}

void rdz_do_print_result(rdz_result *r)
{
  if (r == NULL) return;

  rdz_node *rit = rdz_nodes[r->itnumber];

  for (int ii = 0; ii < r->stackc - 1; ii++) printf("  "); // indent
  if (r->success) rdz_green(); else rdz_red();
  printf("%s", rit->stack[r->stackc - 1]);
  if ( ! r->success) printf(" (FAILED)");
  rdz_clear();
  printf(" (%d)", r->ltnumber);
  printf("\n");
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

void rdz_record(int success, char *msg, int itnumber, int lnumber, int ltnumber)
{
  rdz_result *result =
    rdz_result_malloc(success, msg, itnumber, lnumber, ltnumber);

  rdz_do_record(result);

  rdz_results[rdz_count++] = result;
  if ( ! success) rdz_fail_count++;
}

void rdz_extract_arguments()
{
  // E=example

  rdz_example = getenv("E");

  rdz_lines = malloc(64 * sizeof(int));
  for (size_t i = 0; i < 64; i++) rdz_lines[i] = -1;

  // L=12,67

  char *l = getenv("L");

  if (l == NULL) return;

  for (size_t i = 0; ; i++)
  {
    char *c = strpbrk(l, ",");
    if (c != NULL) *c = '\0';
    rdz_lines[i] = atoi(l);
    if (c == NULL) break;
    l = c + 1;
  }
}

int rdz_run_children(rdz_node *n)
{
  for (size_t i = 0; i < rdz_node_count; i++)
  {
    rdz_node *cn = rdz_nodes[i];
    if (cn->parentnumber != n->nodenumber) continue;
    char ct = cn->type;
    if (ct != 'd' && ct != 'c' && ct != 'i') continue;
    cn->dorun = 1;
    if (ct != 'i') rdz_run_children(cn);
  }
}

int rdz_determine_dorun(rdz_node *n)
{
  char t = n->type;

  if (t == 'B' || t == 'b' || t == 'A' || t == 'a') return 0;

  if (t == 'G' || t == 'g')
  {
    n->dorun = 1;
  }
  else
  {
    // L

    if (rdz_lines[0] == -1) n->dorun = 1;

    for (size_t i = 0; n->dorun == 0 && rdz_lines[i] > -1; i++)
    {
      int l = rdz_lines[i];
      if (l >= n->ltstart && l <= n->ltstart + n->llength) n->dorun = 1;
    }

    // E

    if (t == 'i') // only match "it" instances
    {
      // TODO
    }
  }

  if ( ! n->dorun) return 0;
  if (t == 'i') return 1;

  int r = 0;

  for (size_t i = 0; i < rdz_node_count; i++)
  {
    rdz_node *cn = rdz_nodes[i];
    if (cn->parentnumber != n->nodenumber) continue;
    int rr = rdz_determine_dorun(cn);
    r = r || rr;
  }

  if (r == 0) rdz_run_children(n); // force all children to run

  return 1;
}

void rdz_dorun(rdz_node *n)
{
  if ( ! n->dorun) return;

  char t = n->type;

  if (t == 'i')
  {
    n->func();
  }
  else if (t == 'G' || t == 'g' || t == 'd' || t == 'c')
  {
    for (size_t i = 0; i < rdz_node_count; i++) // before all
    {
      rdz_node *nn = rdz_nodes[i];
      if (nn->type != 'B') continue;
      if (nn->parentnumber != n->nodenumber) continue;
      nn->func();
    }
    for (size_t i = 0; i < rdz_node_count; i++) // children
    {
      rdz_node *nn = rdz_nodes[i];
      if (nn->parentnumber != n->nodenumber) continue;
      if (nn->type != 'd' && nn->type != 'c' && nn->type != 'i') continue;
      rdz_dorun(nn);
    }
    for (size_t i = 0; i < rdz_node_count; i++) // after all
    {
      rdz_node *nn = rdz_nodes[i];
      if (nn->type != 'A') continue;
      if (nn->parentnumber != n->nodenumber) continue;
      nn->func();
    }
  }
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

  for (size_t i = 0, j = 0; i < rdz_count; i++)
  {
    rdz_result *r = rdz_results[i];
    rdz_node *rit = rdz_nodes[r->itnumber];

    if (r->success) continue;

    char *line = rdz_read_line(rit->fname, r->lnumber);
    printf("  %d) %s\n", ++j, r->title);
    printf("     >");
    rdz_red(); printf("%s", line); rdz_clear();
    printf("<\n");
    rdz_cyan(); printf("     # %s:%d", rit->fname, r->lnumber); rdz_clear();
    printf(" (%d)\n", r->ltnumber);
    free(line);
  }

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

    for (size_t i = 0; i < rdz_count; i++)
    {
      rdz_result *r = rdz_results[i];

      if (r->success) continue;

      rdz_red(); printf("make spec L=%d", r->ltnumber);
      rdz_cyan(); printf(" # %s\n", r->title);
      rdz_clear();
    }

    printf("\n");
  }

  for (size_t i = 0; i < rdz_count; i++) rdz_result_free(rdz_results[i]);
  free(rdz_results);
}

