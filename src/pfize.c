
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
#include <string.h>

char *escape(char *line)
{
  int len = strlen(line) - 1;
  line[len] = '\0';
  char *l = malloc(2 * len * sizeof(char));
  char *ll = l;
  for (int i = 0; i < len; i++)
  {
    char c = line[i];
    if (c == '"' || c == '%' || c == '\\') *(ll++) = '\\';
    *(ll++) = c;
  }
  *(ll++) = '\0';
  return l;
}

// arg[1] : function name
// arg[2] : target file name
//
int main(int argc, char *argv[])
{
  char *func_name = argv[1];
  FILE *in = fopen(argv[2], "r");

  char *line = NULL;
  size_t len = 0;

  printf("void %s(FILE *out)\n", func_name);
  printf("{\n");
  printf("  fputs(\n");

  while (getline(&line, &len, in) != -1)
  {
    if (strncmp(line, "//", 2) == 0) continue;

    char *l = escape(line);
    printf("    \"%s\\n\"\n", l);
    free(l);
  }

  free(line);
  fclose(in);

  printf("    , out);\n");
  printf("}\n");
}

