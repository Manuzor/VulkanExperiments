#pragma once

#include "DynamicArray.hpp"
#include "ImageHeader.hpp"

#include <Backbone.hpp>

/// \brief A class containing image data and associated meta data.
///
/// This class is a lightweight container for image data and the description required for interpreting the data,
/// such as the image format, its dimensions, number of sub-images (i.e. cubemap faces, mip levels and array sub-images).
/// However, it does not provide any methods for interpreting or  modifying of the image data.
///
/// The sub-images are stored in a predefined order compatible with the layout of DDS files, that is, it first stores
/// the mip chain for each image, then all faces in a case of a cubemap, then the individual images of an image array.
struct image : public image_header
{
  struct sub_image
  {
    int RowPitch;
    int DepthPitch;
    int DataOffset;
  };

  dynamic_array<sub_image> InternalSubImages;
  dynamic_array<uint8> Data;
};

void
Init(image* Image, allocator_interface* Allocator);

void
Finalize(image* Image);

uint32
ImageNumBlocksX(image const* Image, uint32 MipLevel = 0);

uint32
ImageNumBlocksY(image const* Image, uint32 MipLevel = 0);

uint32
ImageDataSize(image const* Image);


/// \brief Allocates the storage space required for the configured number of sub-images.
///
/// When creating an image, call this method after setting the dimensions and number of mip levels, faces and array indices.
/// Changing the image dimensions or number of sub-images will not automatically reallocate the data.
void
ImageAllocateData(image* Image);

/// \brief Returns the offset in bytes between two subsequent rows of the given mip level.
///
/// This function is only valid to use when the image format is a linear pixel format.
uint32
ImageRowPitch(image const* Image, uint32 MipLevel = 0);

/// \brief Returns the offset in bytes between two subsequent depth slices of the given mip level.
uint32
ImageDepthPitch(image const* Image, uint32 MipLevel = 0);

/// \brief Returns the position in bytes in the data array of the given sub-image.
uint32
ImageDataOffSet(image const* Image, uint32 MipLevel = 0, uint32 Face = 0, uint32 ArrayIndex = 0);
