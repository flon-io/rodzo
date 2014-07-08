
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "flutil.h"


// flutil.c

//
// str functions

int flu_strends(const char *s, const char *end)
{
  size_t ls = strlen(s);
  size_t le = strlen(end);

  if (le > ls) return 0;

  return (strncmp(s + ls - le, end, le) == 0);
}

char *flu_strrtrim(const char *s)
{
  char *r = strdup(s);
  for (size_t l = strlen(r); l > 0; l--)
  {
    char c = r[l - 1];
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r') r[l - 1] = '\0';
    else break;
  }

  return r;
}

char *flu_strtrim(const char *s)
{
  char *s1 = flu_strrtrim(s);
  char *s2 = s1;
  while (1)
  {
    char c = *s2;
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r') ++s2;
    else break;
  }
  char *r = strdup(s2);
  free(s1);

  return r;
}

//
// sbuffer

flu_sbuffer *flu_sbuffer_malloc()
{
  flu_sbuffer *b = calloc(1, sizeof(flu_sbuffer));
  b->stream = open_memstream(&b->string, &b->len);
  return b;
}

void flu_sbuffer_free(flu_sbuffer *b)
{
  if (b == NULL) return;

  if (b->stream != NULL) fclose(b->stream);
  free(b->string);
  free(b);
}

int flu_sbvprintf(flu_sbuffer *b, const char *format, va_list ap)
{
  if (b->stream == NULL) return 0;
  return vfprintf(b->stream, format, ap);
}

int flu_sbprintf(flu_sbuffer *b, const char *format, ...)
{
  va_list ap;
  va_start(ap, format);
  int r = flu_sbvprintf(b, format, ap);
  va_end(ap);

  return r;
}

int flu_sbputc(flu_sbuffer *b, int c)
{
  return putc(c, b->stream);
}

int flu_sbputs(flu_sbuffer *b, const char *s)
{
  return fputs(s, b->stream);
}

int flu_sbputs_n(flu_sbuffer *b, const char *s, size_t n)
{
  int r = 1;
  for (size_t i = 0; i < n; i++)
  {
    if (s[i] == '\0') break;
    r = putc(s[i], b->stream);
    if (r == EOF) return EOF;
  }

  return r;
}

int flu_sbuffer_close(flu_sbuffer *b)
{
  int r = 0;
  if (b->stream != NULL) r = fclose(b->stream);
  b->stream = NULL;

  return r;
}

char *flu_sbuffer_to_string(flu_sbuffer *b)
{
  //int r = flu_sbuffer_close(b);
  //if (r != 0) return NULL;
  flu_sbuffer_close(b);
    //
    // the string should be NULL, let flow and reach free(b)

  char *s = b->string;
  free(b);

  return s;
}

char *flu_sprintf(const char *format, ...)
{
  va_list ap;
  va_start(ap, format);

  flu_sbuffer *b = flu_sbuffer_malloc();
  flu_sbvprintf(b, format, ap);
  char *s = flu_sbuffer_to_string(b);

  va_end(ap);

  return s;
}


//
// die

void flu_die(int exit_value, const char *format, ...)
{
  va_list ap;
  va_start(ap, format);

  flu_sbuffer *b = flu_sbuffer_malloc();
  flu_sbvprintf(b, format, ap);
  char *s = flu_sbuffer_to_string(b);

  perror(s);

  free(s);

  va_end(ap);

  exit(exit_value);
}


//
// escape

char *flu_escape(const char *s)
{
  return flu_n_escape(s, strlen(s));
}

char *flu_n_escape(const char *s, size_t n)
{
  flu_sbuffer *b = flu_sbuffer_malloc();

  for (size_t i = 0; i < n; i++)
  {
    char c = s[i];
    if (c == '\0') break;
    if (c == '\\') flu_sbprintf(b, "\\\\");
    else if (c == '"') flu_sbprintf(b, "\\\"");
    else if (c == '\b') flu_sbprintf(b, "\\b");
    else if (c == '\f') flu_sbprintf(b, "\\f");
    else if (c == '\n') flu_sbprintf(b, "\\n");
    else if (c == '\r') flu_sbprintf(b, "\\r");
    else if (c == '\t') flu_sbprintf(b, "\\t");
    else flu_sbputc(b, c);
  }

  return flu_sbuffer_to_string(b);
}

char *flu_unescape(const char *s)
{
  return flu_n_unescape(s, strlen(s));
}

// based on cutef8 by Jeff Bezanson
//
char *flu_n_unescape(const char *s, size_t n)
{
  char *d = calloc(n + 1, sizeof(char));

  for (size_t is = 0, id = 0; is < n; is++)
  {
    if (s[is] != '\\') { d[id++] = s[is]; continue; }

    char c = s[is + 1];
    if (c == '\\') d[id++] = '\\';
    else if (c == '"') d[id++] = '"';
    else if (c == 'b') d[id++] = '\b';
    else if (c == 'f') d[id++] = '\f';
    else if (c == 'n') d[id++] = '\n';
    else if (c == 'r') d[id++] = '\r';
    else if (c == 't') d[id++] = '\t';
    else if (c == 'u')
    {
      char *su = strndup(s + is + 2, 4);
      unsigned int u = strtol(su, NULL, 16);
      free(su);
      if (u < 0x80)
      {
        d[id++] = (char)u;
      }
      else if (u < 0x800)
      {
        d[id++] = (u >> 6) | 0xc0;
        d[id++] = (u & 0x3f) | 0x80;
      }
      else //if (u < 0x8000)
      {
        d[id++] = (u >> 12) | 0xe0;
        d[id++] = ((u >> 6) & 0x3f) | 0x80;
        d[id++] = (u & 0x3f) | 0x80;
      }
      is += 4;
    }
    else // leave as is
    {
      d[id++] = '\\'; d[id++] = c;
    }
    is++;
  }

  return d;
}


//
// colls

static flu_node *flu_node_malloc(void *item)
{
  flu_node *n = calloc(1, sizeof(flu_node));
  n->item = item;
  n->next = NULL;
  //n->key = ...

  return n;
}

void flu_node_free(flu_node *n)
{
  //if (n->key != NULL) free(n->key)
  free(n);
}

flu_list *flu_list_malloc()
{
  flu_list *l = calloc(1, sizeof(flu_list));

  l->first = NULL;
  l->last = NULL;
  l->size = 0;

  return l;
}

void flu_list_free(flu_list *l)
{
  for (flu_node *n = l->first; n != NULL; )
  {
    flu_node *next = n->next;
    flu_node_free(n);
    n = next;
  }
  free(l);
}

void flu_list_and_items_free(flu_list *l, void (*free_item)(void *))
{
  for (flu_node *n = l->first; n != NULL; n = n->next) free_item(n->item);
  flu_list_free(l);
}

void *flu_list_at(const flu_list *l, size_t n)
{
  size_t i = 0;
  for (flu_node *no = l->first; no != NULL; no = no->next)
  {
    if (i == n) return no->item;
    ++i;
  }
  return NULL;
}

void flu_list_add(flu_list *l, void *item)
{
  flu_node *n = flu_node_malloc(item);

  if (l->first == NULL) l->first = n;
  if (l->last != NULL) l->last->next = n;
  l->last = n;
  l->size++;
}

int flu_list_add_unique(flu_list *l, void *item)
{
  for (flu_node *n = l->first; n != NULL; n = n->next)
  {
    if (n->item == item) return 0; // not added
  }

  flu_list_add(l, item);
  return 1; // added
}

void flu_list_unshift(flu_list *l, void *item)
{
  flu_node *n = flu_node_malloc(item);
  n->next = l->first;
  l->first = n;
  if (l->last == NULL) l->last = n;
  l->size++;
}

void *flu_list_shift(flu_list *l)
{
  if (l->size == 0) return NULL;

  flu_node *n = l->first;
  void *item = n->item;
  l->first = n->next;
  free(n);
  if (l->first == NULL) l->last = NULL;
  l->size--;

  return item;
}

void **flu_list_to_array(const flu_list *l)
{
  void **a = calloc(l->size, sizeof(void *));
  size_t i = 0;
  for (flu_node *n = l->first; n != NULL; n = n->next) a[i++] = n->item;
  return a;
}

void **flu_list_to_array_n(const flu_list *l)
{
  void **a = calloc(l->size + 1, sizeof(void *));
  size_t i = 0;
  for (flu_node *n = l->first; n != NULL; n = n->next) a[i++] = n->item;
  a[i] = NULL;
  return a;
}


//
// misc

char *flu_strdup(char *s)
{
  int l = strlen(s);
  char *r = calloc(l + 1, sizeof(char));
  strcpy(r, s);
  return r;
}

