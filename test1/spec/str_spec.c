
//
// test1
//
// Tue May  6 06:12:47 JST 2014
//

#define _POSIX_C_SOURCE 200809L
#include "flutil.h"


context "strings"
{
  // testing "they"
  //
  they "have a length"
  {
    ensure(strlen("abc") == 3);
  }
}

