#pragma once

#include "ImageLoader.hpp"
#include "Log.hpp"
#include "String.hpp"

#include <Backbone.hpp>

#include <cstdio>
#include <Windows.h> // TODO: Remove this dependency here.


auto
::LoadImageFromFile(image_loader_interface& Loader, image& Image, slice<char const> FileName)
  -> bool
{
  temp_allocator Allocator;

  arc_string SzFileName(FileName);

  FILE* File = std::fopen(StrPtr(SzFileName), "rb");
  if(File == nullptr)
    return false;

  Defer [=](){ std::fclose(File); };

  auto const BufferMemorySize = MiB(1);
  auto Buffer = SliceAllocate<char>(Allocator, BufferMemorySize);
  if(!Buffer)
    return false;

  Defer [&](){ SliceDeallocate(Allocator, Buffer); };

  array<char> Content(Allocator);
  while(!std::feof(File))
  {
    auto const NumBytesRead = std::fread(Buffer.Ptr, 1, Buffer.Num, File);
    auto NewSlice = ExpandBy(Content, NumBytesRead);
    SliceCopy(NewSlice, AsConst(Buffer));
  }

  auto ContentSlice = Slice(Content);
  auto RawContentSlice = SliceReinterpret<void const>(ContentSlice);
  return Loader.LoadImageFromData(AsConst(RawContentSlice), Image);
}


struct image_loader_factory
{
  allocator_interface* Allocator;
  PFN_CreateImageLoader CreateImageLoader;
  PFN_DestroyImageLoader DestroyImageLoader;
};

struct image_loader_module
{
  arc_string Alias;
  arc_string ModuleName;
  arc_string NameOfCreateLoader;
  arc_string NameOfDestroyLoader;
  arc_string AssociatedFileExtension;
  HMODULE ModuleHandle = nullptr;
  image_loader_factory Factory{};
};

struct image_loader_registry
{
  allocator_interface* Allocator;
  array<image_loader_module*> Modules;
};

auto
::CreateImageLoaderRegistry(allocator_interface& Allocator)
  -> image_loader_registry*
{
  auto Registry = Allocate<image_loader_registry>(Allocator);
  if(Registry)
  {
    MemConstruct(1, Registry);
    Registry->Allocator = &Allocator;
    Registry->Modules.Allocator = &Allocator;
  }

  return Registry;
}

auto
::DestroyImageLoaderRegistry(allocator_interface& Allocator, image_loader_registry* Registry)
  -> void
{
  if(Registry)
  {
    MemDestruct(1, Registry);
    Deallocate(Allocator, Registry);
  }
}

auto
::RegisterImageLoaderModule(image_loader_registry& Registry,
                            slice<char const> Alias,
                            slice<char const> ModuleName,
                            slice<char const> NameOfCreateLoader,
                            slice<char const> NameOfDestroyLoader)
  -> image_loader_module*
{
  for(auto Module : Slice(Registry.Modules))
  {
    if(Slice(Module->Alias) == Alias)
    {
      // TODO: Check for same input values?
      return Module;
    }
  }

  auto NewModule = Allocate<image_loader_module>(*Registry.Allocator);
  MemConstruct(1, NewModule);
  NewModule->Alias = Alias;
  NewModule->ModuleName = ModuleName;
  NewModule->NameOfCreateLoader = NameOfCreateLoader;
  NewModule->NameOfDestroyLoader = NameOfDestroyLoader;
  NewModule->Factory.Allocator = Registry.Allocator;

  Registry.Modules += NewModule;

  return NewModule;
}

auto
::GetImageLoaderModuleByName(image_loader_registry& Registry, slice<char const> ModuleName)
  -> image_loader_module*
{
  for(auto Module : Slice(Registry.Modules))
  {
    if(Slice(Module->Alias) == ModuleName || Slice(Module->ModuleName) == ModuleName)
    {
      return Module;
    }
  }

  return nullptr;
}

auto
::AssociateImageLoaderModuleWithFileExtension(image_loader_module& Module,
                                              slice<char const> FileExtension)
  -> void
{
  // TODO: Support for multiple file extensions?
  Module.AssociatedFileExtension = FileExtension;
}

static void
EnsureImageLoaderFactoryIsReady(image_loader_module* Module)
{
  if(Module->ModuleHandle != nullptr)
    return;

  Module->ModuleHandle = GetModuleHandle(StrPtr(Module->ModuleName));
  if(Module->ModuleHandle == nullptr)
    return;

  Module->Factory.CreateImageLoader = Reinterpret<PFN_CreateImageLoader>(GetProcAddress(Module->ModuleHandle, StrPtr(Module->NameOfCreateLoader)));
  Module->Factory.DestroyImageLoader = Reinterpret<PFN_DestroyImageLoader>(GetProcAddress(Module->ModuleHandle, StrPtr(Module->NameOfDestroyLoader)));
}

auto
::GetImageLoaderFactoryByFileExtension(image_loader_registry& Registry, slice<char const> FileExtension)
  -> image_loader_factory*
{
  if(FileExtension.Num == 0)
    return nullptr;

  // Ignore single leading dot.
  if(FileExtension[0] == '.')
    FileExtension = SliceTrimFront(FileExtension, 1);

  for(auto Module : Slice(Registry.Modules))
  {
    if(StrIsEmpty(Module->AssociatedFileExtension))
      continue;

    auto Associated = Slice(Module->AssociatedFileExtension);

    // Ignore single leading dot.
    if(Associated[0] == '.')
      Associated = SliceTrimFront(Associated, 1);

    if(Associated == FileExtension)
    {
      EnsureImageLoaderFactoryIsReady(Module);
      return &Module->Factory;
    }
  }

  return nullptr;
}

auto
::CreateImageLoader(image_loader_factory& Factory)
  -> image_loader_interface*
{
  return Factory.CreateImageLoader(*Factory.Allocator);
}

auto
::DestroyImageLoader(image_loader_factory& Factory, image_loader_interface* Loader)
  -> void
{
  Factory.DestroyImageLoader(*Factory.Allocator, Loader);
}
