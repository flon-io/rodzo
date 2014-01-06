
//
// specifying flutil
//
// Sun Dec 29 08:35:42 JST 2013
//

#define _POSIX_C_SOURCE 200809L
#include "flutil.h"


context "str functions"
{
  before each
  {
    char *s = NULL;
  }
  after each
  {
    if (s != NULL) free(s);
  }

  describe "flu_strrtrim(char *s)"
  {
    it "trims on the right"
    {
      s = flu_strrtrim("brown fox \n\t");

      ensure(strcmp("brown fox nada", s) == 0);
    }

    it "doesn't trim when not necessary"
    {
      s = flu_strrtrim("");

      ensure(strcmp("", s) == 0);
    }

    it "returns a new string"
    {
      char *s0 = strdup("");
      s = flu_strrtrim(s0);

      ensure(s != s0);

      free(s0);
    }
  }

  describe "flu_strends(char *s, char *ending)"
  {
    it "returns 1 if the s ends with ending"
    {
      ensure(flu_strends("toto", "to") == 1);
    }

    it "returns 0 else"
    {
      ensure(flu_strends("toto", "thenada") == 0);
    }
  }
}

