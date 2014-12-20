
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
    it "used to crash when the 1st it had no ensure"
    {
    }
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

    context "==="
    {
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
      it "compares to NULL"
      {
        ensure("ia" === NULL);
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
    }

    context "~=="
    {
      it "matches strings 1"
      {
        ensure(mne_tos(47) ~== "[iI]a");
      }
      it "matches strings 2 (failure)"
      {
        ensure(mne_tos(47) ~== "[xy]a");
      }
      it "matches strings (insensitive)"
      {
        ensure("IA" ~==i "[ijk]a");
      }
      it "understands !~== (hit)"
      {
        ensure("blah" !~== "bl[eu]h");
      }
      it "understands !~== (miss)"
      {
        ensure("blah" !~== "bl[aeu]h");
      }
    }

    context "^=="
    {
      it "starts with (success)"
      {
        ensure("this is true" ^== "this ");
      }
      it "starts with (failure)"
      {
        ensure("this is true" ^== "that ");
      }
      it "starts and ends quickly if the result is NULL"
      {
        ensure(NULL ^== "that ");
      }
      it "starts with (insensitive)"
      {
        ensure("THIS is true" ^==i "this ");
      }
      it "understands !^== (hit)"
      {
        ensure("this is true" !^== "that ");
      }
      it "understands !^== (miss)"
      {
        ensure("this is true" !^== "this ");
      }
    }

    context "$=="
    {
      it "ends with (success)"
      {
        ensure("this is true" $== " true");
      }
      it "ends with (failure)"
      {
        ensure("this is true" $== " false");
      }
      it "ends quickly if the result is NULL"
      {
        ensure(NULL $== " false");
      }
      it "ends with (insensitive)"
      {
        ensure("this is True" $==i "true");
      }
      it "understands !$== (hit)"
      {
        ensure("this is true" !$== " false");
      }
      it "understands !$== (failure)"
      {
        ensure("this is true" !$== " true");
      }
    }

    context ">=="
    {
      it "accepts >== for 'contains' (hit)"
      {
        expect("this is True" >== "is Tru");
      }
      it "accepts >== for 'contains' (miss)"
      {
        expect("this is True" >== "is Fal");
      }
      it "accepts >==i for 'contains, whatever the case'"
      {
        expect("this is True" >==i "is tru");
      }

      it "accepts !>== for 'does not contain' (hit)"
      {
        expect("this is True" !>== "false");
      }
      it "accepts !>== for 'does not contain' (miss)"
      {
        expect("this is True" !>== "is is");
      }
      it "accepts !>==i for 'does not contain, whatever the case'"
      {
        expect("this is True" !>==i "is fal");
      }
    }

    context "===i"
    {
      it "accepts ===i for case insensitive comparison"
      {
        ensure("tRuE" ===i "true");
      }
      it "accepts !==i for case insensitive comparison"
      {
        ensure("tRuE" !==i "false");
      }
    }

    context "typed equals"
    {
      it "succeeds"
      {
        expect(1 i== 1);
      }
      it "fails"
      {
        expect(1 i== 2);
      }
      it "fails 'f'"
      {
        expect(1.0 f== 2.0);
      }
      it "fails 'zd'"
      {
        expect(-1 zd== 0);
      }
    }

    it "accepts empty specs"
    {
      // has to be green
    }
  }
}

