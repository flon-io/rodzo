
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
#include <unistd.h>

#include "flutil.h"


#define RODZO_VERSION "1.2.0"

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

typedef struct {
  int loffset;
  int nodecount;
  int itcount; // it count
  int encount; // ensure count
  node_s *node;
  char *out_fname;
  int debug;
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
    if (cn->children[0] == NULL)
    {
      //push(c, ind, 'p', "not yet implemented 2", fn, cn->lstart);
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
  c->debug = 0;

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

  //printf("0 pull() from %d to %d\n", n->indent, n->parent->indent);

  n->llength = lnumber - n->lstart;

  if (n->type == 'i' && n->hasbody == 0)
  {
    push(c, n->indent, 'p', "not yet implemented", n->fname, lnumber);
    c->node = n->parent->parent; n = n->parent;
  }

  //printf("1 pull() from %d to %d\n", n->indent, n->parent->indent);

  c->node = n->parent;

  if (c->node->type == 'G')
  {
    push(c, 0, 'g', NULL, n->fname, lnumber + 1);
  }
}

//
// processing work

typedef struct line_s {
  int indent;
  char *head;
  char *text;
  char *line;
  int comment; // 0 no change, 1 comment started, -1 comment ended
} line_s;

void line_s_free(line_s *l)
{
  free(l->head);
  free(l->text);
  free(l->line);
  free(l);
}

char *line_s_to_s(line_s *l, int colour)
{
  flu_sbuffer *b = flu_sbuffer_malloc();

  char *con = colour ? "[1;33m" : "";
  char *coff = colour ? "[0;00m" : "";

  flu_sbprintf(
    b,
    "(l i%i c%i h>%s%s%s< t>%s%s%s< l>%s%s%s<)",
    l->indent, l->comment,
    con, l->head, coff, con, l->text, coff, con, l->line, coff);

  return flu_sbuffer_to_string(b);
}

void rtrim(char *s)
{
  if (s && *s != 0) for (size_t i = strlen(s); i > 0; --i)
  {
    char c = s[i - 1];
    if (strchr(" \t\r\n", c) == NULL) break;
    s[i - 1] = 0;
  }
}

line_s *split(int comment, char *line)
{
  size_t len = strlen(line) + 1;
  line_s *l = calloc(1, sizeof(line_s));
  l->indent = 0;
  l->head = NULL;
  l->text = NULL;
  l->line = calloc(len, sizeof(char));
  l->comment = comment;

  int string = 0;
  int escape = 0;

  for (ssize_t i = 0, j = 0, k = 0; i < len; ++i)
  {
    char c = line[i]; char c1 = line[i + 1];

    if ( ! string && c == '*' && c1 == '/') { l->comment = -1; ++i; continue; }

    if (l->comment == 1) continue;

    if ( ! string && c == '/' && c1 == '/') break;
    if ( ! string && c == '/' && c1 == '*') { l->comment = 1; ++i; continue; }

    l->line[j++] = c;

    if (l->head == NULL) // measuring indent
    {
      if (c == ' ' || c == '\t') { ++l->indent; continue; }
      l->head = calloc(len, sizeof(char)); l->head[0] = c; k = 1;
    }
    else if (l->text == NULL) // gathering head
    {
      if (strchr("\"( \t", c)) { l->text = calloc(len, sizeof(char)); k = len; }
      else { l->head[k++] = c; }
    }
    else if (k == len) // space before text
    {
      if (strchr("\"() \t", c) == NULL) { k = 0; l->text[k++] = c; }
      else if (c == ')') k = -1;
    }
    else if (k > -1) // gathering text
    {
      if (string == 1 && escape == 0 && c == '"') k = -1;
      else if (string == 0 && c == ')') k = -1;
      else l->text[k++] = c;
    }

    if (escape == 0 && c == '"') string = ! string;
    escape = (c == '\\');
  }

  rtrim(l->head);
  rtrim(l->text);
  rtrim(l->line);

  return l;
}

int ends_in_semicolon(char *line)
{
  if (*line != 0) for (size_t i = strlen(line) - 1; ; --i)
  {
    char c = line[i];
    if (c == ';') return 1;
    if (i > 0 && (c == '\r' || c == '\n')) continue;
    return 0;
  }
  return 0;
}

char *extract_condition(FILE *in, char *line)
{
  flu_sbuffer *b = flu_sbuffer_malloc();

  flu_sbputs(b, line);

  if (ends_in_semicolon(line)) return flu_sbuffer_to_string(b);

  char *lin = NULL;
  size_t len = 0;

  while (1)
  {
    if (getline(&lin, &len, in) == -1) break;
    flu_sbputs(b, lin);
    if (ends_in_semicolon(lin)) break;
  }

  free(lin);

  return flu_sbuffer_to_string(b);
}

size_t count_lines(char *s)
{
  size_t count = 0;
  while (*(s++) != '\0') { if (*s == '\n') count++; }

  return count;
}

regex_t ensure_operator_rex;

char *chop_right(char *s)
{
  char *brackend = strrchr(s, ')');
  if (brackend) *brackend = 0;

  return s;
}

char *extract_match(char *s, regmatch_t m)
{
  return strndup(s + m.rm_so, m.rm_eo - m.rm_so);
}

int push_ensure(context_s *c, FILE *in, int indent, int lnumber, char *l)
{
  l = strchr(l, 'e');
  char *con = extract_condition(in, l + 6);
  lnumber += count_lines(con);

  //printf("con >%s<\n", con);

  char *ind = calloc(indent + 1, sizeof(char));
  for (size_t i = 0; i < indent; i++) ind[i] = ' ';

  push_linef(c, "%schar *msg%d = NULL;\n", ind, lnumber);

  regmatch_t ms[6];

  if (regexec(&ensure_operator_rex, con, 6, ms, 0)) // no match
  {
    push_linef(c, "%sint r%d = %s", ind, lnumber, con);
  }
  else if (ms[3].rm_eo > ms[3].rm_so) // type
  {
    char *format = extract_match(con, ms[3]);
    char *eq = extract_match(con, ms[4]);

    char *left = strndup(con + 1, ms[1].rm_so - 2);
    //
    char *right = strndup(con + ms[1].rm_eo + 1, strlen(con) - ms[1].rm_eo - 1);
    *(strrchr(right, ')')) = '\0';

    char *type = "int";
    //if (strcmp(format, "d") == 0) type = "int";
    if (strcmp(format, "s") == 0) type = "short";
    else if (strcmp(format, "c") == 0) type = "char";
    else if (strcmp(format, "d") == 0) type = "double";
    else if (strcmp(format, "f") == 0) type = "double";
    else if (strcmp(format, "e") == 0) type = "double";
    else if (strcmp(format, "o") == 0) type = "unsigned int";
    else if (strcmp(format, "u") == 0) type = "unsigned int";
    else if (strcmp(format, "li") == 0) type = "long";
    else if (strcmp(format, "lli") == 0) type = "long long";
    else if (strcmp(format, "lu") == 0) type = "long unsigned";
    else if (strcmp(format, "llu") == 0) type = "long long unsigned";
    else if (strcmp(format, "zu") == 0) type = "size_t";
    else if (strcmp(format, "zd") == 0) type = "ssize_t";

    push_linef(c, "%s%s left%d = %s;\n", ind, type, lnumber, left);
    push_linef(c, "%s%s right%d = %s;\n", ind, type, lnumber, right);

    push_linef(
      c,
      "%sint r%d = (left%d %s right%d);\n",
      ind, lnumber, lnumber, eq, lnumber);

    push_linef(c, "%sif ( ! r%d)\n", ind, lnumber);
    push_linef(c, "%s{\n", ind);
    push_linef(c, "%s  msg%d = calloc(2048, sizeof(char));\n", ind, lnumber);
    push_linef(c, "%s  snprintf(\n", ind);
    push_linef(c, "%s    msg%d, 2048,\n", ind, lnumber);
    push_linef(c, "%s      \"     expected %s%s\\n\"\n", ind, "%", format);
    push_linef(c, "%s      \"     %s8s %s%s\",\n", ind, "%", "%", format);
    push_linef(c, "%s      left%d, \"to %s\", right%d\n", ind, lnumber, eq, lnumber);
    push_linef(c, "%s    );\n", ind);
    push_linef(c, "%s}\n", ind);
  }
  else // string
  {
    char *oper = extract_match(con, ms[5]);

    con[ms[1].rm_so] = '\0';
    char *left = flu_strtrim(con);
    char *right = chop_right(flu_strtrim(con + ms[1].rm_eo));

    char op = oper[0] == '!' && oper[1] != '=' ? oper[1] : oper[0];
    //
    char *fun = "rdz_string_eq";
    if (op == '!') fun = "rdz_string_neq";
    else if (op == '~') fun = "rdz_string_match";
    else if (op == '^') fun = "rdz_string_start";
    else if (op == '$') fun = "rdz_string_end";
    else if (op == '>') fun = "rdz_string_contains";

    push_linef(
      c, "%schar *result%d = %s);\n",
      ind, lnumber, left);
    push_linef(
      c, "%schar *expected%d = %s;\n",
      ind, lnumber, right);
    push_linef(
      c, "%smsg%d = %s(\"%s\", result%d, expected%d);\n",
      ind, lnumber, fun, oper, lnumber, lnumber);
    push_linef(
      c, "%sint r%d = (msg%d == NULL);\n",
      ind, lnumber, lnumber);

    if (strchr(oper, 'f'))
    {
      push_linef(
        c, "%sfree(result%d);\n",
        ind, lnumber);
    }
    else if (strchr(oper, 'F'))
    {
      push_linef(
        c, "%sfree(result%d); free(expected%d);\n",
        ind, lnumber, lnumber);
    }

    free(oper);
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

void push_pending(context_s *c, line_s *l, char *fn, int lstart)
{
  node_s *cn = c->node;

  if (cn->type == 'i' && cn->indent < l->indent)
  {
    push(c, l->indent, 'p', strlen(l->text) > 0 ? l->text : NULL, fn, lstart);
  }
  else // lonely p
  {
    push(c, l->indent, 'i', l->text, fn, lstart);
    push(c, l->indent + 2, 'p', NULL, fn, lstart); // "no reason given"
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
  int comment = 0;

  while (getline(&line, &len, in) != -1)
  {
    lnumber++;

    line_s *l = split(comment, line);

    //printf("** >[1;33m%s[0;00m<\n", line);
    //char *ls = line_s_to_s(l, 1); printf("   %s\n", ls); free(ls);

    comment = l->comment;

    int cindent = -1;
    if (c->node != NULL) cindent = c->node->indent;

    char *head = l->head ? l->head : "";

    if (strcmp(head, "{") == 0 && l->indent == cindent)
    {
      c->node->hasbody = 1;
    }
    else if (strcmp(head, "}") == 0 && l->indent <= cindent)
    {
      //printf("i: %d, ci: %d, ct: %c\n", indent, cindent, ctype);
      pull(c, l->indent, lnumber);
    }
    else if (strcmp(head, "before") == 0 || strcmp(head, "after") == 0)
    {
      char *tline = flu_sprintf("%s %s", l->head, l->text);
      char t = 'b'; // "before each"
      if (strcmp(tline, "before all") == 0) t = 'B';
      else if (strcmp(tline, "before each offline") == 0) t = 'y';
      else if (strcmp(tline, "after each offline") == 0) t = 'z';
      else if (strcmp(tline, "after each") == 0) t = 'a';
      else if (strcmp(tline, "after all") == 0) t = 'A';
      push(c, l->indent, t, tline, path, lnumber);
      free(tline);
    }
    else if (strcmp(head, "describe") == 0)
    {
      push(c, l->indent, 'd', l->text, path, lnumber);
    }
    else if (strcmp(head, "context") == 0)
    {
      push(c, l->indent, 'c', l->text, path, lnumber);
    }
    else if (strcmp(head, "it") == 0 || strcmp(head, "they") == 0)
    {
      push(c, l->indent, 'i', l->text, path, lnumber);
      if (flu_strends(l->line, "{")) c->node->hasbody = 1;
    }
    else if (strcmp(head, "ensure") == 0 || strcmp(head, "expect") == 0)
    {
      lnumber = push_ensure(c, in, l->indent, lnumber, l->line);
    }
    else if (strcmp(head, "pending") == 0)
    {
      push_pending(c, l, path, lnumber);
    }
    else
    {
      push_line(c, line);
    }

    line_s_free(l);
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
  char *type = "none";

  if (t == 'i')
  {
    fprintf(out, "%sint it_%d()\n", ind, n->nodenumber);
    fprintf(out, "%s{\n", ind);

    print_eaches(out, ind, 'b', n->parent);
  }
  else if (offline)
  {
    type = "before_all";
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

    if (n->lines != NULL && strstr(n->lines->string, ") goto _over;\n"))
    {
      fprintf(out, "%s_over:\n", ind);
    }

    print_eaches(out, ind, 'a', n->parent);

    fprintf(out, "\n");
    fprintf(out, "%s  return 1;\n", ind);
    fprintf(out, "%s} // it_%d()\n", ind, n->nodenumber);
  }
  else if (offline)
  {
    fprintf(out, "%s  return 1;\n", ind);
    fprintf(out, "%s} // %s_%d()\n", ind, type, n->nodenumber);
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

  if (c->debug == 0) return; // -d or return

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

void print_nodes(FILE *out, size_t depth, node_s *n)
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

  char *tx = "NULL";
  if (t == 'i' || t == 'd' || t == 'c' || t == 'p') tx = n->text;

  fprintf(
    out,
    "    &(rdz_node)"
    "{ 0, %d, %d, %zu, %s, '%c', \"%s\", %d, %d, %d, \"%s\", %s },\n",
    n->nodenumber,
    n->parent != NULL ? n->parent->nodenumber : -1,
    depth,
    children,
    t,
    n->fname,
    n->lstart, n->ltstart, n->llength, tx, func);

  free(children);
  free(func);

  for (size_t i = 0; n->children[i] != NULL; i++)
  {
    print_nodes(out, depth + 1, n->children[i]);
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
  print_nodes(out, 0, n);
  fprintf(out, "    NULL };\n");
  fprintf(out, "\n");

  int count = c->encount + c->itcount;
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

void add_spec_path(flu_list *l, char *path)
{
  if (flu_strends(path, ".")) return;

  if (flu_strends(path, "_spec.c")) {
    if (access(path, F_OK) == 0) flu_list_add_unique(l, strdup(path));
    return;
  }

  DIR *dir = opendir(path);

  if (dir == NULL) return;

  struct dirent *ep;
  while ((ep = readdir(dir)) != NULL)
  {
    char *fn = flu_sprintf(
      path[strlen(path) - 1] == '/' ? "%s%s" : "%s/%s", path, ep->d_name);

    add_spec_path(l, fn);
  }

  closedir(dir);
}

flu_list *list_spec_files(int argc, char *argv[])
{
  flu_list *l = flu_list_malloc();

  int no_args = 1;

  for (int i = 1; i < argc; i++)
  {
    if (*argv[i] == '-') continue;

    no_args = 0;

    wordexp_t we;
    wordexp(argv[i], &we, 0);

    for (size_t j = 0; j < we.we_wordc; j++) add_spec_path(l, we.we_wordv[j]);

    wordfree(&we);
  }

  if (no_args) add_spec_path(l, ".");

  flu_list_isort(l, (int (*)(const void *, const void *))strcmp);

  return l;
}


//
// main

char *record_call(int argc, char **argv)
{
  flu_sbuffer *b = flu_sbuffer_malloc();

  for (size_t i = 0; i < argc; ++i)
  {
    flu_sbputs(b, argv[i]); if (i < argc - 1) flu_sbputc(b, ' ');
  }

  return flu_sbuffer_to_string(b);
}

char *grab_git_info(char *path)
{
  char *dir = flu_dirname(path);
  char *r = flu_plines("cd %s && git show | head -5", dir);
  free(dir);

  if (strncmp(r, "fatal:", 6) == 0) { free(r); return NULL; }
  if (strlen(r) < 1) { free(r); return NULL; }
  return r;
}

int print_usage(char *arg0)
{
  fprintf(stderr, "" "\n");
  fprintf(stderr, "# rodzo" "\n");
  fprintf(stderr, "" "\n");
  fprintf(stderr, "%s [-o outfile] [-d] [dirs or spec files]" "\n", arg0);
  fprintf(stderr, "" "\n");
  fprintf(stderr, "  turns a spec fileset into a compilable spec.c file" "\n");
  fprintf(stderr, "" "\n");

  return 1;
}

int main(int argc, char *argv[])
{
  regcomp(
    &ensure_operator_rex,
    " ("
      "((c|d|e|f|o|i|li|lli|u|zu|zd|lu|llu)(!?={1,3}))" "|"
      "(!?[=!~\\^\\$>]={2,3}i?[fF]?)"
    ") ",
    REG_EXTENDED);

  // deal with arguments

  context_s *c = malloc_context();

  char *ginfo = grab_git_info(argv[0]);
  char *call = record_call(argc, argv);

  int badarg = 0;
  for (size_t i = 1; i < argc; ++i)
  {
    if (*argv[i] != '-') continue;
    if (argv[i][1] == 'o') c->out_fname = strdup(argv[i + 1]);
    else if (argv[i][1] == 'd') c->debug = 1;
    else badarg = 1;
  }
  if (badarg) return print_usage(argv[0]);

  if (c->out_fname == NULL) c->out_fname = strdup("spec.c");

  // reads specs, grow tree

  flu_list *fnames = list_spec_files(argc, argv);

  for (flu_node *n = fnames->first; n != NULL; n = n->next)
  {
    char *fname = (char *)n->item;
    printf(". processing %s\n", fname);
    process_lines(c, fname);
  }
  flu_list_free_all(fnames);

  // write

  FILE *out = fopen(c->out_fname, "wb");

  if (out == NULL)
  {
    flu_die(1, "couldn't open %s file for writing", c->out_fname);
  }

  fprintf(out, "\n/* rodzo %s */", RODZO_VERSION);
  if (ginfo) fprintf(out, "\n/*\n%s*/", ginfo); free(ginfo);
  fprintf(out, "\n\n// %s", call); free(call);

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

