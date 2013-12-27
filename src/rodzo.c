
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

#include "flutil.h"

#define NODE_MAX_CHILDREN 128


//
// context and tree

typedef struct node_s {
  struct node_s *parent;
  int nodenumber;
  int indent;
  char type;
  char *text;
  char *fname;
  int lstart;
  flu_sbuffer *lines;
  struct node_s **children;
} node_s;

typedef struct context_s {
  int nodecount;
  int itcount;
  node_s *node;
  char *out_fname;
} context_s;

char *type_to_string(char t)
{
  if (t == 'd') return "describe";
  if (t == 'c') return "context";
  if (t == 'i') return "it";
  if (t == 'b') return "before each";
  if (t == 'B') return "before all";
  if (t == 'a') return "after each";
  if (t == 'A') return "after all";
  if (t == 'g') return "g";
  if (t == 'G') return "G";
  return "???";
}

void node_to_s(flu_sbuffer *b, int level, node_s *n)
{
  if (n == NULL)
  {
    flu_sbprintf(b, "*** (nil)");
  }
  else
  {
    int p = n->parent == NULL ? -1 : n->parent->nodenumber;
    char t = n->type;
    char *te = flu_strrtrim(n->text != NULL ? n->text : "(nil)");

    for (int i = 0; i < level; i++) flu_sbputs(b, "  ");
    flu_sbprintf(b, "|-- %s", type_to_string(t));
    flu_sbprintf(b, " n:%d i:%d ", n->nodenumber, n->indent);
    flu_sbprintf(b, "fn:%s l:%d p:%d\n", n->fname, n->lstart, p);
    for (int i = 0; i < level; i++) flu_sbputs(b, "  ");
    flu_sbprintf(b, "|   te: >%s<\n", te);

    free(te);

    for (size_t i = 0; n->children[i] != NULL; i++)
    {
      node_to_s(b, level + 1, n->children[i]);
    }
  }
}

char *node_to_string(node_s *n)
{
  flu_sbuffer *b = flu_sbuffer_malloc();
  node_to_s(b, 0, n);

  return flu_sbuffer_to_string(b);
}

void push_line(context_s *c, char *l)
{
  node_s *n = c->node;
  //printf("push_line() to node: %d >%s", n->nodenumber, l);
  if (n->lines == NULL) n->lines = flu_sbuffer_malloc();
  flu_sbputs(n->lines, l);
}
void push_linef(context_s *c, const char *format, ...)
{
  va_list ap;
  va_start(ap, format);
  node_s *n = c->node;
  if (n->lines == NULL) n->lines = flu_sbuffer_malloc();
  flu_vsbprintf(n->lines, format, ap);
  va_end(ap);
}

void push(context_s *c, int ind, char type, char *text, char *fn, int lstart)
{
  if (text != NULL) text = strdup(text);
  if (fn != NULL) fn = strdup(fn);

  if (ind == 0) while (c->node->parent != NULL) c->node = c->node->parent;
    // if indentation is 0, go back to trunk

  node_s *cn = c->node;

  node_s *n = malloc(sizeof(node_s));
  n->parent = cn;
  n->nodenumber = c->nodecount++;
  n->indent = ind;
  n->type = type;
  n->text = text;
  n->fname = fn;
  n->lstart = lstart;
  n->lines = NULL;
  n->children = calloc(NODE_MAX_CHILDREN + 1, sizeof(node_s *));

  for (size_t i = 0; ; i++)
  {
    if (cn == NULL) break;
    if (cn->children[i] != NULL) continue;
    cn->children[i] = n;
    break;
  }

  if (type != 'e') c->node = n;

  if (type == 'i') c->itcount++;
}

context_s *malloc_context()
{
  context_s *c = malloc(sizeof(context_s));
  c->nodecount = 0;
  c->itcount = 0;
  c->node = NULL;
  c->out_fname = NULL;

  push(c, -1, 'G', NULL, NULL, -1);

  return c;
}

void free_node(node_s *n)
{
  free(n->text);
  free(n->fname);

  flu_sbuffer_free(n->lines);

  for (size_t i = 0; ; i++)
  {
    node_s *c = n->children[i];
    if (c == NULL) break;
    free_node(c);
  }
  free(n->children);

  free(n);
}

void clear_tree(context_s *c)
{
  while (c->node->parent != NULL) c->node = c->node->parent;
  free_node(c->node);
}

void free_context(context_s *c)
{
  clear_tree(c);

  free(c->out_fname);
  free(c);
}

void pull(context_s *c, int lnumber)
{
  node_s *n = c->node;

  c->node = n->parent;

  if (c->node->type == 'G') push(c, 0, 'g', NULL, n->fname, lnumber);
}

//char current_type(context_s *c)
//{
//  if (c->node == NULL) return 'X';
//  return c->node->type;
//}

int current_indent(context_s *c)
{
  if (c->node == NULL) return -1;
  return c->node->indent;
}

char **list_texts(node_s *node)
{
  char **a = malloc(256 * sizeof(char *));
  size_t c = 0;
  node_s *n = node;
  while (n != NULL)
  {
    if (n->text != NULL) a[c++] = strdup(n->text);
    n = n->parent;
  }

  char **r = calloc(c + 1, sizeof(char *));
  for (size_t i = 0; c > 0; ) r[i++] = a[--c];

  free(a);

  return r;
}

char *str_neuter(char *s)
{
  for (size_t i = 0, j = 0; ; i++)
  {
    char c = s[i];
    if (c == '\\') { i++; continue; }
    s[j++] = c;
    if (c == '\0') break;
  }

  return s;
}

char *list_texts_as_literal(node_s *n)
{
  flu_sbuffer *b = flu_sbuffer_malloc();

  char **texts = list_texts(n);
  char **t = texts;

  flu_sbprintf(b, "{ ");

  while (*t != NULL)
  {
    flu_sbprintf(b, "\"%s\", ", str_neuter(*t));
    free(*t);
    t++;
  }

  free(texts);

  flu_sbprintf(b, "NULL }");

  return flu_sbuffer_to_string(b);
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

char *extract_head(char *line)
{
  char *stop = strpbrk(line, "     (");

  if (stop == NULL) return flu_strrtrim(line);
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

char *extract_text(char *line)
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

void process_lines(context_s *c, char *path)
{
  push(c, 0, 'g', NULL, path, 0);

  FILE *in = fopen(path, "r");
  if (in == NULL) return;

  int varcount = 0;

  int lnumber = 0;
  char *line = NULL;
  size_t len = 0;

  while (getline(&line, &len, in) != -1)
  {
    lnumber++;

    int indent = extract_indent(line);
    char *head = extract_head(line);
    char *text = extract_text(line);

    //printf("head: >%s<\n", head);
    //printf("  text: >%s<\n", text);

    //char ctype = current_type(c);
    int cindent = current_indent(c);

    if (strcmp(head, "{") == 0 && indent == cindent)
    {
      // do nothing
    }
    else if (strcmp(head, "}") == 0 && indent == cindent)
    {
      pull(c, lnumber + 1);
    }
    //else if (strcmp(head, "global") == 0 || strcmp(head, "globally") == 0)
    //{
    //  push(c, indent, 'g', head, path, lnumber);
    //}
    else if (strcmp(head, "before") == 0 || strcmp(head, "after") == 0)
    {
      char *tline = flu_strtrim(line);
      char t = 'b';
      if (strncmp(tline, "before all", 10) == 0) t = 'B';
      else if (strncmp(tline, "after each", 10) == 0) t = 'a';
      else if (strncmp(tline, "after all", 9) == 0) t = 'A';
      push(c, indent, t, tline, path, lnumber);
      free(tline);
    }
    else if (strcmp(head, "after") == 0)
    {
      // ...
    }
    else if (strcmp(head, "describe") == 0)
    {
      push(c, indent, 'd', text, path, lnumber);
    }
    else if (strcmp(head, "context") == 0)
    {
      push(c, indent, 'c', text, path, lnumber);
    }
    else if (strcmp(head, "it") == 0)
    {
      push(c, indent, 'i', text, path, lnumber);
    }
    else if (strcmp(head, "ensure") == 0)
    {
      char *l = strpbrk(line, "e");
      char *con = extract_condition(in, l + 6);
      lnumber += count_lines(con);

      char *ind = calloc(indent + 1, sizeof(char));
      for (size_t i = 0; i < indent; i++) ind[i] = ' ';

      push_linef(
        c, "%sint r%i = %s",
        ind, varcount, con);
      push_linef(
        c, "%s  rdz_record(r%i, _s, %i, _fn, %d); ",
        ind, varcount, c->node->nodenumber, lnumber);
      push_linef(
        c, "if ( ! r%i) goto _over;\n",
        varcount);

      free(ind);
      free(con);
      ++varcount;
    }
    else
    {
      push_line(c, line);
    }

    free(head);
    free(text);
  }

  free(line);
  fclose(in);
}

#include "header.c"

//void print_alls(FILE *out, char t, node_s *n)
//{
//  for (size_t i = 0; ; i++)
//  {
//    node_s *cn = n->children[i];
//
//    if (cn == NULL) break;
//    if (cn->type != t) continue;
//
//    flu_sbuffer_close(cn->lines);
//
//    //if (t == 'A') fputs("\n", out);
//    fprintf(out, "  // %s li%d\n", cn->text, cn->lstart);
//    fputs(cn->lines->string, out);
//    //if (t == 'B') fputs("\n", out);
//  }
//}

void print_eaches(FILE *out, char *indent, char t, node_s *n)
{
  if (n == NULL) return;

  if (t == 'b') print_eaches(out, indent, t, n->parent);

  for (size_t i = 0; ; i++)
  {
    node_s *cn = n->children[i];

    if (cn == NULL) break;
    if (cn->type != t) continue;

    flu_sbuffer_close(cn->lines);

    if (t == 'a') fputs("\n", out);
    fprintf(out, "%s  // %s li%d\n", indent, cn->text, cn->lstart);
    fputs(cn->lines->string, out);
    if (t == 'b') fputs("\n", out);
  }

  if (t == 'a') print_eaches(out, indent, t, n->parent);
}

void print_node(FILE *out, node_s *n)
{
  char t = n->type;

  if (t == 'a' || t == 'b') return;
  if (t == 'A' || t == 'B') return;

  char *ind;
  if (n->indent > 0)
  {
    ind = calloc(n->indent + 1, sizeof(char));
    for (size_t i = 0; i < n->indent; i++) ind[i] = ' ';
  }
  else
  {
    ind = strdup("");
  }

  if (t == 'g' && n->lstart == 0)
  {
    fprintf(out, "// file %s\n", n->fname);
  }
  else if (t == 'd' || t == 'c' || t == 'i')
  {
    fprintf(
      out, "\n");
    fprintf(
      out, "%s// %s \"%s\" li%d\n", ind, type_to_string(t), n->text, n->lstart);

    if (t == 'i') fprintf(out, "%s//\n", ind);
  }

  //print_befafts(out, "", 'B', n);

  if (t == 'i')
  {
    char *_s = list_texts_as_literal(n);
    fprintf(out, "%sint it_%d()\n", ind, n->nodenumber);
    fprintf(out, "%s{\n", ind);
    fprintf(out, "%s  char *_s[] = %s;\n", ind, _s);
    fprintf(out, "%s  char *_fn = \"%s\";\n", ind, n->fname);
    fprintf(out, "\n");
    free(_s);

    print_eaches(out, ind, 'b', n->parent);
  }

  if (n->lines != NULL)
  {
    flu_sbuffer_close(n->lines);
    fputs(n->lines->string, out);
  }

  if (t == 'i')
  {
    fprintf(out, "\n");
    fprintf(out, "%s_over:\n", ind);
    print_eaches(out, ind, 'a', n->parent);

    fprintf(out, "%s} // it_%d()\n", ind, n->nodenumber);
  }

  free(ind);

  for (size_t i = 0; ; i++)
  {
    node_s *cn = n->children[i];
    if (cn == NULL) break;
    print_node(out, cn);
  }

  //print_befafts(out, "", 'A', n);
}

void print_body(FILE *out, context_s *c)
{
  node_s *n = c->node; while (n->parent != NULL) n = n->parent;

  char *s = node_to_string(n); puts("\n"); puts(s); free(s); // prints tree

  print_node(out, n);
}

void print_it_calls(FILE *out, node_s *n)
{
  if (n->type == 'i')
  {
    fprintf(out, "  it_%d();", n->nodenumber);
    fprintf(out, " // %s li%d\n", n->fname, n->lstart);
  }
  else
  {
    for (size_t i = 0; ; i++)
    {
      if (n->children[i] == NULL) break;
      print_it_calls(out, n->children[i]);
    }
  }
}

void print_footer(FILE *out, context_s *c)
{
  fputs("\n", out);
  fputs("  /*\n", out);
  fputs("   * rodzo footer\n", out);
  fputs("   */\n\n", out);

  fprintf(out, "int main(int argc, char *argv[])\n");
  fprintf(out, "{\n");
  fprintf(out, "  rdz_results = calloc(%d, sizeof(rdz_result));\n", c->itcount);
  fprintf(out, "\n");

  node_s *n = c->node; while (n->parent != NULL) n = n->parent;
  print_it_calls(out, n);

  fputs("\n", out);
  fprintf(out, "  rdz_summary(%d);\n", c->itcount);
  fputs("}\n", out);
  fputs("\n", out);
}

int add_spec_file(int *count, char **names, char *fname)
{
  if ( ! flu_strends(fname, "_spec.c")) return 0;

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
    char *fn = flu_sprintf(
      path[strlen(path) - 1] == '/' ? "%s%s" : "%s/%s", path, ep->d_name);

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

  // reads specs, grow tree

  char **fnames = list_spec_files(argc, argv);

  for (int i = 0; ; i++)
  {
    if (fnames[i] == NULL) break;

    printf(". processing %s\n", fnames[i]);

    process_lines(c, fnames[i]);

    free(fnames[i]);
  }
  free(fnames);

  // write

  FILE *out = fopen(c->out_fname, "wb");

  if (out == NULL)
  {
    flu_die(1, "couldn't open %s file for writing", c->out_fname);
  }

  print_header(out);
  print_body(out, c);
  print_footer(out, c);

  // over

  fclose(out);

  printf(". wrote %s\n", c->out_fname);

  free_context(c);

  return 0;
}

