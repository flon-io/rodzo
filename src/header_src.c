
//
// Copyright (c) 2013-2015, John Mettraux, jmettraux+flon@gmail.com
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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h> // for isatty()
#include <regex.h>
#include <wordexp.h>
#include <errno.h>
#include <sys/time.h>


int rdz_hexdump_on = 0;

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

typedef double rdz_func();

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

double rdz_now()
{
  struct timeval tv;

  if (gettimeofday(&tv, NULL) != 0) return 0.0;

  return 1000.0 * tv.tv_sec + tv.tv_usec / 1000.0; // turn to ms
}

double rdz_duration(double start)
{
  return rdz_now() - start; // still ms
}

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

#define RDZ_LINES_MAX 32
int *rdz_lines = NULL;
char *rdz_example = NULL;
#define RDZ_FILES_MAX 16
char **rdz_files = NULL;
int rdz_it = -1;

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

static int istty()
{
  int rno = errno;
  int r = isatty(1);
  errno = rno;

  return r;
}

char *rdz_rd() { return istty() ? "[0;31m" : ""; }
char *rdz_gn() { return istty() ? "[0;32m" : ""; }
char *rdz_yl() { return istty() ? "[0;33m" : ""; }
char *rdz_cy() { return istty() ? "[0;36m" : ""; }
char *rdz_gr() { return istty() ? "[1;30m" : ""; }
char *rdz_bl() { return istty() ? "[1;34m" : ""; }
//char *rdz_by() { return istty() ? "[1;33m" : ""; }
char *rdz_cl() { return istty() ? "[0;0m" : ""; }

void rdz_print_level(int nodenumber)//, int min)
{
  if (nodenumber < 0) return;

  rdz_node *n = rdz_nodes[nodenumber];
  if (n->depth == 0) return;

  for (int i = 0; i < n->depth - 1; i++) printf("  "); // indent

  printf("%s", n->text);
  printf(" %sL=%d I=%d%s\n", rdz_gr(), n->ltstart, n->nodenumber, rdz_cl());
}

void rdz_duration_to_s(double duration, char *ca)
{
  *ca = 0;

  if (duration < 0.0) return;

  char *e = getenv("RDZ_NO_DURATION");
  if (e && (strcmp(e, "1") == 0 || strcmp(e, "true"))) return;

  if (duration < 1000.0) { sprintf(ca, "%fms ", duration); return; }

  double s = duration / 1000.0;
  sprintf(ca, "%.6f ", s);
  for (size_t i = 0; ; ++i) { if (ca[i] == '.') { ca[i] = 's'; break; } }
}

void rdz_print_result(rdz_result *r, double duration)
{
  if (r == NULL) return;

  rdz_node *rit = rdz_nodes[r->itnumber];

  for (int i = 0; i < rit->depth - 1; i++) printf("  "); // indent

  char *co = rdz_gn();
  if (r->success == -1) co = rdz_yl();
  else if (r->success == 0) co = rdz_rd();

  printf("%s%s", co, rit->text);

  if (r->success == -1) printf(" (PENDING: %s)", r->message);
  if (r->success == 0) printf(" (FAILED)");

  char du[77]; rdz_duration_to_s(duration, du);

  printf(
    " %s%sL=%d I=%d%s\n", rdz_gr(), du, r->ltnumber, r->itnumber, rdz_cl());
}

char *rdz_truncate(char *s, size_t l, int soe)
{
  if (s == NULL) return NULL;

  size_t ls = strlen(s);

  if (soe == 1)
  {
    char *r = calloc(ls + 3, sizeof(char));
    snprintf(r, ls + 3, "\"%s\"", s);
    return r;
  }

  int ll = l; // "%.*s" wants an int and not a size_t...

  char *r = calloc(l + 6 + 1, sizeof(char));

  if (ls <= l) snprintf(r, l + 6, "\"%s\"", s);
  else if (soe <= 0) snprintf(r, l + 6, "\"%.*s...\"", ll, s);
  else snprintf(r, l + 6, "\"...%.*s\"", ll, s + l + 1);

  return r;
}

char *rdz_string_expected(char *result, char *verb, char *expected)
{
  if (expected == NULL) verb = "to be";

  size_t l = strlen(verb); if (l < 8) l = 8;
  int ll = l; // "%.*s" wants an int and not a size_t...

  int soe = 0; if (verb[3] == 's') soe = -1; else if (verb[3] == 'e') soe = 1;

  char *res = rdz_truncate(result, 49, soe);

  char *s = calloc(4096, sizeof(char));

  if (expected)
    snprintf(
      s, 4096,
      "     %*s %s\n"
      "     %*s \"%s\"",
      ll, "expected", res, ll, verb, expected);
  else
    snprintf(
      s, 4096,
      "     %*s %s\n"
      "     %*s NULL",
      ll, "expected", res, ll, verb);

  if (res) free(res);

  return s;
}

int rdz_hexdump(const void *d, ssize_t len, size_t line, int second)
{
  char *s = (char *)d;
  ssize_t l = len;
  if (len < 0) len = strlen(s);

  size_t line_length = 15;
  size_t j = line * line_length;

  printf("%s%-10p%s ", j == 0 ? rdz_cl() : rdz_gr(), s, rdz_cl());

  size_t over = (j >= len);

  for (size_t i = j; i < j + line_length; ++i)
  {
    if (over) printf("   ");
    else if (s[i] >= 32 && s[i] <= 126) printf("%02x ", s[i]);
    else printf("%s%02x%s ", rdz_bl(), s[i], rdz_cl());
    if (l < 0) { if (s[i] == 0) over = 1; } else { if (i >= l) over = 1; }
    int m = i - j + 1; if (m == 5 || m == 10) printf(" ");
  }

  over = (j >= len);
  printf(" %s|%s", second ? rdz_cl() : rdz_bl(), rdz_cl());
  for (size_t i = j; i < j + line_length; ++i)
  {
    if (l < 0) { if (s[i] == 0) over = 1; } else { if (i >= l) over = 1; }
    if (over) printf(" ");
    else if (s[i] >= 32 && s[i] <= 126) printf("%c", s[i]);
    else printf("%s.%s", rdz_bl(), rdz_cl());
    int m = i - j + 1;
    if (m == 5 || m == 10) printf("%s-%s", rdz_gr(), rdz_cl());
  }
  printf("%s|%s\n", second ? rdz_cl() : rdz_bl(), rdz_cl());

  if (over) return 1; // over

  return 0;
}

void rdz_hexshow(const char *a, const char *b, ssize_t n)
{
  for (size_t line = 0; ; ++line)
  {
    int oa = rdz_hexdump(a, n, line, 0);
    int ob = rdz_hexdump(b, n, line, 1);
    if (oa && ob) break;
  }
}

int rdz_strcmp(char *op, char *a, char *b, ssize_t n)
{
  short i = (strchr(op, 'i') != NULL);

  if (rdz_hexdump_on) rdz_hexshow(a, b, n);

  if (n > -1) return i ? strncasecmp(a, b, n) : strncmp(a, b, n);
  return i ? strcasecmp(a, b) : strcmp(a, b);
}

char *rdz_string_eq(char *operator, char *result, char *expected)
{
  if (expected == NULL && result == NULL) return NULL;

  //if (expected == NULL) return rdz_strdup("     expected NULL");
  if (result == NULL) return rdz_strdup("     result is NULL");

  if (expected && rdz_strcmp(operator, result, expected, -1) == 0) return NULL;

  return rdz_string_expected(result, "to equal", expected);
}

char *rdz_string_neq(char *operator, char *result, char *not_expected)
{
  if (result == NULL) return rdz_strdup("     result is NULL");

  if (not_expected == NULL) return NULL;
  if (rdz_strcmp(operator, result, not_expected, -1) != 0) return NULL;

  size_t l = strlen(not_expected) + 22;

  char *s = calloc(l, sizeof(char));
  snprintf(s, l, "     didn't expect \"%s\"", not_expected);

  return s;
}

char *rdz_string_match(char *operator, char *result, char *expected)
{
  if (result == NULL) return rdz_strdup("     result is NULL");

  char *s = NULL;

  int flags = REG_EXTENDED;
  if (strchr(operator, 'i')) flags = flags | REG_ICASE;

  regex_t *r = calloc(1, sizeof(regex_t));
  regcomp(r, expected, flags);
  regmatch_t ms[1];

  if (regexec(r, result, 0, ms, 0)) // no match
  {
    if (operator[0] != '!')
      s = rdz_string_expected(result, "to match", expected);
  }
  else // match
  {
    if (operator[0] == '!')
      s = rdz_string_expected(result, "not to match", expected);
  }
  regfree(r); free(r);

  return s;
}

char *rdz_string_start(char *operator, char *result, char *expected)
{
  int success =
    result &&
    rdz_strcmp(operator, result, expected, strlen(expected)) == 0;

  if (operator[0] == '!')
  {
    return success ?
      rdz_string_expected(result, "not to start with", expected) : NULL;
  }
  return success ?
    NULL : rdz_string_expected(result, "to start with", expected);
}

char *rdz_string_end(char *operator, char *result, char *expected)
{
  int success =
    result &&
    rdz_strcmp(
      operator, result + strlen(result) - strlen(expected), expected, -1) == 0;

  if (operator[0] == '!')
  {
    return success ?
      rdz_string_expected(result, "not to end with", expected) : NULL;
  }
  return success ?
    NULL : rdz_string_expected(result, "to end with", expected);
}

static char *lower_case(char *s)
{
  char *r = calloc(strlen(s) + 1, sizeof(char));
  for (size_t i = 0; s[i]; ++i) r[i] = tolower(s[i]);

  return r;
}

char *rdz_string_contains(char *operator, char *result, char *expected)
{
  char *re = result;
  char *ex = expected;

  if (result && strchr(operator, 'i'))
  {
    re = lower_case(result);
    ex = lower_case(expected);
  }

  char *r = re ? strstr(re, ex) : NULL;

  if (re && rdz_hexdump_on) rdz_hexshow(re, ex, -1);

  if (re != result) free(re);
  if (ex != expected) free(ex);

  if (operator[0] == '!')
    return r ? rdz_string_expected(result, "not to contain", expected) : NULL;
  else
    return r ? NULL : rdz_string_expected(result, "to contain", expected);
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
    rdz_example = NULL;

    rdz_lines = calloc(RDZ_LINES_MAX + 1, sizeof(int));

    for (size_t i = 0; i < RDZ_LINES_MAX; i++)
    {
      if (l == NULL) { rdz_lines[i] = -1; continue; }

      char *c = strpbrk(l, ",");
      rdz_lines[i] = atoi(l);

      if (c == NULL) { l = NULL; continue; }

      l = c + 1;
    }
  }

  // I=6

  char *i = getenv("I");

  if (i != NULL)
  {
    rdz_example = NULL;

    rdz_it = atoi(i);
  }

  // F=fname

  char *f = getenv("F");

  if (f != NULL)
  {
    rdz_files = calloc(RDZ_FILES_MAX + 1, sizeof(char *));

    for (size_t i = 0; i < RDZ_FILES_MAX; ++i)
    {
      char *ff = strpbrk(f, " \t");
      size_t l = ff ? ff - f : strlen(f);
      char *fn = NULL;
      if (f[0] == '.')
      {
        fn = rdz_strndup(f, l);
      }
      else
      {
        fn = calloc(l + 9, sizeof(char));
        strcpy(fn, "../spec/"); strncpy(fn + 8, f, l);
      }

      wordexp_t we;
      wordexp(fn, &we, 0);

      rdz_files[i] = rdz_strdup(we.we_wordv[0]);

      wordfree(&we);
      free(fn);

      f = ff; if (f == NULL) break;
      while (1) if (f[0] != ' ' && f[0] != '\t') break; else ++f;
    }
  }

  // RDZ_HEXDUMP

  char *rh = getenv("RDZ_HEXDUMP");

  rdz_hexdump_on =
    rh &&
    (
      strcmp(rh, "1") == 0 ||
      strcasecmp(rh, "on") == 0 ||
      strcasecmp(rh, "yes") == 0 ||
      strcasecmp(rh, "true") == 0
    );
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

int rdz_determine_dorun_i(rdz_node *n)
{
  if (rdz_it < 0) return -1;

  return n->nodenumber == rdz_it;
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
    int ri = rdz_determine_dorun_i(n);

    if (rl < 0 && re < 0 && rf < 0 && ri < 0) n->dorun = 1;
    if (rf > 0) n->dorun = 1;
    if (rl > 0) n->dorun = 2; // all children if they're all 0
    if (re > 0) n->dorun = 3; // ancestors and all children
    if (ri > 0) n->dorun = 3;

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
    if (n->children[0] > -1) { rdz_dorun(rdz_nodes[n->children[0]]); return; }

    int rc = rdz_count;

    double du = n->func(); // run the "it"

    if (rdz_count == rc) // no ensure in the example, record a success...
    {
      rdz_record(1, rdz_strdup(n->text), n->nodenumber, n->lstart, n->ltstart);
    }

    rdz_print_result(rdz_results[rdz_count - 1], du);
  }
  else if (t == 'p')
  {
    rdz_record(-1, rdz_strdup(n->text), n->parentnumber, n->lstart, n->ltstart);

    rdz_print_result(rdz_results[rdz_count - 1], -1.0);
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

void rdz_summary(int itcount, double duration)
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

      printf("  %s%s%s\n", rdz_yl(), r->title, rdz_cl());
      printf("   %s# %s%s\n", rdz_cy(), r->message, rdz_cl());
      printf("   %s# %s:%d", rdz_cy(), rit->fname, r->lnumber);
      printf(" %sL=%d I=%d%s\n", rdz_gr(), r->ltnumber, r->itnumber, rdz_cl());
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
      if (r->message) { printf("%s%s%s\n", rdz_rd(), r->message, rdz_cl()); }
      printf("     >");
      printf("%s%s%s", rdz_rd(), line, rdz_cl());
      printf("<\n");
      printf("     %s# %s:%d%s", rdz_cy(), rit->fname, r->lnumber, rdz_cl());
      printf(" %sL=%d I=%d%s\n", rdz_gr(), r->ltnumber, r->itnumber, rdz_cl());
      free(line);
    }
  }

  char sdu[80]; rdz_duration_to_s(duration, sdu);

  printf("\n");
  printf("%s%d examples, ", rdz_fail_count > 0 ? rdz_rd() : rdz_gn(), itcount);
  printf("%d tests seen, ", rdz_count - rdz_pending_count);
  printf("%d failures", rdz_fail_count);
  if (rdz_pending_count > 0) printf(", %d pending", rdz_pending_count);
  if (*sdu != 0) printf("  %s%s", rdz_gr(), sdu);
  printf("%s\n", rdz_cl());
  printf("\n");

  if (rdz_fail_count > 0)
  {
    printf("Failed examples:\n\n");

    for (size_t i = 0; i < rdz_count; i++)
    {
      rdz_result *r = rdz_results[i];

      if (r->success != 0) continue;

      printf("%smake spec I=%d", rdz_rd(), r->itnumber);
      printf(" %s# %s%s\n", rdz_cy(), r->title, rdz_cl());
    }

    printf("\n");
  }

  //printf("%s%s%s\n", rdz_gr(), sdu, rdz_cl());
  //printf("\n");
}

