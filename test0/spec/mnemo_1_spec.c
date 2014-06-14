
#include <string.h>
#include "./mnemo.h"


describe "mne_tos()" {
  context "birds are flying"
  {
    it "finds the コンビニ convenient"
    {
      ensure(1 == 1);
    }
    it "is OK with \"double quotes\" and \tabs"
    {
      ensure(1 == 2);
        // force text to appear in summary
    }
    it "does not care about \n" {
      ensure(1 == 2);
        // force text to appear in summary
    }

    // that's all folks
  }
}

// in the middle

describe "mne_toi()"
{
  it "flips burgers"
  {
    ensure(1 == 1);
  }
}

