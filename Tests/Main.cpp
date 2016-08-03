
#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

int main(int NumArgs, char* const Args[])
{
  int Result = 0;

  {
    Catch::Session Session;
    Result = Session.run( NumArgs, Args );
  }

  return Result;
}
