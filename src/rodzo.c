
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

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <regex.h>
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
  short hasbody;
  char type;
  char *text;
  char *fname;
  int lstart;
  int ltstart;
  int llength;
  flu_sbuffer *lines;
  struct node_s **children;
} node_s;

typedef struct context_s {
  int loffset;
  int nodecount;
  int itcount; // it count
  int encount; // ensure count
  node_s *node;
  char *out_fname;
} context_s;

char *type_to_string(char t)
{
  if (t == 'd') return "describe";
  if (t == 'c') return "context";
  if (t == 'i') return "it";
  if (t == 'p') return "pending";
  if (t == 'b') return "before each";
  if (t == 'y') return "before each offline";
  if (t == 'B') return "before all";
  if (t == 'a') return "after each";
  if (t == 'z') return "after each offline";
  if (t == 'A') return "after all";
  if (t == 'g') return "g";
  if (t == 'G') return "G";
  return "???";
}

void node_to_s(flu_sbuffer *b, int level, node_s *n)
{
  if (n == NULL) { flu_sbprintf(b, "*** (nil)"); return; }

  int p = n->parent == NULL ? -1 : n->parent->nodenumber;
  char t = n->type;
  char *te = flu_strrtrim(n->text != NULL ? n->text : "(nil)");

  for (int i = 0; i < level; i++) flu_sbputs(b, "  ");
  flu_sbprintf(b, "|-- %s", type_to_string(t));
  flu_sbprintf(b, " n:%d i:%d ", n->nodenumber, n->indent);
  flu_sbprintf(b, "fn:%s ", n->fname);
  flu_sbprintf(b, "ls:%d lts:%d ll:%d ", n->lstart, n->ltstart, n->llength);
  flu_sbprintf(b, "p:%d\n", p);
  for (int i = 0; i < level; i++) flu_sbputs(b, "  ");
  flu_sbprintf(b, "|   te: >%s<\n", te);

  free(te);

  for (size_t i = 0; n->children[i] != NULL; i++)
  {
    node_to_s(b, level + 1, n->children[i]);
  }
}

char *node_to_string(node_s *n)
{
  flu_sbuffer *b = flu_sbuffer_malloc();
  node_to_s(b, 0, n);

  return flu_sbuffer_to_string(b);
}

void node_to_p(flu_sbuffer *b, int level, node_s *n)
{
  if (n == NULL) { flu_sbprintf(b, "// NULL"); return; }

  char t = n->type;
  int notg = (t != 'g' && t != 'G');
  int notba = (t != 'b' && t != 'B' && t != 'a' && t != 'A');

  if (notg)
  {
    char *te = flu_strrtrim(n->text != NULL ? n->text : "(nil)");

    for (int i = 0; i < level; i++) flu_sbputs(b, "  ");
    flu_sbprintf(b, "%s \"%s\"\n", type_to_string(n->type), te);

    free(te);
  }

  if (t != 'p')
  {
    if (notg && notba)
    {
      for (int i = 0; i < level; i++) flu_sbputs(b, "  ");
      flu_sbprintf(b, "{\n");
    }

    for (size_t i = 0; n->children[i] != NULL; i++)
    {
      node_to_p(b, level + 1, n->children[i]);
    }

    if (notg && notba)
    {
      for (int i = 0; i < level; i++) flu_sbputs(b, "  ");
      flu_sbprintf(b, "}\n");
    }
  }
}

char *node_to_pseudo(node_s *n)
{
  flu_sbuffer *b = flu_sbuffer_malloc();
  node_to_p(b, 0, n);

  return flu_sbuffer_to_string(b);
}

void push_line(context_s *c, char *l)
{
  node_s *n = c->node;
  if (n->lines == NULL) n->lines = flu_sbuffer_malloc();
  flu_sbputs(n->lines, l);
}
void push_linef(context_s *c, const char *format, ...)
{
  va_list ap;
  va_start(ap, format);
  node_s *n = c->node;
  if (n->lines == NULL) n->lines = flu_sbuffer_malloc();
  flu_sbvprintf(n->lines, format, ap);
  va_end(ap);
}

void push(context_s *c, int ind, char type, char *text, char *fn, int lstart)
{
  if (text == NULL && type == 'p') text = "no reason given";
  if (text != NULL) text = strdup(text);

  if (fn != NULL) fn = strdup(fn);

  if (ind == 0) while (c->node->parent != NULL) c->node = c->node->parent;
    // if indentation is 0, go back to trunk

  node_s *cn = c->node;

  if (cn && cn->type == 'i' && type != 'p') // "it" without bodies
  {
    // TODO: find a way to let pull() deal with the 5 next lines
    if (cn->children[0] == NULL)
    {
      push(c, ind, 'p', "not yet implemented", fn, cn->lstart);
    }
    cn = cn->parent;
  }

  node_s *n = calloc(1, sizeof(node_s));
  n->parent = cn;
  n->nodenumber = c->nodecount++;
  n->indent = ind;
  n->hasbody = 0;
  n->type = type;
  n->text = text;
  n->fname = fn;
  n->lstart = lstart;
  n->ltstart = c->loffset + lstart;
  n->llength = 0;
  n->lines = NULL;
  n->children = calloc(NODE_MAX_CHILDREN + 1, sizeof(node_s *));

  for (size_t i = 0; ; i++)
  {
    if (cn == NULL) break;
    if (cn->children[i] != NULL) continue;
    cn->children[i] = n;
    break;
  }

  c->node = n;
  if (type == 'p') c->node = cn;

  if (type == 'i') c->itcount++;
}

context_s *malloc_context()
{
  context_s *c = calloc(1, sizeof(context_s));
  c->loffset = 0;
  c->nodecount = 0;
  c->itcount = 0;
  c->encount = 0;
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

void pull(context_s *c, int indent, int lnumber)
{
  node_s *n = c->node;

  n->llength = lnumber - n->lstart;

  if (n->type == 'i' && n->hasbody == 0)
  {
    push(c, indent, 'p', "not yet implemented", n->fname, lnumber);
    c->node = n->parent->parent; n = n->parent;
  }

  c->node = n->parent;

  if (c->node->type == 'G')
  {
    push(c, 0, 'g', NULL, n->fname, lnumber + 1);
  }
}

char **list_texts(node_s *node)
{
  char **a = calloc(256, sizeof(char *));
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
  for (int l = strlen(line); l > 1; l--)
  {
    char c = line[l - 1];
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

regex_t ensure_operator_rex;

int push_ensure(context_s *c, FILE *in, int indent, int lnumber, char *l)
{
  l = strpbrk(l, "e");
  char *con = extract_condition(in, l + 6);
  lnumber += count_lines(con);

  char *ind = calloc(indent + 1, sizeof(char));
  for (size_t i = 0; i < indent; i++) ind[i] = ' ';

  push_linef(c, "%schar *msg%d = NULL;\n", ind, lnumber);

  regmatch_t ms[2];

  if (regexec(&ensure_operator_rex, con, 2, ms, 0)) // no match
  {
    push_linef(c, "%sint r%d = %s", ind, lnumber, con);
  }
  else // match
  {
    //if (right[0] == '"') {} // for now it only works with strings

    char *operator = strndup(con + ms[1].rm_so, ms[1].rm_eo - ms[1].rm_so);

    con[ms[1].rm_so] = '\0';
    char *left = flu_strtrim(con);
    char *right = flu_strtrim(con + ms[1].rm_eo);
    right[strlen(right) - 2] = '\0'; // remove trailing ;

    char *fun = "rdz_string_eq";
    if (operator[0] == '!') fun = "rdz_string_neq";
    else if (operator[0] == '~') fun = "rdz_string_match";

    push_linef(
      c, "%schar *result%d = %s);\n",
      ind, lnumber, left);
    push_linef(
      c, "%schar *expected%d = %s;\n",
      ind, lnumber, right);
    push_linef(
      c, "%smsg%d = %s(\"%s\", result%d, expected%d);\n",
      ind, lnumber, fun, operator, lnumber, lnumber);
    push_linef(
      c, "%sint r%d = (msg%d == NULL);\n",
      ind, lnumber, lnumber);

    if (strchr(operator, 'f'))
    {
      push_linef(
        c, "%sfree(result%d);\n",
        ind, lnumber);
    }
    else if (strchr(operator, 'F'))
    {
      push_linef(
        c, "%sfree(result%d); free(expected%d);\n",
        ind, lnumber, lnumber);
    }

    free(operator);
    free(left);
    free(right);
  }

  push_linef(
    c, "%srdz_record(r%d, msg%d, %d, %d, %d); ",
    ind, lnumber, lnumber, c->node->nodenumber, lnumber, c->loffset + lnumber);
  push_linef(
    c, "if ( ! r%d) goto _over;\n",
    lnumber);

  free(ind);
  free(con);

  c->encount++;

  return lnumber;
}

void push_pending(context_s *c, int ind, char *text, char *fn, int lstart)
{
  node_s *cn = c->node;
  if (cn->type == 'i' && cn->indent < ind)
  {
    push(c, ind, 'p', text, fn, lstart);
  }
  else // lonely p
  {
    push(c, ind, 'i', text, fn, lstart);
    push(c, ind + 2, 'p', NULL, fn, lstart); // "no reason given"
  }
}

void process_lines(context_s *c, char *path)
{
  push(c, 0, 'g', NULL, path, 0);

  FILE *in = fopen(path, "r");
  if (in == NULL) return;

  int lnumber = 0;
  char *line = NULL;
  size_t len = 0;

  while (getline(&line, &len, in) != -1)
  {
    lnumber++;

    int indent = extract_indent(line);
    char *head = extract_head(line);
    char *text = extract_text(line);

    //printf("line: >%s<\n", line);
    //printf("head: >%s<\n", head);
    //printf("  text: >%s<\n", text);

    char ctype = 'X'; int cindent = -1;
    if (c->node != NULL) { ctype = c->node->type; cindent = c->node->indent; }

    if (strcmp(head, "{") == 0 && indent == cindent)
    {
      c->node->hasbody = 1;
    }
    else if (strcmp(head, "}") == 0 && (indent <= cindent || ctype != 'i'))
    {
      pull(c, indent, lnumber);
    }
    else if (strcmp(head, "before") == 0 || strcmp(head, "after") == 0)
    {
      char *tline = flu_strtrim(line);
      char t = 'b'; // "before each"
      if (strncmp(tline, "before all", 10) == 0) t = 'B';
      else if (strncmp(tline, "before each off", 15) == 0) t = 'y';
      else if (strncmp(tline, "after each off", 14) == 0) t = 'z';
      else if (strncmp(tline, "after each", 10) == 0) t = 'a';
      else if (strncmp(tline, "after all", 9) == 0) t = 'A';
      push(c, indent, t, tline, path, lnumber);
      free(tline);
    }
    else if (strcmp(head, "describe") == 0)
    {
      push(c, indent, 'd', text, path, lnumber);
    }
    else if (strcmp(head, "context") == 0)
    {
      push(c, indent, 'c', text, path, lnumber);
    }
    else if (strcmp(head, "it") == 0 || strcmp(head, "they") == 0)
    {
      push(c, indent, 'i', text, path, lnumber);
    }
    else if (strcmp(head, "ensure") == 0)
    {
      lnumber = push_ensure(c, in, indent, lnumber, line);
    }
    else if (strcmp(head, "pending") == 0)
    {
      //push(c, indent, 'p', text, path, lnumber);
      push_pending(c, indent, text, path, lnumber);
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

  c->loffset += lnumber;
}

#include "header.c"

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

  if (t == 'b' || t == 'a') return;
  if (t == 'i' && n->children[0] != NULL) return;

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
    fprintf(
      out, "// file %s\n", n->fname);
  }
  else if (t == 'd' || t == 'c' || t == 'i')
  {
    fprintf(
      out, "\n");
    fprintf(
      out, "%s// %s \"%s\" li%d\n", ind, type_to_string(t), n->text, n->lstart);
    fprintf(
      out, "%s// file %s\n", ind, n->fname);

    if (t == 'i') fprintf(out, "%s//\n", ind);
  }

  int offline = (t == 'B' || t == 'A' || t == 'y' || t == 'z');

  if (t == 'i')
  {
    char *_s = list_texts_as_literal(n);
    fprintf(out, "%sint it_%d()\n", ind, n->nodenumber);
    fprintf(out, "%s{\n", ind);
    free(_s);

    print_eaches(out, ind, 'b', n->parent);
  }
  else if (offline)
  {
    char *type = "before_all";
    if (t == 'A') type = "after_all";
    else if (t == 'y') type = "before_each_offline";
    else if (t == 'z') type = "after_each_offline";

    fprintf(out, "\n");
    fprintf(out, "%sint %s_%d()", ind, type, n->nodenumber);
    fprintf(out, " // li%d\n", n->lstart);
    fprintf(out, "%s{\n", ind);
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

    fprintf(out, "\n");
    fprintf(out, "%s  return 1;\n", ind);
    fprintf(out, "%s} // it_%d()\n", ind, n->nodenumber);
  }
  else if (offline)
  {
    fprintf(out, "%s}\n", ind);
  }

  free(ind);

  for (size_t i = 0; ; i++)
  {
    node_s *cn = n->children[i];
    if (cn == NULL) break;
    print_node(out, cn);
  }
}

void print_body(FILE *out, context_s *c)
{
  node_s *n = c->node; while (n->parent != NULL) n = n->parent;

  print_node(out, n);

  // print tree to ./spec_tree.txt

  FILE *f = fopen("spec_tree.txt", "wb");
  char *s = node_to_string(n);
  fputs("\n", f); fputs(s, f); fputs("\n", f);
  //puts("\n"); puts(s); // print to stdout as well
  free(s);
  fclose(f);

  // print pseudo to ./spec_pseudo.txt

  f = fopen("spec_pseudo.txt", "wb");
  s = node_to_pseudo(n);
  fputs("\n", f); fputs(s, f); fputs("\n", f);
  free(s);
  fclose(f);
}

void print_nodes(FILE *out, node_s *n)
{
  char t = n->type;

  flu_sbuffer *b = flu_sbuffer_malloc();
  flu_sbprintf(b, "(int []){");
  for (size_t i = 0; n->children[i] != NULL; i++)
  {
    flu_sbprintf(b, " %d,", n->children[i]->nodenumber);
  }
  flu_sbprintf(b, " -1 }");
  char *children = flu_sbuffer_to_string(b);

  char *func;
  if (t == 'i' && n->children[0] != NULL) func = strdup("NULL");
  else if (t == 'i') func = flu_sprintf("it_%d", n->nodenumber);
  else if (t == 'B') func = flu_sprintf("before_all_%d", n->nodenumber);
  else if (t == 'A') func = flu_sprintf("after_all_%d", n->nodenumber);
  else if (t == 'y') func = flu_sprintf("before_each_offline_%d", n->nodenumber);
  else if (t == 'z') func = flu_sprintf("after_each_offline_%d", n->nodenumber);
  else func = strdup("NULL");

  char *ss;
  if (t == 'i' || t == 'd' || t == 'c' || t == 'p') {
    char *ls = list_texts_as_literal(n);
    ss = flu_sprintf("(char *[])%s", ls);
    free(ls);
  }
  else
  {
    ss = strdup("NULL");
  }

  fprintf(
    out,
    "    &(rdz_node)"
    "{ 0, %d, %d, %s, %d, '%c', \"%s\", %d, %d, %d, %s, %s },\n",
    n->nodenumber,
    n->parent != NULL ? n->parent->nodenumber : -1,
    children,
    n->indent,
    t,
    n->fname,
    n->lstart, n->ltstart, n->llength, ss, func);

  free(children);
  free(ss);
  free(func);

  for (size_t i = 0; n->children[i] != NULL; i++)
  {
    print_nodes(out, n->children[i]);
  }
}

void print_footer(FILE *out, context_s *c)
{
  fputs("\n", out);
  fputs("  /*\n", out);
  fputs("   * rodzo footer\n", out);
  fputs("   */\n\n", out);

  node_s *n = c->node; while (n->parent != NULL) n = n->parent;

  fprintf(out, "int main(int argc, char *argv[])\n");
  fprintf(out, "{\n");

  fprintf(out, "  rdz_extract_arguments();\n");
  fprintf(out, "\n");

  fprintf(out, "  rdz_nodes = (rdz_node *[]){\n");
  print_nodes(out, n);
  fprintf(out, "    NULL };\n");
  fprintf(out, "\n");

  int count = c->encount;
  if (count < c->itcount) count = c->itcount;
  //
  fprintf(out, "  rdz_results = calloc(%d, sizeof(rdz_result *));\n", count);
  fprintf(out, "\n");

  fprintf(out, "  rdz_determine_dorun();\n");
  fprintf(out, "  rdz_dorun(rdz_nodes[0]);\n");

  fputs("\n", out);
  fprintf(out, "  rdz_summary(%d);\n", c->itcount);

  fputs("\n", out);
  fprintf(out, "  for (size_t i = 0; i < rdz_count; i++) rdz_result_free(rdz_results[i]);\n");
  fprintf(out, "  free(rdz_results);\n");

  fputs("\n", out);
  fprintf(out, "  free(rdz_lines);\n");

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

int _strcmp(const void *s0, const void *s1)
{
  return strcmp(*(char * const *)s0, *(char * const *)s1);
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

  qsort(r, c, sizeof(char *), _strcmp);

  return r;
}

int print_usage()
{
  printf("rodzo usage...");
  // TODO: write usage
  return 1;
}

int main(int argc, char *argv[])
{
  regcomp(&ensure_operator_rex, " ([!=~]==[fF]?) ", REG_EXTENDED);

  // deal with arguments

  context_s *c = malloc_context();

  for (int i = 1; i < argc; i++)
  {
    char *a = argv[i];
    if (strcmp(a, "-o") == 0) {
      if (i + 1 >= argc) return print_usage(); // TODO: print then die?
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

  regfree(&ensure_operator_rex);

  return 0;
}

