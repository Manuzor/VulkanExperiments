#pragma once

#include "ImageLoader.hpp"

#include <Backbone.hpp>

#include <cstdio>


auto
::LoadImageFromFile(image_loader_interface* Loader,
                    image* Image,
                    allocator_interface* TempAllocator,
                    char const* FileName)
  -> bool
{
  FILE* File = std::fopen(FileName, "rb");
  Defer(=, std::fclose(File));

  const auto BufferMemorySize = MiB(1);
  auto BufferMemory = TempAllocator->Allocate(BufferMemorySize, 1);
  auto Buffer = Slice(BufferMemorySize, Cast<char*>(BufferMemory));
  Defer(=, TempAllocator->Deallocate(BufferMemory));

  scoped_array<char> Content(TempAllocator);
  while(!std::feof(File))
  {
    const auto NumBytesRead = std::fread(Buffer.Ptr, 1, Buffer.Num, File);
    auto NewSlice = ExpandBy(&Content, NumBytesRead);
    SliceCopy(NewSlice, AsConst(Buffer));
  }

  auto ContentSlice = Slice(&Content);
  auto RawContentSlice = SliceReinterpret<void const>(ContentSlice);
  return Loader->LoadImageFromData(AsConst(RawContentSlice), Image);
}
