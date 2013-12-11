
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

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "mnemo.h"


const int syl_count = 47;

char *syls[] = {
  "a",  "i",  "u",  "e",  "o",
  "ka", "ki", "ku", "ke", "ko",
  "sa", "shi", "su", "se", "so",
  "ta", "ti", "tsu", "te", "to",
  "na", "ni", "nu", "ne", "no",
  "ha", "hi", "fu", "he", "ho",
  "ma", "mi", "mu", "me", "mo",
  "ya",       "yu",       "yo",
  "ra", "ri", "ru", "re", "ro",
  "wa",             "we", "wo",
  "n"
};
char *neg = "wi";

void tos(char *s, long l)
{
  long mod = l % syl_count;
  long rest = l / syl_count;

  if (rest > 0) tos(s, rest);

  strncat(s, syls[mod], 3);
}

char *mne_tos(long l)
{
  char *r = calloc(147, sizeof(char));

  if (l < 0)
  {
    strncat(r, neg, 2);
    l = l * -1;
  }

  tos(r, l);

  return r;
}

mne_tol_result tol(int isStart, char *s, long l)
{
  if (s == NULL) return (mne_tol_result){ -1, 0 };

  int blank = (*s == '\0');
  if (isStart && blank) return (mne_tol_result){ 1, 0 };
  if (blank) return (mne_tol_result){ 0, l };

  if (strncmp("wi", s, 2) == 0)
  {
    mne_tol_result mtr = tol(0, s + 2, 0);
    if (mtr.err != 0) return mtr;
    return (mne_tol_result){ 0, -1 * mtr.result };
  }

  for (int i = 0; i < syl_count; i++)
  {
    char *syl = syls[i];
    int len = strlen(syl);
    if (strncmp(syl, s, len) == 0) return tol(0, s + len, syl_count * l + i);
  }

  return (mne_tol_result){ 1, 0 };

  // TODO: set errno?
}

mne_tol_result mne_tol(char *s)
{
  return tol(1, s, 0);
}

int mne_ismnemo(char *s)
{
  mne_tol_result mtr = tol(1, s, 0);

  if (mtr.err == -1) return mtr.err;
  return (mtr.err == 0);
}

/*
int main(int argc, char *argv[])
{
  long l = atol(argv[1]);
  char *s = mne_tos(l);
  printf("%ld -> %s\n", l, s);
  free(s);
}
*/

