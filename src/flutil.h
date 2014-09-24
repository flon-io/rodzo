
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

#include <stdio.h>
#include <stdarg.h>


#define FLU_VERSION "1.0.0"

//
// str functions

/* Returns 1 if the string s ends with the end string. Returns 0 else.
 */
int flu_strends(const char *s, const char *end);

/* Returns a copy of the string, trimmed on the right.
 */
char *flu_strrtrim(const char *s);

/* Returns a trimmed copy of the string, left and right.
 */
char *flu_strtrim(const char *s);

/* Returns the index of the first occurence of char c in string s.
 * Starts searching at s + off.
 */
ssize_t flu_index(const char *s, size_t off, char c);

/* Returns the index of the last occurence of char c in string s.
 * Starts searching at s + off.
 * Setting off to -1, is equivalent to setting it to strlen(s) - 1.
 */
ssize_t flu_rindex(const char *s, ssize_t off, char c);


//
// sbuffer

typedef struct flu_sbuffer {
  FILE *stream;
  char *string;
  size_t len;
} flu_sbuffer;

/* Creates a buffer (its stream) and returns a pointer to it.
 */
flu_sbuffer *flu_sbuffer_malloc();

/* Frees the sbuffer (closes the stream and frees the string if necessary).
 */
void flu_sbuffer_free(flu_sbuffer *b);

/* Formats input and writes into buffer. Takes a va_list.
 */
int flu_sbvprintf(flu_sbuffer *b, const char *format, va_list ap);

/* Formats input and writes into buffer. Called like printf is called.
 */
int flu_sbprintf(flu_sbuffer *b, const char *format, ...);

/* Puts a single char to the buffer. It's the equivalent of putc.
 */
int flu_sbputc(flu_sbuffer *b, int c);

/* The equivalent of fputs(). Warning: the buffer is the first argument.
 */
int flu_sbputs(flu_sbuffer *b, const char *s);

/* Puts the n first chars of s to the buffer. Return a non-negative int
 * on success, EOF in case of error.
 * Stops upon encountering a \0.
 */
int flu_sbputs_n(flu_sbuffer *b, const char *s, size_t n);

/* Merely encapsulates a fwrite().
 */
size_t flu_sbwrite(flu_sbuffer *b, const char *s, size_t n);

/* Encapsulates a fwrite().
 */
size_t flu_sbfwrite(flu_sbuffer *b, const void *s, size_t l, size_t n);

/* Closes the buffer (stream) which causes the string to be made available.
 *
 * Doesn't not free the buffer, it still is around for further reading
 * of b->string.
 */
int flu_sbuffer_close(flu_sbuffer *b);

/* Closes the buffer, frees it and returns the pointer to the produced string.
 *
 * Returns NULL in case of issue (buffer should be free anyway).
 */
char *flu_sbuffer_to_string(flu_sbuffer *b);

/* Wraps the sbuffer operations in a single call, yielding the result string.
 *
 * Returns NULL in case of issue.
 */
char *flu_svprintf(const char *format, va_list ap);

/* Wraps the sbuffer operations in a single call, yielding the result string.
 *
 * Returns NULL in case of issue.
 */
char *flu_sprintf(const char *format, ...);


//
// using sbuffer to read the entirety of a file

/* Given a path, reads the content of the path in a new string.
 * Returns NULL if reading failed for any reason.
 */
char *flu_readall(const char *path);

/* Given a file, reads all its content to a new string.
 * Returns NULL if reading failed for any reason.
 */
char *flu_freadall(FILE *in);


//
// flu_list
//
// a minimal list/stack/set with no ambition

typedef struct flu_node {
  struct flu_node *next;
  void *item;
  char *key;
} flu_node;

typedef struct flu_list {
  flu_node *first;
  flu_node *last;
  size_t size;
} flu_list;

#define flu_dict flu_list

/* Creates a new, empty, flu_list
 */
flu_list *flu_list_malloc();

/* Frees a flu_list and all its nodes. But doesn't attempt freeing the
 * items in the nodes.
 */
void flu_list_free(flu_list *l);

/* Frees a flu_list and all its nodes. Calls the given free_item function
 * on each of the items within the nodes.
 */
void flu_list_and_items_free(flu_list *l, void (*free_item)(void *));

/* A shortcut for `flu_list_and_items_free(l, free)`.
 */
void flu_list_free_all(flu_list *l);

/* Returns the nth element in a flu_list. Warning, takes n steps.
 * Returns NULL if n > size of flu_list.
 */
void *flu_list_at(const flu_list *l, size_t n);

//size_t flu_list_indexof(const flu_list *l, void *item);
//int flu_list_contains(const flu_list *l, void *item);

enum // flags for flu_list_to_array()
{
  FLU_F_REVERSE     = 1 << 0, // reverse the order of the returned elements
  FLU_F_EXTRA_NULL  = 1 << 1  // add a final NULL element
};

/* Returns an array of void pointers, pointers to the items in the flu_list.
 * The size of the array is the size of the flu_list.
 * If the flag FLU_REVERSE is set, the array order will be the reverse of
 * the flu_list order.
 * If the flag FLU_EXTRA_NULL is set, the array will have one final extra
 * NULL element.
 */
void **flu_list_to_array(const flu_list *l, int flags);

/* Adds an item at the end of a flu_list.
 */
void flu_list_add(flu_list *l, void *item);

/* Adds an item at the end of a flu_list. Doesn't add if the list already
 * contains the item (well a pointer to the item).
 * Returns 1 if the item was added, 0 if the item was found and not added.
 *
 * This function is used to turn a flu_list into a rickety "set". The
 * bigger the list, the costlier the calls.
 */
int flu_list_add_unique(flu_list *l, void *item);

/* Adds an item at the beginning of the list.
 */
void flu_list_unshift(flu_list *l, void *item);

/* Removes the first item in the list and returns it.
 * Returns NULL if the list is empty.
 */
void *flu_list_shift(flu_list *l);

//void *flu_list_pop(flu_list *l);
//void flu_list_insert(flu_list *l, size_t index, const void *item);

//
// flu_list dictionary functions
//
// Where flu_list is used as a dictionary (warning: no hashing underneath,
// plain unshifting of new bindings).

/* Sets an item under a given key.
 * Unshifts the new binding (O(1)).
 */
void flu_list_set(flu_list *l, const char *key, void *item);

/* Sets an item under a given key, but at then end of the list.
 * Useful for "defaults".
 */
void flu_list_set_last(flu_list *l, const char *key, void *item);

/* Given a key, returns the item bound for it, NULL instead.
 * (O(n)).
 */
void *flu_list_get(flu_list *l, const char *key);

/* Returns a trimmed (a unique value per key) version of the given flu_list
 * dictionary. Meant for iterating over key/values.
 */
flu_list *flu_list_dtrim(flu_list *l);

/* Given a va_list builds a flu_list dict. Is used underneath by flu_d().
 */
flu_list *flu_vd(va_list ap);

/* Given a succession, key/value, key/value, builds a flu_list dict.
 *
 * Warning, it must be stopped with a NULL key, else it'll loop until
 * it has made a dict of all the memory from v0...
 */
flu_list *flu_d(char *k0, void *v0, ...);


//
// escape

/* Returns an escaped copy of the given string.
 * Only escapes \ " \b \f \n \r \t. It doesn't escape UTF-8 chars (the
 * ones above ASCII).
 */
char *flu_escape(const char *s);
char *flu_n_escape(const char *s, size_t n);

/* Returns an unescaped copy of the given string.
 */
char *flu_unescape(const char *s);
char *flu_n_unescape(const char *s, size_t n);


//
// misc

/* Makes the process exit with the given exit_value. Right before
 * that it does perror(msg) where msg is composed with the given format
 * and arguments.
 */
void flu_die(int exit_value, const char *format, ...);

/* For those "warning: implicit declaration of function 'strdup'" times.
 */
char *flu_strdup(char *s);

/* Returns the count of milliseconds (10-3) since the Epoch.
 */
long long flu_getms();

/* Returns the count of microseconds (10-6) since the Epoch.
 */
long long flu_getMs();

#endif // FLON_FLUTIL_H

