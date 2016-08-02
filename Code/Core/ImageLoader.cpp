#pragma once

#include "ImageLoader.hpp"
#include "Log.hpp"

#include <Backbone.hpp>

#include <cstdio>
#include <Windows.h> // TODO: Remove this dependency here.


auto
::LoadImageFromFile(image_loader_interface* Loader, image* Image, slice<char const> FileName)
  -> bool
{
  temp_allocator TempAllocator;
  allocator_interface* Allocator = *TempAllocator;

  // Copy FileName here since neither the C-API nor windows accept strings
  // that are sized (i.e. not zero-termianted).
  auto SzFileName = SliceAllocate<char>(Allocator, FileName.Num + 1);
  Defer [=](){ SliceDeallocate(Allocator, SzFileName); };

  SliceCopy(SzFileName, FileName);
  SzFileName[SzFileName.Num - 1] = '\0';

  FILE* File = std::fopen(SzFileName.Ptr, "rb");
  if(File == nullptr)
    return false;

  Defer [=](){ std::fclose(File); };

  const auto BufferMemorySize = MiB(1);
  auto BufferMemory = Allocator->Allocate(BufferMemorySize, 1);
  if(BufferMemory == nullptr)
    return false;

  Defer [=](){ Allocator->Deallocate(BufferMemory); };
  auto Buffer = Slice(BufferMemorySize, Cast<char*>(BufferMemory));

  scoped_array<char> Content(Allocator);
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

auto
::ImageLoaderFactoryByFileExtension(slice<char const> FileExtension)
  -> image_loader_factory
{
  if(FileExtension.Num == 0)
  {
    LogError("Empty file extension.");
    return {};
  }

  if(FileExtension[0] == '.')
  {
    FileExtension = SliceTrimFront(FileExtension, 1);
  }

  if(FileExtension == SliceFromString("dds"))
  {
     return { Reinterpret<PFN_CreateImageLoader>(GetProcAddress(GetModuleHandle(nullptr), "CreateImageLoader_DDS")),
              Reinterpret<PFN_DestroyImageLoader>(GetProcAddress(GetModuleHandle(nullptr), "DestroyImageLoader_DDS")) };
  }

  LogWarning("Do not know a image loader that supported this file extension: %*s",
             Convert<int>(FileExtension.Num), FileExtension.Ptr);
  return {};
}
