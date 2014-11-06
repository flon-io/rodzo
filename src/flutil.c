
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

// https://github.com/flon-io/flutil

#define _POSIX_C_SOURCE 200809L

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <libgen.h>
#include <errno.h>

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

ssize_t flu_index(const char *s, size_t off, char c)
{
  for (size_t i = off; ; ++i)
  {
    char cc = s[i];
    if (cc == '\0') break;
    if (cc == c) return i;
  }
  return -1;
}

ssize_t flu_rindex(const char *s, ssize_t off, char c)
{
  off = (off == -1) ? strlen(s) - 1 : off;
  for (size_t i = off; ; --i)
  {
    if (s[i] == c) return i;
    if (i < 1) break;
  }
  return -1;
}

flu_list *flu_split(const char *s, const char *delim)
{
  size_t dl = strlen(delim);
  flu_list *r = flu_list_malloc();

  for (char *n = NULL; ; )
  {
    n = strstr(s, delim);

    if (n == NULL) { flu_list_add(r, strdup(s)); break; }

    flu_list_add(r, strndup(s, n - s));
    s = n + dl;
  }

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
  if (b->string != NULL) free(b->string);
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

size_t flu_sbwrite(flu_sbuffer *b, const char *s, size_t n)
{
  return fwrite(s, sizeof(char), n, b->stream);
}

size_t flu_sbfwrite(flu_sbuffer *b, const void *s, size_t l, size_t n)
{
  return fwrite(s, l, n, b->stream);
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
  b->string = NULL;

  free(b);

  return s;
}

char *flu_svprintf(const char *format, va_list ap)
{
  flu_sbuffer *b = flu_sbuffer_malloc();
  flu_sbvprintf(b, format, ap);

  return flu_sbuffer_to_string(b);
}

char *flu_sprintf(const char *format, ...)
{
  va_list ap; va_start(ap, format);
  char *s = flu_svprintf(format, ap);
  va_end(ap);

  return s;
}

//
// readall

char *flu_readall(const char *path, ...)
{
  va_list ap; va_start(ap, path);
  char *s = flu_vreadall(path, ap);
  va_end(ap);

  return s;
}

char *flu_vreadall(const char *path, va_list ap)
{
  char *spath = flu_svprintf(path, ap);

  FILE *in = fopen(spath, "r");

  free(spath);

  if (in == NULL) return NULL;

  char *s = flu_freadall(in);
  fclose(in);

  return s;
}

char *flu_freadall(FILE *in)
{
  if (in == NULL) return NULL;

  flu_sbuffer *b = flu_sbuffer_malloc();
  char rb[4096];

  while (1)
  {
    size_t s = fread(rb, sizeof(char), 4096, in);

    if (s > 0) flu_sbputs_n(b, rb, s);

    if (s < 4096)
    {
      if (feof(in) == 1) break;

      // else error...
      flu_sbuffer_free(b);
      return NULL;
    }
  }

  return flu_sbuffer_to_string(b);
}

int flu_writeall(const char *path, ...)
{
  va_list ap; va_start(ap, path);
  char *spath = flu_svprintf(path, ap);
  char *format = va_arg(ap, char *);

  FILE *f = fopen(spath, "w");
  if (f == NULL) return 0;

  free(spath);

  vfprintf(f, format, ap);
  if (fclose(f) != 0) return 0;

  va_end(ap);

  return 1;
}


//
// "path" functions

int flu_unlink(const char *path, ...)
{
  va_list ap; va_start(ap, path);
  char *spath = flu_svprintf(path, ap);
  va_end(ap);

  int r = unlink(spath);

  free(spath);

  return r;
}

char *flu_canopath(const char *path, ...)
{
  va_list ap; va_start(ap, path);
  char *s = flu_svprintf(path, ap);
  va_end(ap);

  if (s[0] != '/')
  {
    char *cwd = getcwd(NULL, 0);
    char *ss = flu_sprintf("%s/%s", cwd, s);
    free(cwd);
    free(s);
    s = ss;
  }

  char *r = calloc(strlen(s) + 1, sizeof(char));
  *r = '/';
  char *rr = r + 1;

  char *a = s + 1;
  char *b = NULL;

  while (1)
  {
    b = strchr(a, '/');

    size_t l = b ? b + 1 - a : strlen(a);

    size_t dots = 0;
    if (l == 2 && strncmp(a, "./", 2) == 0) dots = 1;
    else if (l == 1 && strncmp(a, "/", 1) == 0) dots = 1;
    else if (l == 1 && strncmp(a, ".\0", 2) == 0) dots = 1;
    else if (l >= 2 && strncmp(a, "..", 2) == 0) dots = 2;

    if (dots == 2 && rr > r + 1)
    {
      *(rr - 1) = 0;
      rr = strrchr(r, '/');
      rr = rr ? rr + 1 : r + 1;
    }
    else if (dots < 1)
    {
      strncpy(rr, a, l);
      rr = rr + l;
    }
    *rr = 0;

    if (b == NULL) break;

    a = b + 1;
  }

  free(s);

  return r;
}

char *flu_dirname(const char *path, ...)
{
  va_list ap; va_start(ap, path);
  char *s = flu_svprintf(path, ap);
  va_end(ap);

  char *dn = dirname(s);
  char *ddn = strdup(dn);
  free(s);

  return ddn;
}

char *flu_basename(const char *path, ...)
{
  va_list ap; va_start(ap, path);
  char *s = flu_svprintf(path, ap);
  char *new_suffix = va_arg(ap, char *);
  va_end(ap);

  if (new_suffix && *new_suffix != '.') { free(s); return NULL; }

  char *bn = basename(s);
  char *dbn = strdup(bn);
  free(s);

  if (new_suffix) strcpy(strrchr(dbn, '.'), new_suffix);

  return dbn;
}

char flu_fstat(const char *path, ...)
{
  va_list ap; va_start(ap, path);
  char *p = flu_svprintf(path, ap);
  va_end(ap);

  struct stat s;
  int r = stat(p, &s);
  free(p);

  if (r == 0) return S_ISDIR(s.st_mode) ? 'd' : 'f';
  else return 0;
}

int flu_move(const char *orig, ...)
{
  va_list ap; va_start(ap, orig);
  char *ori = flu_svprintf(orig, ap);

  if (flu_fstat(ori) == 0) { free(ori); return 1; }

  char *dest = va_arg(ap, char *);
  char *des = flu_svprintf(dest, ap);
  va_end(ap);

  char *np = des;

  if (flu_fstat(des) == 'd')
  {
    char *ob = strdup(ori);
    char *obn = basename(ob);
    np = flu_sprintf("%s/%s", des, obn);
    free(ob);
  }

  int r = rename(ori, np);

  free(ori);
  free(des);
  if (np != des) free(np);

  return r;
}

int flu_mkdir_p(const char *path, ...)
{
  va_list ap; va_start(ap, path);
  char *p = flu_svprintf(path, ap);
  int mode = va_arg(ap, int);
  va_end(ap);

  int r = 0;
  char *pp = NULL;

  for (char *b = p; ; )
  {
    b = strstr(b, "/");
    pp = b ? strndup(p, b - p) : strdup(p);
    r = mkdir(pp, mode);
    if (r != 0 && (errno != EEXIST || flu_fstat(pp) == 'f')) break;
    free(pp); pp = NULL;
    if (b == NULL) { r = 0; errno = 0; break; }
    ++b;
  }

  if (pp) free(pp);
  free(p);

  return r;
}


//
// flu_list

static flu_node *flu_node_malloc(void *item)
{
  flu_node *n = calloc(1, sizeof(flu_node));
  n->item = item;
  n->next = NULL;
  n->key = NULL;

  return n;
}

void flu_node_free(flu_node *n)
{
  if (n->key != NULL) free(n->key);
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

void flu_list_free_all(flu_list *l)
{
  flu_list_and_items_free(l, free);
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

void **flu_list_to_array(const flu_list *l, int flags)
{
  size_t s = l->size + (flags & FLU_F_EXTRA_NULL ? 1 : 0);
  void **a = calloc(s, sizeof(void *));
  size_t i = flags & FLU_F_REVERSE ? l->size - 1 : 0;
  for (flu_node *n = l->first; n != NULL; n = n->next)
  {
    a[i] = n->item;
    i = i + (flags & FLU_F_REVERSE ? -1 : 1);
  }
  return a;
}

static void flu_list_ins(
  flu_list *l, flu_node *n, int (*cmp)(const void *, const void *))
{
  if ( ! l->first) { l->first = n; l->last = n; n->next = NULL; return; }

  for (flu_node **pnn = &l->first; *pnn != NULL; pnn = &((*pnn)->next))
  {
    flu_node *nn = *pnn;
    if (cmp(n->item, nn->item) < 0) { n->next = nn; *pnn = n; return; }
  }

  l->last->next = n; l->last = n; n->next = NULL;
}

void flu_list_isort(flu_list *l, int (*cmp)(const void *, const void *))
{
  flu_list *ll = flu_list_malloc();
  for (flu_node *n = l->first; n != NULL; )
  {
    flu_node *next = n->next;
    flu_list_ins(ll, n, cmp);
    n = next;
  }
  l->first = ll->first;
  l->last = ll->last;
  free(ll);
}

void flu_list_set(flu_list *l, const char *key, void *item)
{
  flu_list_unshift(l, item); l->first->key = strdup(key);
}

void flu_list_set_last(flu_list *l, const char *key, void *item)
{
  flu_list_add(l, item); l->last->key = strdup(key);
}

static flu_node *flu_list_getn(flu_list *l, const char *key)
{
  for (flu_node *n = l->first; n != NULL; n = n->next)
  {
    if (n->key != NULL && strcmp(n->key, key) == 0) return n;
  }
  return NULL;
}

void *flu_list_get(flu_list *l, const char *key)
{
  flu_node *n = flu_list_getn(l, key);

  return n == NULL ? NULL : n->item;
}

flu_list *flu_list_dtrim(flu_list *l)
{
  flu_list *r = flu_list_malloc();

  for (flu_node *n = l->first; n != NULL; n = n->next)
  {
    if (n->key == NULL) continue;
    if (flu_list_getn(r, n->key) != NULL) continue;
    flu_list_add(r, n->item); r->last->key = strdup(n->key);
  }

  return r;
}

flu_list *flu_vd(va_list ap)
{
  flu_list *d = flu_list_malloc();

  while (1)
  {
    char *k = va_arg(ap, char *);
    if (k == NULL) break;
    void *v = va_arg(ap, void *);
    flu_list_set(d, k, v);
  }

  return d;
}

flu_list *flu_d(char *k0, void *v0, ...)
{
  va_list ap; va_start(ap, v0);
  flu_list *d = flu_vd(ap);
  va_end(ap);

  flu_list_set(d, k0, v0);

  return d;
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

char *flu_urlencode(const char *s, ssize_t n)
{
  if (n < 0) n = strlen(s);

  flu_sbuffer *b = flu_sbuffer_malloc();

  for (size_t i = 0; i < n; ++i)
  {
    char c = s[i];

    if (
      (c >= 'a' && c <= 'z') ||
      (c >= 'A' && c <= 'Z') ||
      (c >= '0' && c <= '9') ||
      c == '-' || c == '_' || c == '.' || c == '~'
    )
      flu_sbputc(b, c);
    else
      flu_sbprintf(b, "%%%02x", c);
  }

  return flu_sbuffer_to_string(b);
}

char *flu_urldecode(const char *s, ssize_t n)
{
  if (n < 0) n = strlen(s);

  char *r = calloc(n + 1, sizeof(char));

  for (size_t i = 0, j = 0; i < n; ++j)
  {
    if (s[i] != '%') { r[j] = s[i++]; continue; }

    char *code = strndup(s + i + 1, 2);
    char c = strtol(code, NULL, 16);
    free(code);
    i = i + 3;
    r[j] = c;
  }

  return r;
}


//
// misc

void flu_die(int exit_value, const char *format, ...)
{
  va_list ap;
  va_start(ap, format);

  flu_sbuffer *b = flu_sbuffer_malloc();
  flu_sbvprintf(b, format, ap);

  va_end(ap);

  char *s = flu_sbuffer_to_string(b);

  perror(s);

  free(s);

  exit(exit_value);
}

char *flu_strdup(char *s)
{
  int l = strlen(s);
  char *r = calloc(l + 1, sizeof(char));
  strcpy(r, s);

  return r;
}

int flu_system(const char *cmd, ...)
{
  va_list ap; va_start(ap, cmd); char *c = flu_svprintf(cmd, ap); va_end(ap);

  int r = system(c);

  free(c);

  return r;
}

long long flu_stoll(char *s, size_t l, int base)
{
  char *ss = strndup(s, l);
  long long r = strtoll(ss, NULL, base);
  free(ss);
  //printf("flu_stoll() >%s< in >%s< --> %li\n", s, strndup(s, l), r);

  return r;
}

