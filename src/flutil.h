
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

/*
 * Returns 1 if the string s ends with the end string. Returns 0 else.
 */
int flu_strends(char *s, char *end);

/*
 * Returns a copy of the string, trimmed on the right.
 */
char *flu_strrtrim(char *s);


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
int flu_vsbprintf(flu_sbuffer *b, const char *format, va_list ap);

/*
 * Formats input and writes into buffer. Called like printf is called.
 */
int flu_sbprintf(flu_sbuffer *b, const char *format, ...);

/*
 * The equivalent of fputs(). Warning: the buffer is the first argument.
 */
int flu_sbputs(flu_sbuffer *b, char *s);

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

