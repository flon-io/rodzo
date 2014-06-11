
//
// testing rodzo
//
// Thu Jun 12 06:15:47 JST 2014
//

#include "flutil.h"


context "functions defined within the specs"
{
  char *get_name(int i)
  {
    return "Kurt";
  }

  it "calls get_name()"
  {
    ensure(get_name(1) === "Kurt");
  }
}

