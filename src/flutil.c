
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
#include <stdlib.h>
#include <stdarg.h>

#include "flutil.h"


// flutil.c

//
// str functions

int flu_strends(char *s, char *end)
{
  size_t ls = strlen(s);
  size_t le = strlen(end);

  if (le > ls) return 0;

  return (strncmp(s + ls - le, end, le) == 0);
}

char *flu_strrtrim(char *s)
{
  char *r = strdup(s);
  for (size_t l = strlen(r); l > 0; l--)
  {
    char c = r[l - 1];
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
      r[l - 1] = '\0';
      break;
    }
  }
  return r;
}

//
// sbuffer

flu_sbuffer *flu_sbuffer_malloc()
{
  flu_sbuffer *b = malloc(sizeof(flu_sbuffer));
  b->stream = open_memstream(&b->string, &b->len);
  return b;
}

void flu_sbuffer_free(flu_sbuffer *b)
{
  if (b == NULL) return;

  if (b->stream != NULL)
  {
    fclose(b->stream);
    //b->stream = NULL;
    free(b->string);
  }
  free(b);
}

int flu_vsbprintf(flu_sbuffer *b, const char *format, va_list ap)
{
  if (b->stream == NULL) return 0;
  return vfprintf(b->stream, format, ap);
}

int flu_sbprintf(flu_sbuffer *b, const char *format, ...)
{
  va_list ap;
  va_start(ap, format);
  int r = flu_vsbprintf(b, format, ap);
  va_end(ap);

  return r;
}

int flu_sbputs(flu_sbuffer *b, char *s)
{
  return fputs(s, b->stream);
}

int flu_sbuffer_close(flu_sbuffer *b)
{
  int r = fclose(b->stream);
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
  flu_vsbprintf(b, format, ap);
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
  flu_vsbprintf(b, format, ap);
  char *s = flu_sbuffer_to_string(b);

  perror(s);

  free(s);

  va_end(ap);

  exit(exit_value);
}

