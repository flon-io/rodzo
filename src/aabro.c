
//
// Copyright (c) 2013-2014, John Mettraux, jmettraux+flon@gmail.com
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <limits.h>

#include "aabro.h"

#define MAX_P_CHILDREN 128
#define MAX_DEPTH 2048


abr_tree *abr_tree_malloc(
  short result,
  size_t offset,
  size_t length,
  char *note,
  abr_parser *p,
  abr_tree *child
)
{
  abr_tree *t = calloc(1, sizeof(abr_tree));

  t->name = (p->name == NULL) ? NULL : strdup(p->name);
  t->result = result;
  t->offset = offset;
  t->length = length;
  t->note = (note == NULL) ? NULL : strdup(note);
  t->parser = p;
  t->sibling = NULL;
  t->child = child;

  return t;
}

void abr_tree_free(abr_tree *t)
{
  if (t->name != NULL) free(t->name);
  if (t->note != NULL) free(t->note);

  for (abr_tree *c = t->child; c != NULL; )
  {
    abr_tree *s = c->sibling;
    abr_tree_free(c);
    c = s;
  }

  free(t);
}

char *abr_tree_string(const char *input, abr_tree *t)
{
  return strndup(input + t->offset, t->length);
}

char *abr_p_names[] = { // const ?
  "string", "regex",
  "rep", "alt", "seq",
  "not", "name", "presence", "absence", "n"
};

void abr_t_to_s(abr_tree *t, const char *input, flu_sbuffer *b, int indent)
{
  for (int i = 0; i < indent; i++) flu_sbprintf(b, "  ");

  if (t == NULL)
  {
    flu_sbprintf(b, "{null}");
    return;
  }

  char *name = "null";
  char *note = "null";
  if (t->name) name = flu_sprintf("\"%s\"", t->name);
  if (t->note) note = flu_sprintf("\"%s\"", t->note);
  //
  flu_sbprintf(
    b,
    "[ %s, %d, %d, %d, %s, \"%s\", ",
    name, t->result, t->offset, t->length, note, abr_p_names[t->parser->type]);
  //
  if (t->name) free(name);
  if (t->note) free(note);

  if (t->child == NULL)
  {
    if (input == NULL || t->result != 1)
    {
      flu_sbprintf(b, "[] ]");
    }
    else
    {
      char *s = flu_n_escape(input + t->offset, t->length);
      flu_sbprintf(b, "\"%s\" ]", s);
      free(s);
    }
    return;
  }

  flu_sbprintf(b, "[");

  for (abr_tree *c = t->child; c != NULL; c = c->sibling)
  {
    if (c != t->child) flu_sbputc(b, ',');
    flu_sbputc(b, '\n');
    abr_t_to_s(c, input, b, indent + 1);
  }

  flu_sbputc(b, '\n');
  for (int i = 0; i < indent; i++) flu_sbprintf(b, "  ");
  flu_sbprintf(b, "] ]");
}

char *abr_tree_to_string(abr_tree *t)
{
  flu_sbuffer *b = flu_sbuffer_malloc();
  abr_t_to_s(t, NULL, b, 0);
  return flu_sbuffer_to_string(b);
}

char *abr_tree_to_string_with_leaves(const char *input, abr_tree *t)
{
  flu_sbuffer *b = flu_sbuffer_malloc();
  abr_t_to_s(t, input, b, 0);
  return flu_sbuffer_to_string(b);
}

//
// the abr_parser methods

static flu_list *abr_p_list(flu_list *l, abr_parser *p)
{
  // using the [not] cheap flu_list_add_unique trick
  // but parsers shan't be that big

  if (l == NULL) l = flu_list_malloc();
  int r = flu_list_add_unique(l, p);
  if (r && p->children) for (size_t i = 0; p->children[i] != NULL; i++)
  {
    abr_p_list(l, p->children[i]);
  }
  return l;
}

static void abr_p_free(void *v)
{
  abr_parser *p = v;

  if (p->name != NULL)
  {
    free(p->name);
  }

  // free the regex if it was created with abr_regex_s(char *s)
  if (p->regex != NULL && p->string != NULL)
  {
     regfree(p->regex);
     free(p->regex);
  }

  if (p->string != NULL)
  {
    free(p->string);
  }

  if (p->children != NULL)
  {
    free(p->children);
  }

  free(p);
}

void abr_parser_free(abr_parser *p)
{
  // list all parsers, then free them

  flu_list *ps = abr_p_list(NULL, p);
  flu_list_and_items_free(ps, abr_p_free);
}

static abr_parser *abr_parser_malloc(unsigned short type, const char *name)
{
  abr_parser *p = calloc(1, sizeof(abr_parser));

  p->name = (name == NULL) ? NULL : strdup(name);
  p->type = type;
  p->string = NULL;
  p->string_length = 0;
  p->regex = NULL;
  p->min = -1; p->max = -1;
  p->children = NULL;

  return p;
}

//
// the builder methods

/*static*/ abr_parser **abr_single_child(abr_parser *p)
{
  abr_parser **children = calloc(2, sizeof(abr_parser *));
  children[0] = p;
  return children;
}

void abr_do_name(abr_parser *named, abr_parser *target)
{
  if (named->name == NULL) return;

  if (target->type == 9)
  {
    if (strcmp(target->name, named->name) != 0) return;
    if (target->children == NULL) target->children = abr_single_child(named);
    return;
  }

  if (target->children == NULL) return;

  for (size_t i = 0; target->children[i] != NULL; i++)
  {
    abr_do_name(named, target->children[i]);
  }
}

abr_parser **abr_list_children(abr_parser *p, abr_parser *child0, va_list ap)
{
  // TODO: eventually, *choke* on MAX_P_CHILDREN
  //       idea: introduce a parser that stands for a [parser building] error...

  abr_parser **ps = calloc(MAX_P_CHILDREN + 1, sizeof(abr_parser *));

  ps[0] = child0;

  size_t i = 1;
  for (; i < MAX_P_CHILDREN; i++)
  {
    ps[i] = va_arg(ap, abr_parser *);
    if (ps[i] == NULL) break;
  }

  abr_parser **children = calloc(i + 1, sizeof(abr_parser *));
  for (size_t j = 0; j < i; j++) children[j] = ps[j];

  free(ps);

  return children;
}

/*
 * string
 * regex
 * repetition
 * alternative
 * sequence
 * not, negation
 * name
 * presence
 * absence
 * placeholder (abr_n)
 */

abr_parser *abr_string(const char *s)
{
  return abr_n_string(NULL, s);
}

abr_parser *abr_n_string(const char *name, const char *s)
{
  abr_parser *p = abr_parser_malloc(0, name);
  p->string = strdup(s);
  p->string_length = strlen(s);
  return p;
}

abr_parser *abr_regex(const char *s)
{
  return abr_n_regex(NULL, s);
}

abr_parser *abr_n_regex(const char *name, const char *s)
{
  abr_parser *p = abr_parser_malloc(1, name);
  p->string = strdup(s); // keep a copy of the original
  p->regex = calloc(1, sizeof(regex_t));
  regcomp(p->regex, p->string, REG_EXTENDED);
  return p;
}

abr_parser *abr_regex_r(regex_t *r)
{
  return abr_n_regex_r(NULL, r);
}

abr_parser *abr_n_regex_r(const char *name, regex_t *r)
{
  abr_parser *p = abr_parser_malloc(1, name);
  p->regex = r;
  return p;
}

abr_parser *abr_rep(abr_parser *p, int min, int max)
{
  return abr_n_rep(NULL, p, min, max);
}

abr_parser *abr_n_rep(const char *name, abr_parser *p, int min, int max)
{
  abr_parser *r = abr_parser_malloc(2, name);
  r->min = min;
  r->max = max;
  r->children = abr_single_child(p);
  abr_do_name(r, p);
  return r;
}

abr_parser *abr_alt(abr_parser *p, ...)
{
  abr_parser *r = abr_parser_malloc(3, NULL);

  va_list l; va_start(l, p);
  r->children = abr_list_children(r, p, l);
  va_end(l);

  return r;
}

abr_parser *abr_n_alt(const char *name, abr_parser *p, ...)
{
  abr_parser *r = abr_parser_malloc(3, name);

  va_list l; va_start(l, p);
  r->children = abr_list_children(r, p, l);
  va_end(l);
  abr_do_name(r, r);

  return r;
}

abr_parser *abr_seq(abr_parser *p, ...)
{
  abr_parser *r = abr_parser_malloc(4, NULL);

  va_list l; va_start(l, p);
  r->children = abr_list_children(r, p, l);
  va_end(l);

  return r;
}

abr_parser *abr_n_seq(const char *name, abr_parser *p, ...)
{
  abr_parser *r = abr_parser_malloc(4, name);

  va_list l; va_start(l, p);
  r->children = abr_list_children(r, p, l);
  va_end(l);
  abr_do_name(r, r);

  return r;
}

abr_parser *abr_name(const char *name, abr_parser *p)
{
  abr_parser *r = abr_parser_malloc(6, name);
  r->children = abr_single_child(p);
  abr_do_name(r, r);
  return r;
}

abr_parser *abr_n(const char *name)
{
  return abr_parser_malloc(9, name);
}

//
// the to_s methods

typedef void abr_p_to_s_func(flu_sbuffer *, flu_list *, int, abr_parser *);

void abr_p_to_s(flu_sbuffer *b, flu_list *seen, int indent, abr_parser *p);

void abr_p_string_to_s(
  flu_sbuffer *b, flu_list *seen, int indent, abr_parser *p)
{
  if (p->name == NULL) flu_sbprintf(b, "abr_string(\"%s\")", p->string);
  else flu_sbprintf(b, "abr_n_string(\"%s\", \"%s\")", p->name, p->string);
}

void abr_p_regex_to_s(
  flu_sbuffer *b, flu_list *seen, int indent, abr_parser *p)
{
  if (p->string == NULL)
  {
    if (p->name == NULL) flu_sbprintf(b, "abr_regex_r(%p)", p->regex);
    else flu_sbprintf(b, "abr_n_regex_r(\"%s\", %p)", p->name, p->regex);
  }
  else
  {
    if (p->name == NULL) flu_sbprintf(b, "abr_regex(\"%s\")", p->string);
    else flu_sbprintf(b, "abr_n_regex(\"%s\", \"%s\")", p->name, p->string);
  }
}

void abr_p_rep_to_s(
  flu_sbuffer *b, flu_list *seen, int indent, abr_parser *p)
{
  if (p->name == NULL)
  {
    flu_sbprintf(b, "abr_rep(\n");
  }
  else
  {
    flu_sbprintf(b, "abr_n_rep(\n");
    for (int i = 0; i < indent + 1; i++) flu_sbprintf(b, "  ");
    flu_sbprintf(b, "\"%s\",\n", p->name);
  }
  abr_p_to_s(b, seen, indent + 1, p->children[0]);
  flu_sbprintf(b, ", %i, %i)", p->min, p->max);
}

void abr_p_wchildren_to_s(
  const char *n, flu_sbuffer *b, flu_list *seen, int indent, abr_parser *p)
{
  if (p->name == NULL)
  {
    flu_sbprintf(b, "abr_%s(\n", n);
  }
  else
  {
    flu_sbprintf(b, "abr_n_%s(\n", n);
    for (int i = 0; i < indent + 1; i++) flu_sbprintf(b, "  ");
    flu_sbprintf(b, "\"%s\",\n", p->name);
  }
  if (p->children != NULL) for (size_t i = 0; ; i++)
  {
    abr_parser *c = p->children[i];
    abr_p_to_s(b, seen, indent + 1, c);
    if (c == NULL) break;
    flu_sbprintf(b, ",\n");
  }
  flu_sbprintf(b, ")");
}

void abr_p_alt_to_s(
  flu_sbuffer *b, flu_list *seen, int indent, abr_parser *p)
{
  abr_p_wchildren_to_s("alt", b, seen, indent, p);
}

void abr_p_seq_to_s(
  flu_sbuffer *b, flu_list *seen, int indent, abr_parser *p)
{
  abr_p_wchildren_to_s("seq", b, seen, indent, p);
}

void abr_p_name_to_s(
  flu_sbuffer *b, flu_list *seen, int indent, abr_parser *p)
{
  flu_sbprintf(b, "abr_name(\n");
  for (int i = 0; i < indent + 1; i++) flu_sbprintf(b, "  ");
  flu_sbprintf(b, "\"%s\",\n", p->name);
  abr_p_to_s(b, seen, indent + 1, p->children[0]);
  flu_sbprintf(b, ")");
}

void abr_p_not_to_s(
  flu_sbuffer *b, flu_list *seen, int indent, abr_parser *p)
{
}

void abr_p_presence_to_s(
  flu_sbuffer *b, flu_list *seen, int indent, abr_parser *p)
{
}

void abr_p_absence_to_s(
  flu_sbuffer *b, flu_list *seen, int indent, abr_parser *p)
{
}

void abr_p_n_to_s(
  flu_sbuffer *b, flu_list *seen, int indent, abr_parser *p)
{
  flu_sbprintf(b, "abr_n(\"%s\")", p->name);
  if (p->children == NULL) flu_sbprintf(b, " /* not linked */", p->name);
  //else flu_sbprintf(b, " /* linked */", p->name);
}

abr_p_to_s_func *abr_p_to_s_funcs[] = { // const ?
  abr_p_string_to_s,
  abr_p_regex_to_s,
  abr_p_rep_to_s,
  abr_p_alt_to_s,
  abr_p_seq_to_s,
  abr_p_not_to_s,
  abr_p_name_to_s,
  abr_p_presence_to_s,
  abr_p_absence_to_s,
  abr_p_n_to_s
};

void abr_p_to_s(flu_sbuffer *b, flu_list *seen, int indent, abr_parser *p)
{
  for (int i = 0; i < indent; i++) flu_sbprintf(b, "  ");
  if (p == NULL)
  {
    flu_sbprintf(b, "NULL");
  }
  else
  {
    int r = flu_list_add_unique(seen, p);
    if (r) abr_p_to_s_funcs[p->type](b, seen, indent, p);
    else flu_sbprintf(b, "abr_n(\"%s\")", p->name);
  }
}

char *abr_parser_to_string(abr_parser *p)
{
  flu_sbuffer *b = flu_sbuffer_malloc();
  flu_list *seen = flu_list_malloc();

  abr_p_to_s(b, seen, 0, p);

  flu_list_free(seen);

  return flu_sbuffer_to_string(b);
}

char *abr_parser_to_s(abr_parser *p)
{
  size_t ccount = 0;
  if (p->children) while (p->children[ccount] != NULL) { ++ccount; }

  char *name = "";
  if (p->name) name = flu_sprintf("'%s' ", p->name);

  char *minmax = "";
  if (p->type == 2) minmax = flu_sprintf(" mn%i mx%i", p->min, p->max);

  char *s = flu_sprintf(
    "%s t%i %sc%i%s",
    abr_p_names[p->type], p->type, name, ccount, minmax);

  if (*name != '\0') free(name);
  if (*minmax != '\0') free(minmax);

  return s;
}

//
// the parse methods

typedef abr_tree *abr_p_func(
  const char *, size_t, size_t, abr_parser *, const abr_conf);
//
abr_tree *abr_do_parse(
  const char *input,
  size_t offset, size_t depth,
  abr_parser *p,
  const abr_conf co);

abr_tree *abr_p_string(
  const char *input,
  size_t offset, size_t depth,
  abr_parser *p,
  const abr_conf co)
{
  char *s = p->string;
  size_t le = p->string_length;

  int su = 1;

  if (strncmp(input + offset, s, le) != 0) { su = 0; le = 0; }

  //free(s);
    // no, it's probably a string literal...
    // let the caller free it if necessary

  return abr_tree_malloc(su, offset, le, NULL, p, NULL);
}

abr_tree *abr_p_regex(
  const char *input,
  size_t offset, size_t depth,
  abr_parser *p,
  const abr_conf co)
{
  regmatch_t ms[1];

  if (regexec(p->regex, input + offset, 1, ms, 0))
  {
    // failure
    return abr_tree_malloc(0, offset, 0, NULL, p, NULL);
  }

  // success
  return abr_tree_malloc(1, offset, ms[0].rm_eo - ms[0].rm_so, NULL, p, NULL);
}

abr_tree *abr_p_rep(
  const char *input,
  size_t offset, size_t depth,
  abr_parser *p,
  const abr_conf co)
{
  short result = 1;
  size_t off = offset;
  size_t length = 0;

  abr_tree *first = NULL;
  abr_tree *prev = NULL;

  size_t count = 0;
  //
  for (; ; count++)
  {
    if (p->max > 0 && count >= p->max) break;
    abr_tree *t = abr_do_parse(input, off, depth + 1, p->children[0], co);

    if (first == NULL) first = t;
    if (prev != NULL) prev->sibling = t;
    prev = t;

    if (t->result < 0) result = -1;
    if (t->result != 1) break;
    off += t->length;
    length += t->length;
  }

  if (result == 1 && count < p->min) result = 0;
  if (result < 0) length = 0;

  return abr_tree_malloc(result, offset, length, NULL, p, first);
}

abr_tree *abr_p_alt(
  const char *input,
  size_t offset, size_t depth,
  abr_parser *p,
  const abr_conf co)
{
  short result = 0;
  size_t length = 0;

  abr_tree *first = NULL;
  abr_tree *prev = NULL;

  for (size_t i = 0; p->children[i] != NULL; i++)
  {
    abr_parser *pc = p->children[i];

    abr_tree *t = abr_do_parse(input, offset, depth + 1, pc, co);

    if (first == NULL) first = t;
    if (prev != NULL) prev->sibling = t;
    prev = t;

    result = t->result;
    if (result < 0) { break; }
    if (result == 1) { length = t->length; break; }
  }

  return abr_tree_malloc(result, offset, length, NULL, p, first);
}

abr_tree *abr_p_seq(
  const char *input,
  size_t offset, size_t depth,
  abr_parser *p,
  const abr_conf co)
{
  short result = 1;
  size_t length = 0;
  size_t off = offset;

  abr_tree *first = NULL;
  abr_tree *prev = NULL;

  for (size_t i = 0; p->children[i] != NULL; i++)
  {
    abr_parser *pc = p->children[i];

    abr_tree *t = abr_do_parse(input, off, depth + 1, pc, co);

    if (first == NULL) first = t;
    if (prev != NULL) prev->sibling = t;
    prev = t;

    if (t->result != 1) { result = t->result; length = 0; break; }
    off += t->length;
    length += t->length;
  }

  return abr_tree_malloc(result, offset, length, NULL, p, first);
}

abr_tree *abr_p_not(
  const char *input,
  size_t offset, size_t depth,
  abr_parser *p,
  const abr_conf co)
{
  // not yet implemented
  return NULL;
}

abr_tree *abr_p_name(
  const char *input,
  size_t offset, size_t depth,
  abr_parser *p,
  const abr_conf co)
{
  abr_tree *t = abr_do_parse(input, offset, depth + 1, p->children[0], co);

  return abr_tree_malloc(t->result, t->offset, t->length, NULL, p, t);
}

abr_tree *abr_p_presence(
  const char *input,
  size_t offset, size_t depth,
  abr_parser *p,
  const abr_conf co)
{
  // not yet implemented
  return NULL;
}

abr_tree *abr_p_absence(
  const char *input,
  size_t offset, size_t depth,
  abr_parser *p,
  const abr_conf co)
{
  // not yet implemented
  return NULL;
}

abr_tree *abr_p_n(
  const char *input,
  size_t offset, size_t depth,
  abr_parser *p,
  const abr_conf co)
{
  if (p->children == NULL)
  {
    char *note = flu_sprintf("unlinked abr_n(\"%s\")", p->name);
    abr_tree *t = abr_tree_malloc(-1, offset, 0, note, p, NULL);
    free(note);
    return t;
  }
  return abr_do_parse(input, offset, depth, p->children[0], co);
}

abr_p_func *abr_p_funcs[] = { // const ?
  abr_p_string,
  abr_p_regex,
  abr_p_rep,
  abr_p_alt,
  abr_p_seq,
  abr_p_not,
  abr_p_name,
  abr_p_presence,
  abr_p_absence,
  abr_p_n
};

abr_tree *abr_do_parse(
  const char *input,
  size_t offset, size_t depth,
  abr_parser *p,
  const abr_conf co)
{
  if (depth > MAX_DEPTH)
  {
    return abr_tree_malloc(
      -1, offset, 0, "too much recursion, parser loop?", p, NULL);
  }

  abr_tree *t = abr_p_funcs[p->type](input, offset, depth, p, co);

  if (co.prune == 0 || t->child == NULL) return t;

  abr_tree *first = t->child;
  t->child = NULL;
  abr_tree **sibling = &t->child;
  abr_tree *next = NULL;
  for (abr_tree *c = first; c != NULL; c = next)
  {
    next = c->sibling;
    c->sibling = NULL;

    if (t->result == 0 || c->result == 0)
    {
      abr_tree_free(c);
    }
    else
    {
      *sibling = c;
      sibling = &c->sibling;
    }
  }

  return t;
}


//
// entry point

abr_tree *abr_parse(const char *input, size_t offset, abr_parser *p)
{
  const abr_conf co = { .prune = 1, .all = 0 };

  return abr_parse_c(input, offset, p, co);
}

abr_tree *abr_parse_all(const char *input, size_t offset, abr_parser *p)
{
  const abr_conf co = { .prune = 1, .all = 1 };

  return abr_parse_c(input, offset, p, co);
}

abr_tree *abr_parse_c(
  const char *input, size_t offset, abr_parser *p, const abr_conf co)
{
  abr_tree *t = abr_do_parse(input, offset, 0, p, co);

  if (co.all == 0) return t;

  // check if all the input got parsed

  if (t->result == 1 && t->length < strlen(input))
  {
    t->result = 0;
    t->note = strdup(""
      "not all the input could be parsed");
  }
  else if (t->result == 1 && t->length > strlen(input))
  {
    t->result = -1;
    t->note = strdup(""
      "something wrong happened, something longer than the input got parsed");
  }

  return t;
}


//
// helper functions

char *abr_error_message(abr_tree *t)
{
  if (t->result == -1 && t->note != NULL) return t->note;

  for (abr_tree *c = t->child; c != NULL; c = c->sibling)
  {
    char *s = abr_error_message(c);
    if (s != NULL) return s;
  }

  return NULL;
}

abr_tree *abr_tree_lookup(abr_tree *t, const char *name)
{
  if (t->name != NULL && strcmp(t->name, name) == 0) return t;

  for (abr_tree *c = t->child; c != NULL; c = c->sibling)
  {
    abr_tree *r = abr_tree_lookup(c, name);
    if (r != NULL) return r;
  }

  return NULL;
}

static void abr_t_list(flu_list *l, abr_tree *t, abr_tree_func *f)
{
  short r = f(t);

  if (r < 0) { return; }
  if (r > 0) { flu_list_add(l, t); return; }

  for (abr_tree *c = t->child; c != NULL; c = c->sibling)
  {
    abr_t_list(l, c, f);
  }
}

flu_list *abr_tree_list(abr_tree *t, abr_tree_func *f)
{
  flu_list *l = flu_list_malloc();

  abr_t_list(l, t, f);

  return l;
}

static void abr_t_list_named(flu_list *l, abr_tree *t, const char *name)
{
  if (t->result != 1) { return; }
  if (t->name && strcmp(t->name, name) == 0) { flu_list_add(l, t); return; }

  for (abr_tree *c = t->child; c != NULL; c = c->sibling)
  {
    abr_t_list_named(l, c, name);
  }
}

flu_list *abr_tree_list_named(abr_tree *t, const char *name)
{
  flu_list *l = flu_list_malloc();

  abr_t_list_named(l, t, name);

  return l;
}

abr_tree **abr_tree_collect(abr_tree *t, abr_tree_func *f)
{
  flu_list *l = abr_tree_list(t, f);

  abr_tree **ts = (abr_tree **)flu_list_to_array_n(l);
  flu_list_free(l);

  return ts;
}

abr_parser *abr_p_child(abr_parser *p, size_t index)
{
  // expensive, but safer...
  // but well, what's the point the abr_parser struct is public...

  if (p->children) for (size_t i = 0; p->children[i] != NULL; i++)
  {
    if (index == 0) return p->children[i];
    --index;
  }

  return NULL;
}

abr_tree *abr_t_child(abr_tree *t, size_t index)
{
  for (abr_tree *c = t->child; c != NULL; c = c->sibling)
  {
    if (index == 0) return c;
    --index;
  }

  return NULL;
}

