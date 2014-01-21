
//
// specifying aabro
//
// Wed Dec 18 16:19:13 JST 2013
//

#include "aabro.h"

char *global_string = NULL;

before all
{
  printf("0 BEFORE ALL\n");
}
after all
{
  printf("0 AFTER ALL\n");
}

describe "abr_node"
{
  before all
  {
    printf("1 BEFORE ALL\n");
  }
  after all
  {
    printf("1 AFTER ALL\n");
  }
  before each
  {
    printf("0 before each\n");
  }
  after each
  {
    printf("0 after each\n");
  }
  describe "abr_malloc_node()"
  {
    it "creates a node"
    {
      ensure(1 == 2);
    }
  }
  describe "abr_node_to_string(n)"
  {
    before each
    {
      printf("1 before each\n");
    }
    after each
    {
      printf("1 after each\n");
    }
    before each offline
    {
      printf("1 before each offline\n");
    }
    after each offline
    {
      printf("1 after each offline\n");
    }
    it "returns a string representation of the node"
    {
      //abr_node *n = abr_malloc_node("nada", 0);
      //ensure(strcmp("[ nada, 0 ]", abr_node_to_string(n)) == 0);
      printf("in A");
      ensure(5 == 5);
    }
    it "returns a string representation of the node"
    {
      //abr_node *n = abr_malloc_node("nada", 0);
      //ensure(strcmp("[ nada, 0 ]", abr_node_to_string(n)) == 0);
      printf("in B");
      ensure(4 == 5);
    }
  }
}

