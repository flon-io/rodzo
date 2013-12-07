
#include <string.h>
#include "../mnemo/mnemo.h"

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
  }
}

