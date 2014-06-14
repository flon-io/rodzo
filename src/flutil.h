
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

// flutil.h

#ifndef FLON_FLUTIL_H
#define FLON_FLUTIL_H

#include <stdarg.h>


#define FLU_VERSION "1.0.0"

//
// str functions

/*
 * Returns 1 if the string s ends with the end string. Returns 0 else.
 */
int flu_strends(char *s, char *end);

/*
 * Returns a copy of the string, trimmed on the right.
 */
char *flu_strrtrim(char *s);

/*
 * Returns a trimmed copy of the string, left and right.
 */
char *flu_strtrim(char *s);


//
// sbuffer

typedef struct flu_sbuffer {
  FILE *stream;
  char *string;
  size_t len;
} flu_sbuffer;

/*
 * Creates a buffer (its stream) and returns a pointer to it.
 */
flu_sbuffer *flu_sbuffer_malloc();

/*
 * Frees the sbuffer (closes the stream and frees the string if necessary).
 */
void flu_sbuffer_free(flu_sbuffer *b);

/*
 * Formats input and writes into buffer. Takes a va_list.
 */
int flu_sbvprintf(flu_sbuffer *b, const char *format, va_list ap);

/*
 * Formats input and writes into buffer. Called like printf is called.
 */
int flu_sbprintf(flu_sbuffer *b, const char *format, ...);

/*
 * Puts a single char to the buffer. It's the equivalent of putc.
 */
int flu_sbputc(flu_sbuffer *b, int c);

/*
 * The equivalent of fputs(). Warning: the buffer is the first argument.
 */
int flu_sbputs(flu_sbuffer *b, char *s);

/*
 * Puts the n first chars of s to the buffer. Return a non-negative int
 * on success, EOF in case of error.
 * Stops upon encountering a \0.
 */
int flu_sbputs_n(flu_sbuffer *b, char *s, size_t n);

/*
 * Closes the buffer (stream) which causes the string to be made available.
 *
 * Doesn't not free the buffer, it still is around for further reading
 * of b->string.
 */
int flu_sbuffer_close(flu_sbuffer *b);

/*
 * Closes the buffer, frees it and returns the pointer to the produced string.
 *
 * Returns NULL in case of issue (buffer should be free anyway).
 */
char *flu_sbuffer_to_string(flu_sbuffer *b);

/*
 * Wraps the sbuffer operations in a single call, yielding the result string.
 *
 * Returns NULL in case of issue.
 */
char *flu_sprintf(const char *format, ...);


//
// die

/*
 * Makes the process exit with the given exit_value. Right before
 * that it does perror(msg) where msg is composed with the given format
 * and arguments.
 */
void flu_die(int exit_value, const char *format, ...);


//
// escape

/*
 * Returns an escaped copy of the given string.
 * Only escapes \ " \b \f \n \r \t. It doesn't escape UTF-8 chars (the
 * ones above ASCII).
 */
char *flu_escape(char *s);
char *flu_n_escape(char *s, size_t n);

/*
 * Returns an unescaped copy of the given string.
 */
char *flu_unescape(char *s);
char *flu_n_unescape(char *s, size_t n);

#endif // FLON_FLUTIL_H

