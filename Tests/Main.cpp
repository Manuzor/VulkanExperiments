
#include <Core/Log.hpp>
#include <cstdio>

#define CATCH_CONFIG_RUNNER
#include "TestHeader.hpp"

int main(int NumArgs, char* const Args[])
{
  int Result = 0;

  {
    Catch::Session Session;
    Result = Session.run( NumArgs, Args );
  }

  return Result;
}

auto
::ReadFileContentIntoArray(array<uint8>& Array, char const* FileName)
  -> bool
{
  Clear(Array);

  auto File = std::fopen(FileName, "rb");
  if(File == nullptr)
    return false;

  Defer [File](){ std::fclose(File); };

  size_t const ChunkSize = KiB(4);

  while(true)
  {
    auto NewSlice = ExpandBy(Array, ChunkSize);
    auto const NumBytesRead = std::fread(NewSlice.Ptr, 1, ChunkSize, File);
    auto const Delta = ChunkSize - NumBytesRead;

    if(std::feof(File))
    {
      // Correct the internal array value in case we didn't exactly read a
      // ChunkSize worth of bytes last time.
      Array.Num -= Delta;

      return true;
    }

    if(Delta > 0)
    {
      LogError("Didn't reach the end of file but failed to read any more bytes: %s", FileName);
      return false;
    }
  }
}
