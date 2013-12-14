
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

typedef struct rdz_failure {
  char *title;
  char *fname;
  int lnumber;
} rdz_failure;

rdz_failure *rdz_malloc_failure(int sc, char *s[], char *fname, int lnumber)
{
  char *title = calloc(sc * 160, sizeof(char));
  char *t = title;
  for (int i = 0; i < sc; i++) {
    strcpy(t, s[i]); t += strlen(s[i]);
    if (i < sc - 1) *(t++) = ' ';
  }

  rdz_failure *f = malloc(sizeof(rdz_failure));
  f->title = title;
  f->fname = rdz_strdup(fname);
  f->lnumber = lnumber;

  return f;
}

void rdz_free_failure(rdz_failure *f)
{
  free(f->title);
  free(f->fname);
  free(f);
}

int rdz_count = 0;
int rdz_fail_count = 0;
rdz_failure **rdz_failures = NULL;
char *rdz_last_context = NULL;

void rdz_red() { printf("[31m"); }
void rdz_green() { printf("[32m"); }
void rdz_yellow() { printf("[33m"); }
void rdz_white() { printf("[37m"); }
void rdz_clear() { printf("[0m"); }

void rdz_result(int success, int sc, char *s[], char *fname, int lnumber)
{
  if (rdz_last_context == NULL) rdz_last_context = rdz_strdup("");

  char *context = s[sc - 2];

  if (strcmp(rdz_last_context, context) != 0)
  {
    for (int i = 0; i < sc - 1; i++)
    {
      for (int ii = 0; ii < i; ii++) printf("  "); // indent
      printf("%s\n", s[i]);
    }
  }

  for (int ii = 0; ii < sc - 1; ii++) printf("  "); // indent
  if (success) rdz_green();
  else rdz_red();
  printf("%s", s[sc - 1]);
  if ( ! success) printf(" (FAILED)");
  printf("\n");
  rdz_clear();

  if (rdz_last_context != NULL) free(rdz_last_context);
  rdz_last_context = rdz_strdup(context);

  if ( ! success)
  {
    rdz_failures[rdz_fail_count++] = rdz_malloc_failure(sc, s, fname, lnumber);
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

void rdz_summary()
{
  printf("\nfail count: %d\n\n", rdz_fail_count);
  for (int i = 0; i < rdz_fail_count; i++)
  {
    rdz_failure *f = rdz_failures[i];
    char *line = rdz_read_line(f->fname, f->lnumber);
    printf("fail:\n");
    printf("  %s\n", f->title);
    printf("  %s:%d\n", f->fname, f->lnumber);
    printf("  >%s<\n", line);

    rdz_free_failure(f);
    free(line);
  }
  free(rdz_failures);
  printf("\n");
  if (rdz_fail_count > 0) rdz_red(); else rdz_green();
  printf("%d tests, %d failures\n", rdz_count, rdz_fail_count);
  rdz_clear();
  printf("\n");
}

void rdz_free()
{
  if (rdz_last_context != NULL) free(rdz_last_context);
}

