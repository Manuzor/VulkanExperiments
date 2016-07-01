#pragma once

#include "CoreAPI.hpp"
#include "ImageLoaderInterface.hpp"


class image_loader_dds : public image_loader_interface
{
  virtual bool LoadImageFromData(slice<void> RawImageData, image* ResultImage) override;
  virtual bool WriteImageToArray(image* Image, dynamic_array<uint8> const* RawImageData) override;
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
