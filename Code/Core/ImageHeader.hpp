#pragma once

#include "CoreAPI.hpp"
#include "ImageFormat.hpp"

struct image_header
{
  uint32 NumMipLevels = 1;
  uint32 NumFaces = 1;
  uint32 NumArrayIndices = 1;

  uint32 Width = 0;
  uint32 Height = 0;
  uint32 Depth = 1;

  image_format Format;
};


/// \brief Returns the image width for a given mip level, clamped to 1.
CORE_API
uint32
ImageWidth(image_header const& Header, uint32 MipLevel = 0);

/// \brief Returns the image height for a given mip level, clamped to 1.
CORE_API
uint32
ImageHeight(image_header const& Header, uint32 MipLevel = 0);

/// \brief Returns the image depth for a given mip level, clamped to 1.
CORE_API
uint32
ImageDepth(image_header const& Header, uint32 MipLevel = 0);
