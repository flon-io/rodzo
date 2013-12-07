
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

  /*
   * zest header
   */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char *ze_last_context = NULL;

void ze_red() { printf("[31m"); }
void ze_green() { printf("[32m"); }
void ze_yellow() { printf("[33m"); }
void ze_white() { printf("[37m"); }
void ze_clear() { printf("[0m"); }

void ze_result(int success, int sc, char *s[], char *fname, int lnumber)
{
  if (ze_last_context == NULL) ze_last_context = malloc(7 * 80 * sizeof(char));

  char *context = s[sc - 2];

  if (strcmp(ze_last_context, context) != 0)
  {
    for (int i = 0; i < sc - 1; i++)
    {
      for (int ii = 0; ii < i; ii++) printf("  "); // indent
      printf("%s\n", s[i]);
    }
  }

  for (int ii = 0; ii < sc - 1; ii++) printf("  "); // indent
  if (success) ze_green();
  else ze_red();
  printf("%s", s[sc - 1]);
  if ( ! success) printf(" (FAILED)");
  printf("\n");
  ze_clear();

  strcpy(ze_last_context, context);
  ze_last_context[strlen(context)] = '\0';
    // avoiding strdup and the posix_source requirement...
}

void ze_summary()
{
  printf("\nTODO: print 'Failures:' summary\n");
}

void ze_free()
{
  if (ze_last_context != NULL) free(ze_last_context);
}

