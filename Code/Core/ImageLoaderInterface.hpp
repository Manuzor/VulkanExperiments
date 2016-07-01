#pragma once

#include "Allocator.hpp"
#include "DynamicArray.hpp"

#include <Backbone.hpp>

struct image;

class image_loader_interface
{
  virtual bool LoadImageFromData(slice<void> RawImageData, image* ResultImage) = 0;
  virtual bool WriteImageToArray(image* Image, dynamic_array<uint8> const* RawImageData) = 0;
};

using PFN_CreateImageLoader = image_loader_interface* (*)(allocator_interface* Allocator);
using PFN_DestroyImageLoader = void (*)(allocator_interface* Allocator, image_loader_interface* Loader);
