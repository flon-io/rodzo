
//
// specifying flutil
//
// Sun Dec 29 08:35:42 JST 2013
//

#define _POSIX_C_SOURCE 200809L
#include "flutil.h"


context "pending everything"
{

  it "accepts pending stuff" // not yet implemented
  it "accepts pending stuff 2" // not yet implemented
    //
  pending "accepts pending nodes" // no reason given
  {
    printf("whatever...\n");
  }
    //
  pending "accepts pending nodes without body" // no reason given
    //
  it "accepts explicitely pending stuff" // no reason given
  {
    pending();
  }
    //
  it "accepts explicitely pending stuff with text" // nada
  {
    pending("nada");
  }
    //
  //it "accepts explicitely pending stuff with multiline text"
  //{
  //  pending(
  //    "nada "
  //    "paloma"
  //  );
  //}
}

