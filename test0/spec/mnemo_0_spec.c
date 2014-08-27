
#include <string.h>
#include "./mnemo.h"

//describe 'mne_tos()' do
//
//  it 'turns longs to mnemonic strings' do
//
//    Mne.mne_tos(0).should == 'a'
//    Mne.mne_tos(1).should == 'i'
//    Mne.mne_tos(47).should == 'ia'
//    Mne.mne_tos(-1).should == 'wii'
//    Mne.mne_tos(1234567).should == 'shirerete'
//
//    # so, who frees the strings?
//
//    Mne.mne_tos(-1).class.should == String
//  end
//end

describe "mne_tos()"
{
  context "cows are flying"
  {
    it "turns longs to mnemonic (hopefully) strings"
    {
      for (int i = 0; i < 10; i++)
      {
        //printf("i: %d\n", i);
      }
      ensure(
        strcmp(mne_tos(0), "a") == 0
      );
      ensure(strcmp(mne_tos(7), "FAIL") == 0);
      ensure(strcmp(mne_tos(1), "i") == 0);
      ensure(strcmp(mne_tos(47), "ia") == 0);
      ensure(strcmp(mne_tos(-1), "wii") == 0);
      ensure(strcmp(mne_tos(1234567), "shirerete") == 0);
      ensure(strcmp(mne_tos(0), "a") == 0);
    }
    it "flips \"burgers\""
    {
      ensure(strcmp(mne_tos(0), "a") == 0);
    }
    it "compares strings 1"
    {
      ensure(mne_tos(1) === "i");
    }
    it "compares strings 2"
    {
      ensure(mne_tos(7) === "FAIL");
    }
    it "compares strings 2"
    {
      ensure(
        mne_tos(47) === "ia"
      );
    }
    it "expects strings to differ 1"
    {
      ensure(mne_tos(47) !== "nada");
    }
    it "expects strings to differ 2"
    {
      ensure(mne_tos(47) !== "ia");
    }
    it "frees strings 1"
    {
      ensure(mne_tos(47) ===f "ia");
    }
    it "doesn't crash when strings are NULL"
    {
      ensure(NULL === "ia");
    }

    it "compares empty strings"
    {
      ensure("" === "");
    }
    //it "compares empty strings (2)"
    //{
    //  ensure(NULL === "");
    //}
    //it "compares empty strings (3)"
    //{
    //  void *a = NULL;
    //  ensure(a === "");
    //}

    it "accepts empty specs"
    {
      // has to be green
    }

    it "matches strings 1"
    {
      ensure(mne_tos(47) ~== "[iI]a");
    }
    it "matches strings 2 (fail)"
    {
      ensure(mne_tos(47) ~== "[xy]a");
    }
  }
}

