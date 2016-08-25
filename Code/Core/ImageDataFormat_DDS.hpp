#pragma once

#include "CoreAPI.hpp"
#include "ImageLoader.hpp"
#include "Array.hpp"


class image_loader_dds : public image_loader_interface
{
public:
  virtual bool LoadImageFromData(slice<void const> RawImageData, image* ResultImage) override;
  virtual bool WriteImageToArray(image* Image, array<uint8>* RawImageData) override;
};

using PFN_CreateImageLoader = image_loader_interface* (*)(allocator_interface* Allocator);
using PFN_DestroyImageLoader = void (*)(allocator_interface* Allocator, image_loader_interface* Loader);

extern "C"
{
  CORE_API image_loader_interface*
  CreateImageLoader_DDS(allocator_interface* Allocator);

  CORE_API void
  DestroyImageLoader_DDS(allocator_interface* Allocator, image_loader_interface* Loader);
}
