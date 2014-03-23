
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
#include <unistd.h> // for isatty()

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
  int nodenumber;
  int parentnumber;
  int *children;
  int indent;
  char type;
  char *fname;
  int lstart;
  int ltstart;
  int llength;
  char **stack;
  rdz_func *func;
} rdz_node;

rdz_node **rdz_nodes = NULL;

//void rdz_print_node(rdz_node *n)
//{
//  for (int i = 1; i < n->indent; i++) printf("  ");
//
//  char *s = NULL;
//  if (n->stack) for (size_t i = 0; n->stack[i] != NULL; i++) s = n->stack[i];
//
//  printf(
//    "n:%d %c lts:%d \"%s\" dr:%d\n",
//    n->nodenumber, n->type, n->ltstart, s, n->dorun
//  );
//}

typedef struct rdz_result {
  int success;
  char *message;
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

  char *title = calloc(sc * 160, sizeof(char));
  char *t = title;

  for (int i = 0; i < sc; i++) {
    strcpy(t, n->stack[i]); t += strlen(n->stack[i]);
    if (i < sc - 1) *(t++) = ' ';
  }

  rdz_result *r = malloc(sizeof(rdz_result));
  r->success = success;
  r->message = msg;
  r->title = title;
  r->itnumber = itnumber;
  r->stackc = sc;
  r->lnumber = lnumber;
  r->ltnumber = ltnumber;

  return r;
}

void rdz_result_free(rdz_result *r)
{
  free(r->message);
  free(r->title);

  free(r);
}

int *rdz_lines = NULL;
char *rdz_example = NULL;

int rdz_count = 0;
int rdz_fail_count = 0;
int rdz_pending_count = 0;
rdz_result **rdz_results = NULL;

void rdz_red() { if (isatty(1)) printf("[31m"); }
void rdz_green() { if (isatty(1)) printf("[32m"); }
void rdz_yellow() { if (isatty(1)) printf("[33m"); }
//void rdz_blue() { if (isatty(1)) printf("[34m"); }
//void rdz_magenta() { if (isatty(1)) printf("[35m"); }
void rdz_cyan() { if (isatty(1)) printf("[36m"); }
//void rdz_white() { if (isatty(1)) printf("[37m"); }
void rdz_clear() { if (isatty(1)) printf("[0m"); }

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

  if (r->success == -1) rdz_yellow();
  else if (r->success == 0) rdz_red();
  else rdz_green();

  printf("%s", rit->stack[r->stackc - 1]);

  if (r->success == -1) printf(" (PENDING)");
  if (r->success == 0) printf(" (FAILED)");

  rdz_clear();

  printf(" (%d)", r->ltnumber);
  printf("\n");
}

void rdz_print_result(rdz_result *p, rdz_result *r)
{
  if (r != NULL && (p == NULL || p->itnumber == r->itnumber)) return;

  rdz_do_print_result(p);
}

char *rdz_string_eq(char *operator, char *result, char *expected)
{
  if (expected == NULL && result == NULL) return NULL;

  if (expected == NULL) return rdz_strdup("     expected NULL");
  if (result == NULL) return rdz_strdup("     result is NULL");

  if (strcmp(result, expected) == 0) return NULL;

  size_t le = strlen(expected);
  size_t lr = strlen(result);

  char *s = calloc(lr + le + 15 + 17 + 1 + 1, sizeof(char));

  strcpy(s, "     expected \"");
  strcpy(s + 15, expected);
  strcpy(s + 15 + le, "\"\n          got \"");
  strcpy(s + 15 + le + 17, result);
  strcpy(s + 15 + le + 17 + lr, "\"");

  return s;
}

char *rdz_string_neq(char *operator, char *result, char *not_expected)
{
  if (strcmp(result, not_expected) != 0) return NULL;

  size_t lne = strlen(not_expected);

  char *s = calloc(lne + 20 + 1 + 1, sizeof(char));

  strcpy(s, "     didn't expect \"");
  strcpy(s + 20, not_expected);
  strcpy(s + 20 + lne, "\"");

  return s;
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

  if (success == -1) rdz_pending_count++;
  if (success == 0) rdz_fail_count++;
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

void rdz_run_all_children(rdz_node *n)
{
  for (size_t i = 0; n->children[i] > -1; i++)
  {
    rdz_node *cn = rdz_nodes[n->children[i]];
    char ct = cn->type;
    if (ct != 'd' && ct != 'c' && ct != 'i') continue;
    if (cn->dorun < 1) cn->dorun = 1;
    if (ct != 'i') rdz_run_all_children(cn);
  }
}
void rdz_run_all_parents(int parentnumber)
{
  if (parentnumber < 0) return;

  rdz_node *pn = rdz_nodes[parentnumber];
  if (pn->dorun < 1) pn->dorun = 1;

  rdz_run_all_parents(pn->parentnumber);
}

int rdz_determine_dorun_l(rdz_node *n)
{
  for (size_t i = 0; rdz_lines[i] > -1; i++)
  {
    int l = rdz_lines[i];
    if (l >= n->ltstart && l <= n->ltstart + n->llength) return 1;
  }

  return 0;
}

int rdz_determine_dorun_e(rdz_node *n)
{
  char *s = NULL;
  for (size_t i = 0; n->stack[i] != NULL; i++) s = n->stack[i];

  return (strstr(s, rdz_example) != NULL);
}

void rdz_determine_dorun()
{
  // first pass, determine if a node should get run on its own

  for (size_t i = 0; rdz_nodes[i] != NULL; i++)
  {
    rdz_node *n = rdz_nodes[i];
    char t = n->type;

    //if (t == 'B' || t == 'b' || t == 'A' || t == 'a') continue;
    if (t == 'G' || t == 'g') n->dorun = 1;
    if (t != 'd' && t != 'c' && t != 'i') continue;

    int rl = -1;
    int re = -1;
    if (rdz_example != NULL) re = rdz_determine_dorun_e(n);
    if (rdz_lines[0] > -1) rl = rdz_determine_dorun_l(n);

    if (rl < 0 && re < 0) n->dorun = 1;
    if (rl > 0) n->dorun = 2; // all children if they're all 0
    if (re > 0) n->dorun = 3; // ancestors and all children
  }

  // second pass, ancestors and children are brought in

  for (size_t i = 0; rdz_nodes[i] != NULL; i++)
  {
    rdz_node *n = rdz_nodes[i];

    if (n->dorun < 2) continue;

    int run_parents = 0;
    int run_children = 0;

    if (n->dorun == 2)
    {
      run_children = 1;

      for (size_t j = 0; n->children[j] > -1; j++)
      {
        if (rdz_nodes[n->children[j]]->dorun > 1) run_children = 0;
      }
    }
    else //if (n->dorun == 3)
    {
      run_parents = 1;
      run_children = 1;
    }

    if (run_parents) rdz_run_all_parents(n->parentnumber);
    if (run_children) rdz_run_all_children(n);
  }
}

void rdz_run_offlines(int nodenumber, char type)
{
  if (nodenumber == -1) return;
  rdz_node *n = rdz_nodes[nodenumber];

  // before each offline
  if (type == 'y') rdz_run_offlines(n->parentnumber, type);

  for (size_t i = 0; n->children[i] > -1; i++)
  {
    rdz_node *nn = rdz_nodes[n->children[i]];
    if (nn->type == type) nn->func();
  }

  // after each offline
  if (type == 'z') rdz_run_offlines(n->parentnumber, type);
}

void rdz_run_pending(int nodenumber)
{
  rdz_node *n = rdz_nodes[nodenumber];

  char *s = NULL;
  //if (n->stack) for (size_t i = 0; n->stack[i] != NULL; i++) s = n->stack[i];
  // TODO: use stackc... or find the text... probably in rodzo.c

  rdz_record(-1, s, n->parentnumber, n->lstart, n->ltstart);
}

void rdz_dorun(rdz_node *n)
{
  if ( ! n->dorun) return;

  char t = n->type;

  if (t == 'i')
  {
    if (n->children[0] > -1)
    {
      return rdz_run_pending(n->children[0]);
    }

    int rc = rdz_count;

    n->func();

    if (rdz_count == rc)
    {
      rdz_record(1, NULL, n->nodenumber, n->lstart, n->ltstart);
    }
  }
  else if (t == 'G' || t == 'g' || t == 'd' || t == 'c')
  {
    for (size_t i = 0; n->children[i] > -1; i++) // before all
    {
      rdz_node *nn = rdz_nodes[n->children[i]];
      if (nn->type == 'B') nn->func();
    }
    for (size_t i = 0; n->children[i] > -1; i++) // children
    {
      rdz_node *nn = rdz_nodes[n->children[i]];
      if (nn->type != 'd' && nn->type != 'c' && nn->type != 'i') continue;
      rdz_run_offlines(n->nodenumber, 'y'); // before each offline
      rdz_dorun(nn);
      rdz_run_offlines(n->nodenumber, 'z'); // after each offline
    }
    for (size_t i = 0; n->children[i] > -1; i++) // after all
    {
      rdz_node *nn = rdz_nodes[n->children[i]];
      if (nn->type == 'A') nn->func();
    }
  }
}

char *rdz_read_line(char *fname, int lnumber)
{
  char *l = calloc(1024, sizeof(char));
  char *ll = l;

  FILE *in = fopen(fname, "r");

  if (in == NULL)
  {
    fprintf(stderr, "cannot find source file %s\n", fname);
    exit(1);
  }

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

  if (rdz_pending_count > 0)
  {
    printf("Pending:\n");

    for (size_t i = 0, j = 0; i < rdz_count; i++)
    {
      rdz_result *r = rdz_results[i];

      if (r->success != -1) continue;

      rdz_node *rit = rdz_nodes[r->itnumber];

      rdz_yellow(); printf("  %s\n", r->title); rdz_clear();
      rdz_cyan(); printf("   # %s\n", r->message); rdz_clear();
      rdz_cyan(); printf("   # %s:%d\n", rit->fname, r->lnumber); rdz_clear();

      //char *line = rdz_read_line(rit->fname, r->lnumber);
      //printf("  %zu) %s\n", ++j, r->title);
      //if (r->message) { rdz_red(); puts(r->message); rdz_clear(); }
      //printf("     >");
      //rdz_red(); printf("%s", line); rdz_clear();
      //printf("<\n");
      //rdz_cyan(); printf("     # %s:%d", rit->fname, r->lnumber); rdz_clear();
      //printf(" (%d)\n", r->ltnumber);
      //free(line);
    }

    printf("\n");
  }

  if (rdz_fail_count > 0)
  {
    printf("Failures:\n\n");

    for (size_t i = 0, j = 0; i < rdz_count; i++)
    {
      rdz_result *r = rdz_results[i];

      if (r->success != 0) continue;

      rdz_node *rit = rdz_nodes[r->itnumber];

      char *line = rdz_read_line(rit->fname, r->lnumber);
      printf("  %zu) %s\n", ++j, r->title);
      if (r->message) { rdz_red(); puts(r->message); rdz_clear(); }
      printf("     >");
      rdz_red(); printf("%s", line); rdz_clear();
      printf("<\n");
      rdz_cyan(); printf("     # %s:%d", rit->fname, r->lnumber); rdz_clear();
      printf(" (%d)\n", r->ltnumber);
      free(line);
    }
  }

  printf("\n");
  if (rdz_fail_count > 0) rdz_red(); else rdz_green();
  printf("%d examples, ", itcount);
  printf("%d tests seen, ", rdz_count);
  printf("%d failures", rdz_fail_count);
  if (rdz_pending_count > 0) printf(", %d pending", rdz_pending_count);
  printf("\n");
  rdz_clear();
  printf("\n");

  if (rdz_fail_count > 0)
  {
    printf("Failed examples:\n\n");

    for (size_t i = 0; i < rdz_count; i++)
    {
      rdz_result *r = rdz_results[i];

      if (r->success != 0) continue;

      rdz_red(); printf("make spec L=%d", r->ltnumber);
      rdz_cyan(); printf(" # %s\n", r->title);
      rdz_clear();
    }

    printf("\n");
  }
}

