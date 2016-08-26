#pragma once

#include "CoreAPI.hpp"
#include "Allocator.hpp"
#include "Array.hpp"

#include <Backbone.hpp>

struct image;

class image_loader_interface
{
public:
  virtual bool LoadImageFromData(slice<void const> RawImageData, image& ResultImage) = 0;
  virtual bool WriteImageToArray(image& Image, array<uint8>& RawImageData) = 0;
};

using PFN_CreateImageLoader = image_loader_interface* (*)(allocator_interface& Allocator);
using PFN_DestroyImageLoader = void (*)(allocator_interface& Allocator, image_loader_interface* Loader);

CORE_API bool
LoadImageFromFile(image_loader_interface& Loader, image& Image, slice<char const> FileName);

struct image_loader_registry;
struct image_loader_module;
struct image_loader_factory;

CORE_API
image_loader_registry*
CreateImageLoaderRegistry(allocator_interface& Allocator);

CORE_API
void
DestroyImageLoaderRegistry(allocator_interface& Allocator, image_loader_registry* Registry);

CORE_API
image_loader_module*
RegisterImageLoaderModule(image_loader_registry& Registry,
                          slice<char const> Alias,
                          slice<char const> ModuleName,
                          slice<char const> NameOfCreateLoader,
                          slice<char const> NameOfDestroyLoader);

CORE_API
image_loader_module*
GetImageLoaderModuleByName(image_loader_registry& Registry, slice<char const> ModuleName);

CORE_API
void
AssociateImageLoaderModuleWithFileExtension(image_loader_module& Module,
                                            slice<char const> FileExtension);

CORE_API
image_loader_factory*
GetImageLoaderFactoryByFileExtension(image_loader_registry& Registry, slice<char const> FileExtension);

CORE_API
image_loader_interface*
CreateImageLoader(image_loader_factory& Factory);

CORE_API
void
DestroyImageLoader(image_loader_factory& Factory, image_loader_interface* Loader);
