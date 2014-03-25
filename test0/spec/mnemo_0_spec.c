
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

    it "accepts empty specs"
    {
      // has to be green
    }

    it "accepts pending stuff" // not yet implemented
      //
    //pending "accepts pending nodes"
    //{
    //  printf("whatever...\n");
    //}
      //
    //pending "accepts pending nodes without body"
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
}

