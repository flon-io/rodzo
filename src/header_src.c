
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
#include <regex.h>
#include <wordexp.h>

  // avoiding strdup and the posix_source requirement...
char *rdz_strdup(char *s)
{
  int l = strlen(s);
  char *r = calloc(l + 1, sizeof(char));
  strcpy(r, s);
  return r;
}
char *rdz_strndup(char *s, size_t n)
{
  char *r = calloc(n + 1, sizeof(char));
  for (size_t i = 0; i < n; i++) r[i] = s[i];
  return r;
}

typedef int rdz_func();

typedef struct rdz_node {
  int dorun;
  int nodenumber;
  int parentnumber;
  size_t depth;
  int *children;
  //int indent;
  char type;
  char *fname;
  int lstart;
  int ltstart;
  int llength;
  char *text;
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
  int lnumber;
  int ltnumber;
} rdz_result;

char *rdz_determine_title(rdz_node *n)
{
  char **texts = calloc(128, sizeof(char *));
  rdz_node *nn = n;
  size_t i = 0;
  size_t l = 0;

  while (1)
  {
    texts[i++] = nn->text;
    l = l + strlen(nn->text) + 1;
    if (nn->parentnumber < 1) break;
    nn = rdz_nodes[nn->parentnumber];
  }

  char *title = calloc(l + 1, sizeof(char));
  char *t = title;

  for (size_t ii = i - 1; ; ii--)
  {
    l = strlen(texts[ii]);
    strncpy(t, texts[ii], l);
    t = t + l;
    strcpy(t, " ");
    t = t + 1;
    if (ii == 0) break;
  }

  free(texts);

  return title;
}

rdz_result *rdz_result_malloc(
  int success, char *msg, int itnumber, int lnumber, int ltnumber)
{
  rdz_node *n = rdz_nodes[itnumber];

  rdz_result *r = calloc(1, sizeof(rdz_result));
  r->success = success;
  r->message = msg;
  r->title = rdz_determine_title(n);
  r->itnumber = itnumber;
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

#define RDZ_LINES_MAX 16
int *rdz_lines = NULL;
char *rdz_example = NULL;
char **rdz_files = NULL;

int rdz_count = 0;
int rdz_fail_count = 0;
int rdz_pending_count = 0;
rdz_result **rdz_results = NULL;

// PS1="\[\033[1;34m\][\$(date +%H%M)][\u@\h:\w]$\[\033[0m\] "
//
// Black       0;30     Dark Gray     1;30
// Blue        0;34     Light Blue    1;34
// Green       0;32     Light Green   1;32
// Cyan        0;36     Light Cyan    1;36
// Red         0;31     Light Red     1;31
// Purple      0;35     Light Purple  1;35
// Brown       0;33     Yellow        1;33
// Light Gray  0;37     White         1;37

void rdz_red() { if (isatty(1)) printf("[0;31m"); }
void rdz_green() { if (isatty(1)) printf("[0;32m"); }
void rdz_yellow() { if (isatty(1)) printf("[0;33m"); }
void rdz_cyan() { if (isatty(1)) printf("[0;36m"); }
void rdz_clear() { if (isatty(1)) printf("[0;0m"); }

void rdz_print_level(int nodenumber)//, int min)
{
  if (nodenumber < 0) return;

  rdz_node *n = rdz_nodes[nodenumber];
  if (n->depth == 0) return;

  for (int i = 0; i < n->depth - 1; i++) printf("  "); // indent

  printf("%s", n->text);
  printf(" (%d)", n->ltstart);
  printf("\n");
}

void rdz_print_result(rdz_result *r)
{
  if (r == NULL) return;

  rdz_node *rit = rdz_nodes[r->itnumber];

  for (int i = 0; i < rit->depth - 1; i++) printf("  "); // indent

  if (r->success == -1) rdz_yellow();
  else if (r->success == 0) rdz_red();
  else rdz_green();

  printf("%s", rit->text);

  if (r->success == -1) printf(" (PENDING: %s)", r->message);
  if (r->success == 0) printf(" (FAILED)");

  rdz_clear();

  printf(" (%d)", r->ltnumber);
  printf("\n");
}

char *rdz_string_expected(char *result, char *verb, char *expected)
{
  size_t l = strlen(verb); if (l < 8) l = 8;

  char *s = calloc(2048, sizeof(char));

  snprintf(s, 2048, ""
    "     %*s \"%s\"\n"
    "     %*s \"%s\"",
    l, "expected", result, l, verb, expected);

  return s;
}

char *rdz_string_eq(char *operator, char *result, char *expected)
{
  if (expected == NULL && result == NULL) return NULL;

  if (expected == NULL) return rdz_strdup("     expected NULL");
  if (result == NULL) return rdz_strdup("     result is NULL");

  if (strcmp(result, expected) == 0) return NULL;

  return rdz_string_expected(result, "got", expected);
}

char *rdz_string_neq(char *operator, char *result, char *not_expected)
{
  if (strcmp(result, not_expected) != 0) return NULL;

  size_t l = strlen(not_expected) + 22;

  char *s = calloc(l, sizeof(char));
  snprintf(s, l, "     didn't expect \"%s\"", not_expected);

  return s;
}

char *rdz_string_match(char *operator, char *result, char *expected)
{
  char *s = NULL;
  regex_t *r = calloc(1, sizeof(regex_t));
  regcomp(r, expected, REG_EXTENDED);
  regmatch_t ms[0];
  if (regexec(r, result, 0, ms, 0)) // no match
  {
    s = rdz_string_expected(result, "to match", expected);
  }
  regfree(r); free(r);
  return s;
}

char *rdz_string_start(char *operator, char *result, char *expected)
{
  if (strncmp(result, expected, strlen(expected)) == 0)
    return NULL;

  char *start = rdz_strndup(result, 49);
  char *s = rdz_string_expected(start, "to start with", expected);
  free(start);

  return s;
}

char *rdz_string_end(char *operator, char *result, char *expected)
{
  if (strcmp(result + strlen(result) - strlen(expected), expected) == 0)
    return NULL;

  return rdz_string_expected(result, "to end with", expected);
}

void rdz_record(int success, char *msg, int itnumber, int lnumber, int ltnumber)
{
  rdz_result *result =
    rdz_result_malloc(success, msg, itnumber, lnumber, ltnumber);

  rdz_results[rdz_count++] = result;

  if (success == -1) rdz_pending_count++;
  if (success == 0) rdz_fail_count++;
}

void rdz_extract_arguments()
{
  // E=example

  rdz_example = getenv("E");

  // L=12,67

  char *l = getenv("L");

  if (l != NULL)
  {
    rdz_lines = calloc(RDZ_LINES_MAX, sizeof(int));

    for (size_t i = 0; i < RDZ_LINES_MAX; i++)
    {
      if (l == NULL) { rdz_lines[i] = -1; continue; }

      char *c = strpbrk(l, ",");
      rdz_lines[i] = atoi(l);

      if (c == NULL) { l = NULL; continue; }

      l = c + 1;
    }
  }

  // F=fname

  char *f = getenv("F");

  if (f != NULL)
  {
    char *ff = calloc(strlen(f) + 9, sizeof(char));
    sprintf(ff, "../spec/%s", f);

    wordexp_t we;
    wordexp(ff, &we, 0);

    rdz_files = calloc(we.we_wordc + 1, sizeof(char *));

    for (int i = 0; i < we.we_wordc; i++)
    {
      rdz_files[i] = rdz_strdup(we.we_wordv[i]);
    }

    wordfree(&we);
    free(ff);
  }
}

void rdz_run_all_children(rdz_node *n)
{
  for (size_t i = 0; n->children[i] > -1; i++)
  {
    rdz_node *cn = rdz_nodes[n->children[i]];
    char ct = cn->type;
    if (ct != 'd' && ct != 'c' && ct != 'i' && ct != 'p') continue;
    if (cn->dorun < 1) cn->dorun = 1;
    if (ct != 'i' || ct != 'p') rdz_run_all_children(cn);
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
  if (rdz_lines == NULL) return -1;

  for (size_t i = 0; rdz_lines[i] > -1; i++)
  {
    int l = rdz_lines[i];
    if (l >= n->ltstart && l <= n->ltstart + n->llength) return 1;
  }

  return 0;
}

int rdz_determine_dorun_e(rdz_node *n)
{
  if (rdz_example == NULL) return -1;
  return (strstr(n->text, rdz_example) != NULL);
}

int rdz_determine_dorun_f(rdz_node *n)
{
  if (rdz_files == NULL) return -1;

  for (size_t i = 0; ; i++)
  {
    char *fn = rdz_files[i];
    if (fn == NULL) break;
    if (strcmp(fn, n->fname) == 0) return 1;
  }

  return 0;
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
    if (t != 'd' && t != 'c' && t != 'i' && t != 'p') continue;

    int re = rdz_determine_dorun_e(n);
    int rl = rdz_determine_dorun_l(n);
    int rf = rdz_determine_dorun_f(n);

    if (rl < 0 && re < 0 && rf < 0) n->dorun = 1;
    if (rf > 0) n->dorun = 1;
    if (rl > 0) n->dorun = 2; // all children if they're all 0
    if (re > 0) n->dorun = 3; // ancestors and all children

    //printf(
    //  "%zu) re: %d, rl: %d, rf: %d n->dorun: %d\n",
    //  i, re, rl, rf, n->dorun);
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

void rdz_dorun(rdz_node *n)
{
  if (n->nodenumber == 0) printf("\n"); // initial blank line

  if ( ! n->dorun) return;

  char t = n->type;

  if (t == 'i')
  {
    if (n->children[0] > -1) return rdz_dorun(rdz_nodes[n->children[0]]);

    int rc = rdz_count;

    n->func(); // run the "it"

    if (rdz_count == rc) // no ensure in the example, record a success...
    {
      rdz_record(1, rdz_strdup(n->text), n->nodenumber, n->lstart, n->ltstart);
    }

    rdz_print_result(rdz_results[rdz_count - 1]);
  }
  else if (t == 'p')
  {
    rdz_record(-1, rdz_strdup(n->text), n->parentnumber, n->lstart, n->ltstart);

    rdz_print_result(rdz_results[rdz_count - 1]);
  }
  else if (t == 'G' || t == 'g' || t == 'd' || t == 'c')
  {
    rdz_print_level(n->nodenumber);
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
  printf("\n");

  if (rdz_pending_count > 0)
  {
    printf("Pending:\n");

    for (size_t i = 0; i < rdz_count; i++)
    {
      rdz_result *r = rdz_results[i];

      if (r->success != -1) continue;

      rdz_node *rit = rdz_nodes[r->itnumber];

      rdz_yellow(); printf("  %s\n", r->title); rdz_clear();
      rdz_cyan(); printf("   # %s\n", r->message); rdz_clear();
      rdz_cyan(); printf("   # %s:%d\n", rit->fname, r->lnumber); rdz_clear();
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
  printf("%d tests seen, ", rdz_count - rdz_pending_count);
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

