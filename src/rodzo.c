
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

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <libgen.h>
#include <wordexp.h>

#define TITLE_MAX_LENGTH 210


//
// helper functions

int str_ends(char *s, char *end)
{
  size_t ls = strlen(s);
  size_t le = strlen(end);

  if (le > ls) return 0;

  return (strncmp(s + ls - le, end, le) == 0);
}

//
// context

typedef struct context_s {
  int itcount;
  //int incc;
  //char **includes;
  char *out_fname;
} context_s;

context_s *malloc_context()
{
  context_s *c = malloc(sizeof(context_s));
  c->itcount = 0;
  //c->incc = 0;
  //c->includes = malloc(147 * sizeof(char *));
  c->out_fname = NULL;
  return c;
}

void free_context(context_s *c)
{
  //for (int i = 0; i < c->incc; i++) { free(c->includes[i]); }
  //free(c->includes);
  free(c->out_fname);
  free(c);
}

//void include(context_s *c, char *title)
//{
//  for (int i = 0; i < c->incc; i++)
//  {
//    if (strcmp(c->includes[i], title) == 0) return;
//  }
//  c->includes[c->incc++] = strdup(title);
//}

//
// the stack

typedef struct level_s {
  struct level_s *parent;
  int indent;
  char type;
  char *title;
  int lstart;
} level_s;

void push(level_s **stack, int indent, char type, char *title, int lstart)
{
  level_s *l = malloc(sizeof(level_s));
  l->parent = *stack;
  l->indent = indent;
  l->type = type;
  l->title = strdup(title);
  l->lstart = lstart;
  *stack = l;
}

int depth(level_s **stack)
{
  level_s *top = *stack;

  if (top->parent == NULL) return 1;
  return 1 + depth(&(top->parent));
}

void free_level(level_s *l)
{
  free(l->title);
  free(l);
}

int pop(level_s **stack)
{
  if (*stack == NULL) return 1;
  //printf("pop: %p -> %p -> %p\n", stack, *stack, (*stack)->parent);
  level_s *t = *stack;
  *stack = t->parent;
  free_level(t);
  return 0;
}

void free_stack(level_s **stack)
{
  while ( ! pop(stack)) {}
}

char **list_titles(level_s **stack)
{
  int d = depth(stack);
  level_s *top = *stack;

  char **result = malloc(d * TITLE_MAX_LENGTH * sizeof(char));

  for (int i = d - 1; i >= 0; i--)
  {
    result[i] = strdup(top->title);
    top = top->parent;
  }

  return result;
}

size_t str_neuter_copy(char *target, char *source)
{
  size_t c = 0;
  for (size_t i = 0; ; i++)
  {
    char cs = source[i];
    if (cs == '\\') { i++; continue; }
    if (cs == '\0') break;
    target[c++] = cs;
  }
  return c;
}

char *list_titles_as_literal(level_s **stack)
{
  int d = depth(stack);

  char **titles = list_titles(stack);

  char *r = calloc(d + 1, 4 + TITLE_MAX_LENGTH * sizeof(char));
  char *rr = r;

  strcpy(rr, "{ "); rr += 2;

  for (int i = 0; i < d; i++)
  {
    strcpy(rr, "\""); rr += 1;
    rr += str_neuter_copy(rr, titles[i]);
    free(titles[i]);
    strcpy(rr, "\""); rr += 1;
    if (i < d - 1) { strcpy(rr, ", "); rr += 2; }
  }

  strcpy(rr, " }"); rr += 2;
  *rr = '\0';

  free(titles);

  return r;
}

//
// processing work

int extract_indent(char *line)
{
  for (int i = 0; i < strlen(line); i++)
  {
    if (*(line + i) == ' ') continue;
    if (*(line + i) == '\t') continue;
    return i;
  }
  return -1;
}

char *str_rtrim(char *s)
{
  char *r = strdup(s);
  for (size_t l = strlen(r); l > 0; l--)
  {
    char c = r[l - 1];
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r') r[l - 1] = '\0';
  }
  return r;
}

char *extract_head(char *line)
{
  char *stop = strpbrk(line, "     (");

  if (stop == NULL) return str_rtrim(line);
  if (stop == line) return extract_head(line + 1);
  return strndup(line, stop - line);
}

char *extract_string(char *line)
{
  char *l = line;
  char *r = (char *)calloc(strlen(line), sizeof(char));
  char *rr = r;
  char prev = 0;

  while (1)
  {
    if (*l == '\0') break;
    else if (*l == '"' && prev != '\\') break;
    *(rr++) = *l;
    prev = *l;
    ++l;
  }

  if (*l == '"') return r;

  free(r);
  return NULL;
}

char *extract_title(char *line)
{
  char *start = strpbrk(line, "\"");
  if (start == NULL) return NULL;

  return extract_string(start + 1);
}

int ends_in_semicolon(char *line)
{
  //printf("strlen(line): %ld\n", strlen(line));
  for (int l = strlen(line); l > 1; l--)
  {
    char c = line[l - 1];
    //printf("c: >%c<\n", c);
    if (c == ';') return 1;
    if (c == ' ' || c == '\t' || c == '\n') continue;
    return 0;
  }
  return 0;
}

char *extract_condition(FILE *in, char *line)
{
  char *r = malloc(147 * 80 * sizeof(char));
  char *rr = r;

  strcpy(rr, line);
  rr += strlen(line);

  if (ends_in_semicolon(line)) return r;

  char *lin = NULL;
  size_t len = 0;

  while (1)
  {
    if (getline(&lin, &len, in) == -1) break;
    strcpy(rr, lin);
    rr += strlen(lin);
    if (ends_in_semicolon(lin)) break;
  }
  *rr = '\0';

  free(lin);

  return r;
}

int count_lines(char *s)
{
  int count = -1;
  while (*(s++) != '\0') { if (*s == '\n') count++; }

  return count;
}

void process_lines(FILE *out, context_s *c, char *path)
{
  fprintf(out, "\n");
  fprintf(out, "  /*\n");
  fprintf(out, "   * %s\n", path);
  fprintf(out, "   */\n");

  FILE *in = fopen(path, "r");
  if (in == NULL) return;

  level_s *stack = NULL;
  int varcount = 0;

  int lnumber = 0;
  char *line = NULL;
  size_t len = 0;

  while (getline(&line, &len, in) != -1)
  {
    lnumber++;

    int indent = extract_indent(line);
    char *head = extract_head(line);
    char *title = extract_title(line);

    char stype = (stack != NULL) ? stack->type : 'X';
    int sindent = (stack != NULL) ? stack->indent : -1;

    if (strcmp(head, "{") == 0 && stype != 'i')
    {
      // do not output
    }
    else if (strcmp(head, "}") == 0 && indent == sindent)
    {
      pop(&stack);
      if (stype == 'i')
      {
        fprintf(out, "  return 1;\n");
        fprintf(out, "%s", line);
      }
    }
    else if (strcmp(head, "describe") == 0)
    {
      push(&stack, indent, 'd', title, lnumber);
    }
    else if (strcmp(head, "context") == 0)
    {
      push(&stack, indent, 'c', title, lnumber);
    }
    else if (strcmp(head, "it") == 0)
    {
      push(&stack, indent, 'i', title, lnumber);
      char *s = list_titles_as_literal(&stack);
      int sc = depth(&stack);
      c->itcount++;
      fprintf(out, "\n");
      fprintf(out, "int sc_%i = %i;\n", c->itcount, sc);
      fprintf(out, "char *s_%i[] = %s;\n", c->itcount, s);
      fprintf(out, "char *fn_%i = \"%s\";\n", c->itcount, path);
      fprintf(out, "int it_%i()\n", c->itcount);
      free(s);
    }
    else if (strcmp(head, "ensure") == 0)
    {
      char *l = strpbrk(line, "e");
      char *con = extract_condition(in, l + 6);
      lnumber += count_lines(con);
      fprintf(
        out,
        "  int r%i = %s", varcount, con);
      fprintf(
        out,
        "    rdz_record(r%i, sc_%i, s_%i, %i, fn_%i, %d);\n",
        varcount, c->itcount, c->itcount, c->itcount, c->itcount, lnumber);
      fprintf(
        out,
        "    if ( ! r%i) return 0;\n",
        varcount);
      free(con);
      ++varcount;
    }
    else
    {
      fprintf(out, "%s", line);
    }

    free(head);
    free(title);
  }

  free(line);
  fclose(in);

  free_stack(&stack);
}

#include "header.c"

void print_footer(FILE *out, int itcount)
{
  fputs("\n", out);
  fputs("  /*\n", out);
  fputs("   * rodzo footer\n", out);
  fputs("   */\n\n", out);

  fprintf(out, "int main(int argc, char *argv[])\n");
  fprintf(out, "{\n");
  fprintf(out, "  rdz_results = calloc(%d, sizeof(rdz_result));\n", itcount);
  fprintf(out, "\n");
  for (int i = 1; i <= itcount; i++)
  {
    fprintf(out, "  it_%d();\n", i);
  }
  fputs("\n", out);
  fprintf(out, "  rdz_summary(%d);\n", itcount);
  fputs("}\n", out);
  fputs("\n", out);
}

int add_spec_file(int *count, char **names, char *fname)
{
  if ( ! str_ends(fname, "_spec.c")) return 0;

  for (int i = 0; i < *count; i++)
  {
    if (strcmp(names[i], fname) == 0) return 1; // prevent duplicates
  }

  names[(*count)++] = strdup(fname);
  return 1;
}

void add_spec_files(int *count, char **names, char *path)
{
  if (add_spec_file(count, names, path)) return;

  DIR *dir = opendir(path);

  if (dir == NULL) return;

  struct dirent *ep;
  while ((ep = readdir(dir)) != NULL)
  {
    char *fn = malloc((strlen(path) + strlen(ep->d_name) + 4) * sizeof(char));
    sprintf(fn, "%s/%s", path, ep->d_name);

    add_spec_file(count, names, fn);

    free(fn);
  }

  closedir(dir);
}

char **list_spec_files(int argc, char *argv[])
{
  char **r = calloc(512, sizeof(char *));
  int c = 0;
  int args_seen = 0;

  for (int i = 1; i < argc; i++)
  {
    char *arg = argv[i];

    if (strncmp(arg, "-", 1) == 0) continue;

    args_seen++;

    wordexp_t we;
    wordexp(arg, &we, 0);

    for (int j = 0; j < we.we_wordc; j++)
    {
      add_spec_files(&c, r, we.we_wordv[j]);
    }

    wordfree(&we);
  }

  if (args_seen < 1) add_spec_files(&c, r, ".");

  return r;
}

int print_usage()
{
  printf("rodzo usage...");
  return 1;
}

int main(int argc, char *argv[])
{
  // deal with arguments

  context_s *c = malloc_context();

  for (int i = 1; i < argc; i++)
  {
    char *a = argv[i];
    if (strcmp(a, "-o") == 0) {
      if (i + 1 >= argc) return print_usage(); // TODO: replace with die()
      c->out_fname = strdup(argv[++i]);
    }
  }

  if (c->out_fname == NULL) c->out_fname = strdup("spec.c");

  // begin work

  FILE *out = fopen(c->out_fname, "wb");

  if (out == NULL)
  {
    char *msg = malloc((32 + strlen(c->out_fname)) * sizeof(char));
    sprintf(msg, "couldn't open %s file for writing", c->out_fname);
    perror(msg); // TODO: replace with die()
    free(msg);
    return 1;
  }

  print_header(out);

  char **fnames = list_spec_files(argc, argv);

  for (int i = 0; ; i++)
  {
    if (fnames[i] == NULL) break;

    printf(". processing %s\n", fnames[i]);

    process_lines(out, c, fnames[i]);

    free(fnames[i]);
  }
  free(fnames);

  print_footer(out, c->itcount);

  fclose(out);

  printf(". wrote %s\n", c->out_fname);

  free_context(c);

  return 0;
}

