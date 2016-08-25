#pragma once

#include "CoreAPI.hpp"
#include "Array.hpp"
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

  array<sub_image> InternalSubImages;
  array<uint8> Data;
};

CORE_API
void
Init(image* Image, allocator_interface* Allocator);

CORE_API
void
Finalize(image* Image);

CORE_API
void
Copy(image* Target, image const& Source);

CORE_API
uint32
ImageNumBlocksX(image const* Image, uint32 MipLevel = 0);

CORE_API
uint32
ImageNumBlocksY(image const* Image, uint32 MipLevel = 0);

CORE_API
uint32
ImageDataSize(image const* Image);


/// \brief Allocates the storage space required for the configured number of sub-images.
///
/// When creating an image, call this method after setting the dimensions and number of mip levels, faces and array indices.
/// Changing the image dimensions or number of sub-images will not automatically reallocate the data.
CORE_API
void
ImageAllocateData(image* Image);

/// \brief Returns the offset in bytes between two subsequent rows of the given mip level.
///
/// This function is only valid to use when the image format is a linear pixel format.
CORE_API
uint32
ImageRowPitch(image const* Image, uint32 MipLevel = 0);

/// \brief Returns the offset in bytes between two subsequent depth slices of the given mip level.
CORE_API
uint32
ImageDepthPitch(image const* Image, uint32 MipLevel = 0);

/// \brief Returns the position in bytes in the data array of the given sub-image.
CORE_API
uint32
ImageDataOffSet(image const* Image, uint32 MipLevel = 0, uint32 Face = 0, uint32 ArrayIndex = 0);

CORE_API
image::sub_image const*
ImageInternalSubImage(image const* Image, uint32 MipLevel, uint32 Face, uint32 ArrayIndex);

CORE_API
image::sub_image*
ImageInternalSubImage(image* Image, uint32 MipLevel, uint32 Face, uint32 ArrayIndex);


template<typename T>
T const*
ImageDataPointer(image const* Image)
{
  return Reinterpret<T const*>(&Image->Data[0]);
}

template<typename T>
T*
ImageDataPointer(image* Image)
{
  return const_cast<T*>(ImageDataPointer<T>(AsPtrToConst(Image)));
}

template<typename T>
T const*
ImageSubImagePointer(image const* Image, uint32 MipLevel, uint32 Face, uint32 ArrayIndex)
{
  return Reinterpret<T const*>(&Image->Data[ImageInternalSubImage(MipLevel, Face, ArrayIndex)->DataOffset]);
}

template<typename T>
T*
ImageSubImagePointer(image* Image, uint32 MipLevel, uint32 Face, uint32 ArrayIndex)
{
  return const_cast<T*>(ImageSubImagePointer<T>(AsPtrToConst(Image), MipLevel, Face, ArrayIndex));
}

template<typename T>
T const*
ImagePixelPointer(image const* Image, uint32 MipLevel, uint32 Face, uint32 ArrayIndex, uint32 X, uint32 Y, uint32 Z)
{
  if(ImageFormatType(Image->Format) != image_format_type::LINEAR)
  {
    LogError("Pixel pointer can only be retrieved for linear formats.");
    Assert(false);
  }
  BoundsCheck(X < Image->Width);
  BoundsCheck(Y < Image->Heighth);
  BoundsCheck(Z < Image->Depth);

  uint8 const* Ptr = ImageSubImagePointer<uint8>(Image, MipLevel, Face, ArrayIndex);

  Ptr += Z * ImageDepthPitch(Image, MipLevel);
  Ptr += Y * ImageRowPitch(Image, MipLevel);
  Ptr += X * ImageFormatBitsPerPixel(Image->Format) / 8;

  return Reinterpret<T const*>(Ptr);
}

template<typename T>
T*
ImagePixelPointer(image* Image, uint32 MipLevel, uint32 Face, uint32 ArrayIndex, uint32 X, uint32 Y, uint32 Z)
{
  return const_cast<T*>(ImagePixelPointer<T>(AsPtrToConst(Image), MipLevel, Face, ArrayIndex, X, Y, Z));
}


template<typename T>
T const*
ImageBlockPointer(image const* Image, uint32 MipLevel, uint32 Face, uint32 ArrayIndex, uint32 BlockX, uint32 BlockY, uint32 Z)
{
  if(ImageFormatType(Image->Format) != image_format_type::BLOCK_COMPRESSED)
  {
    LogError("Block pointer can only be retrieved for block compressed formats.");
    Assert(false);
  }

  uint8 const* BasePointer = ImageSubImagePointer<uint8>(Image, MipLevel, Face, ArrayIndex);

  BasePointer += Z * ImageDepthPitch(Image, MipLevel);

  uint32 const BlockSize = 4;
  uint32 const NumBlocksX = ImageWidth(Image, MipLevel) / BlockSize;
  uint32 const BlockIndex = BlockX + NumBlocksX * BlockY;

  BasePointer += BlockIndex * BlockSize * BlockSize * ImageFormatBitsPerPixel(Image->Format) / 8;
  return Reinterpret<T const*>(BasePointer);
}

template<typename T>
T*
ImageBlockPointer(image* Image, uint32 MipLevel, uint32 Face, uint32 ArrayIndex, uint32 BlockX, uint32 BlockY, uint32 Z)
{
  return const_cast<T*>(ImageBlockPointer<T>(AsPtrToConst(Image), MipLevel, Face, ArrayIndex, BlockX, BlockY, Z));
}

//
// Default Load Functions
//

/// Set's the image data to be a solid color in the given format.
CORE_API
bool
ImageSetAsSolidColor(image* Image, union color_linear const& Color, image_format Format);
