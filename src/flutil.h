
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

// flutil.h

#include <stdarg.h>
#include <string.h>


//
// str functions

int flu_strends(char *s, char *end);

//
// string_buffer

typedef struct flu_sbuffer {
  FILE *stream;
  char *string;
  size_t len;
} flu_sbuffer;

flu_sbuffer *flu_malloc_sbuffer();

int flu_vsbprintf(flu_sbuffer *b, const char *format, va_list ap);
int flu_sbprintf(flu_sbuffer *b, const char *format, ...);

int flu_sbuffer_close(flu_sbuffer *b);
char *flu_sbuffer_to_string(flu_sbuffer *b);

char *flu_sprintf(const char *format, ...);

