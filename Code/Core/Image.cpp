#include "Image.hpp"

#include "Log.hpp"
#include "Color.hpp"


static bool
ImageValidateSubImageIndices(image const* Image, uint32 MipLevel, uint32 Face, uint32 ArrayIndex)
{
  if(MipLevel >= Image->NumMipLevels)
  {
    LogError("Invalid Mip Level: %d / %d", MipLevel, Image->NumMipLevels);
    return false;
  }
  if(Face >= Image->NumFaces)
  {
    LogError("Invalid Face: %d / %d", Face, Image->NumFaces);
    return false;
  }
  if(ArrayIndex >= Image->NumArrayIndices)
  {
    LogError("Invalid Array Slice: %d / %d", ArrayIndex, Image->NumArrayIndices);
    return false;
  }

  return true;
}

image::sub_image const*
::ImageInternalSubImage(image const* Image, uint32 MipLevel, uint32 Face, uint32 ArrayIndex)
{
  if(!ImageValidateSubImageIndices(Image, MipLevel, Face, ArrayIndex))
    return nullptr;

  auto Index = MipLevel + Image->NumMipLevels * (Face + Image->NumFaces * ArrayIndex);
  return &Image->InternalSubImages[Index];
}

image::sub_image*
::ImageInternalSubImage(image* Image, uint32 MipLevel, uint32 Face, uint32 ArrayIndex)
{
  if(!ImageValidateSubImageIndices(Image, MipLevel, Face, ArrayIndex))
    return nullptr;

  auto Index = MipLevel + Image->NumMipLevels * (Face + Image->NumFaces * ArrayIndex);
  return &Image->InternalSubImages[Index];
}

auto
::Init(image* Image, allocator_interface* Allocator)
  -> void
{
  Init(&Image->InternalSubImages, Allocator);
  Init(&Image->Data, Allocator);
}

auto
::Finalize(image* Image)
  -> void
{
  Finalize(&Image->Data);
  Finalize(&Image->InternalSubImages);
}

auto
::Copy(image* Target, image const& Source)
  -> void
{
  // Header is safe to copy completely
  *Reinterpret<image_header*>(Target) = Source;

  Copy(&Target->InternalSubImages, Source.InternalSubImages);
  Copy(&Target->Data, Source.Data);
}

auto
::ImageNumBlocksX(image const* Image, uint32 MipLevel)
  -> uint32
{
  if(ImageFormatType(Image->Format) != image_format_type::BLOCK_COMPRESSED)
  {
    LogError("Number of blocks can only be retrieved for block compressed formats.");
    Assert(false);
  }
  uint32 const BlockSize = 4;
  return (ImageWidth(Image, MipLevel) + BlockSize - 1) / BlockSize;
}

auto
::ImageNumBlocksY(image const* Image, uint32 MipLevel)
  -> uint32
{
  if(ImageFormatType(Image->Format) != image_format_type::BLOCK_COMPRESSED)
  {
    LogError("Number of blocks can only be retrieved for block compressed formats.");
    Assert(false);
  }
  uint32 const BlockSize = 4;
  return (ImageHeight(Image, MipLevel) + BlockSize - 1) / BlockSize;
}

auto
::ImageDataSize(image const* Image)
  -> uint32
{
  if(Image->Data.Num < 16)
    return 0;
  return Convert<uint32>(Image->Data.Num - 16);
}

auto
::ImageAllocateData(image* Image)
  -> void
{
  const auto NumSubImages = Image->NumMipLevels * Image->NumFaces * Image->NumArrayIndices;
  SetNum(&Image->InternalSubImages, NumSubImages);

  int DataSize = 0;

  bool IsCompressed = ImageFormatType(Image->Format) == image_format_type::BLOCK_COMPRESSED;
  uint32 BitsPerPixel = ImageFormatBitsPerPixel(Image->Format);

  for (uint32 ArrayIndex = 0; ArrayIndex < Image->NumArrayIndices; ArrayIndex++)
  {
    for (uint32 Face = 0; Face < Image->NumFaces; Face++)
    {
      for (uint32 MipLevel = 0; MipLevel < Image->NumMipLevels; MipLevel++)
      {
        image::sub_image* SubImage = ImageInternalSubImage(Image, MipLevel, Face, ArrayIndex);

        SubImage->DataOffset = DataSize;

        if (IsCompressed)
        {
          uint32 const BlockSize = 4;
          SubImage->RowPitch = 0;
          SubImage->DepthPitch = ImageNumBlocksX(Image, MipLevel) * ImageNumBlocksY(Image, MipLevel) * BlockSize * BlockSize * BitsPerPixel / 8;
        }
        else
        {
          SubImage->RowPitch = ImageWidth(Image, MipLevel) * BitsPerPixel / 8;
          SubImage->DepthPitch = ImageHeight(Image, MipLevel) * SubImage->RowPitch;
        }

        DataSize += SubImage->DepthPitch * ImageDepth(Image, MipLevel);
      }
    }
  }

  SetNum(&Image->Data, DataSize + 16);
}

auto
::ImageRowPitch(image const* Image, uint32 MipLevel)
  -> uint32
{
  if(ImageFormatType(Image->Format) != image_format_type::LINEAR)
  {
    LogError("Row pitch can only be retrieved for linear formats.");
    Assert(false);
  }

  return ImageInternalSubImage(Image, MipLevel, 0, 0)->RowPitch;
}

auto
::ImageDepthPitch(image const* Image, uint32 MipLevel)
  -> uint32
{
  return ImageInternalSubImage(Image, MipLevel, 0, 0)->DepthPitch;
}

auto
::ImageDataOffSet(image const* Image, uint32 MipLevel, uint32 Face, uint32 ArrayIndex)
  -> uint32
{
  return ImageInternalSubImage(Image, MipLevel, Face, ArrayIndex)->DataOffset;
}

static bool
IsSupportedSolidColorFormat(image_format Format)
{
  switch(Format)
  {
    default:                               return false;
    case image_format::R32G32B32A32_FLOAT: return true;
  }
}

static void
ColorToPixel(color_linear const& Color, image_format Format, slice<float> OutPixelData)
{
  BoundsCheck(OutPixelData.Num >= 4);

  switch(Format)
  {
    default: SliceSet(OutPixelData, 0.0f); return;
    case image_format::R32G32B32A32_FLOAT: SliceCopy(OutPixelData, AsConst(Slice(Color.Data))); return;
  }
}

auto
::ImageSetAsSolidColor(image* Image, color_linear const& Color, image_format Format)
  -> bool
{
  if(!IsSupportedSolidColorFormat(Format))
  {
    LogError("Unsupported image format for use as solid color: %s", ImageFormatName(Format));
    return false;
  }
  Image->Format = Format;

  fixed_block<4, float> Pixel;
  ColorToPixel(Color, Format, Slice(Pixel));

  Image->Width  = 2;
  Image->Height = 2;
  ImageAllocateData(Image);
  float* DestPtr = ImageDataPointer<float>(Image);

  auto const NumPixels = Image->Width * Image->Height;
  for(size_t PixelIndex = 0; PixelIndex < NumPixels; ++PixelIndex)
  {
    MemCopy<float>(4, DestPtr, &Pixel[0]);
    DestPtr += 4;
  }

  return true;
}
