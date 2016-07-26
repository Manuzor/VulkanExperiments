#pragma once

#include "CoreAPI.hpp"
#include "Allocator.hpp"
#include "DynamicArray.hpp"

#include <Backbone.hpp>

struct image;

class image_loader_interface
{
public:
  virtual bool LoadImageFromData(slice<void const> RawImageData, image* ResultImage) = 0;
  virtual bool WriteImageToArray(image* Image, dynamic_array<uint8> const* RawImageData) = 0;
};

using PFN_CreateImageLoader = image_loader_interface* (*)(allocator_interface* Allocator);
using PFN_DestroyImageLoader = void (*)(allocator_interface* Allocator, image_loader_interface* Loader);

CORE_API bool
LoadImageFromFile(image_loader_interface* Loader, image* Image, slice<char const> FileName);

struct image_loader_factory
{
  PFN_CreateImageLoader CreateImageLoader;
  PFN_DestroyImageLoader DestroyImageLoader;
};

CORE_API image_loader_factory
ImageLoaderFactoryByFileExtension(slice<char const> FileExtension);
