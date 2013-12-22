
//
// specifying aabro
//
// Wed Dec 18 16:19:13 JST 2013
//

#include "aabro.h"

globally
{
  char *global_string = NULL;
}

describe "abr_node"
{
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
      do_something();
    }
    it "returns a string representation of the node"
    {
      //abr_node *n = abr_malloc_node("nada", 0);
      //ensure(strcmp("[ nada, 0 ]", abr_node_to_string(n)) == 0);
      ensure(5 == 5);
    }
    it "returns a string representation of the node"
    {
      //abr_node *n = abr_malloc_node("nada", 0);
      //ensure(strcmp("[ nada, 0 ]", abr_node_to_string(n)) == 0);
      ensure(4 == 5);
    }
  }
}

finally
{
}
literally
{
}

