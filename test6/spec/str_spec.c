
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
    expect(get_name(1) === "Kurt");
  }

  // fighting gh-10
  //
  it "is ok with comments after an ensure"
  {
    expect("1" === "1"); // easy
  }
  it "does that thing"
  {
    expect(2 == 2); // peasy
  }
  it "does it again"
  {
    expect(1 == 1);
  }

  // fighting gh-25
  //
  it "doesn't stumble on formats within the expect"
  {
    expect(flu_sprintf("type %s", "t") ===f "type t");
  }
}

