#pragma once

#include <Backbone.hpp>

enum class image_format_type
{
  UNKNOWN,
  LINEAR,
  BLOCK_COMPRESSED,
};

enum class image_format
{
  UNKNOWN,

  // 32b per component, 4 components
  R32G32B32A32_TYPELESS,
  R32G32B32A32_FLOAT,
  R32G32B32A32_UINT,
  R32G32B32A32_SINT,

  // 32b per component, 3 components
  R32G32B32_TYPELESS,
  R32G32B32_FLOAT,
  R32G32B32_UINT,
  R32G32B32_SINT,

  // 16b per component, 4 components
  R16G16B16A16_TYPELESS,
  R16G16B16A16_FLOAT,
  R16G16B16A16_UNORM,
  R16G16B16A16_UINT,
  R16G16B16A16_SNORM,
  R16G16B16A16_SINT,

  // 32b per component, 2 components
  R32G32_TYPELESS,
  R32G32_FLOAT,
  R32G32_UINT,
  R32G32_SINT,

  // Pseudo depth-stencil formats
  R32G8X24_TYPELESS,
  D32_FLOAT_S8X24_UINT,
  R32_FLOAT_X8X24_TYPELESS,
  X32_TYPELESS_G8X24_UINT,

  // 10b and 11b per component
  R10G10B10A2_TYPELESS,
  R10G10B10A2_UNORM,
  R10G10B10A2_UINT,
  R10G10B10_XR_BIAS_A2_UNORM,
  R11G11B10_FLOAT,

  // 8b per component, 4 components
  R8G8B8A8_UNORM,
  R8G8B8A8_TYPELESS,
  R8G8B8A8_UNORM_SRGB,
  R8G8B8A8_UINT,
  R8G8B8A8_SNORM,
  R8G8B8A8_SINT,

  B8G8R8A8_UNORM,
  B8G8R8X8_UNORM,
  B8G8R8A8_TYPELESS,
  B8G8R8A8_UNORM_SRGB,
  B8G8R8X8_TYPELESS,
  B8G8R8X8_UNORM_SRGB,

  // 16b per component, 2 components
  R16G16_TYPELESS,
  R16G16_FLOAT,
  R16G16_UNORM,
  R16G16_UINT,
  R16G16_SNORM,
  R16G16_SINT,

  // 32b per component, 1 component
  R32_TYPELESS,
  D32_FLOAT,
  R32_FLOAT,
  R32_UINT,
  R32_SINT,

  // Mixed 24b/8b formats
  R24G8_TYPELESS,
  D24_UNORM_S8_UINT,
  R24_UNORM_X8_TYPELESS,
  X24_TYPELESS_G8_UINT,

  // 8b per component, three components
  B8G8R8_UNORM,

  // 8b per component, two components
  R8G8_TYPELESS,
  R8G8_UNORM,
  R8G8_UINT,
  R8G8_SNORM,
  R8G8_SINT,

  // 5b and 6b per component
  B5G6R5_UNORM,
  B5G5R5A1_UNORM,
  B5G5R5X1_UNORM,

  // 16b per component, one component
  R16_TYPELESS,
  R16_FLOAT,
  D16_UNORM,
  R16_UNORM,
  R16_UINT,
  R16_SNORM,
  R16_SINT,

  // 8b per component, one component
  R8_TYPELESS,
  R8_UNORM,
  R8_UINT,
  R8_SNORM,
  R8_SINT,
  A8_UNORM,

  // 1b per component, one component
  R1_UNORM,
  R9G9B9E5_SHAREDEXP,

  // Block compression formats
  BC1_TYPELESS,
  BC1_UNORM,
  BC1_UNORM_SRGB,
  BC2_TYPELESS,
  BC2_UNORM,
  BC2_UNORM_SRGB,
  BC3_TYPELESS,
  BC3_UNORM,
  BC3_UNORM_SRGB,
  BC4_TYPELESS,
  BC4_UNORM,
  BC4_SNORM,
  BC5_TYPELESS,
  BC5_UNORM,
  BC5_SNORM,
  BC6H_TYPELESS,
  BC6H_UF16,
  BC6H_SF16,
  BC7_TYPELESS,
  BC7_UNORM,
  BC7_UNORM_SRGB,

  // 4b per component
  B4G4R4A4_UNORM,

  NUM
};

constexpr size_t
NumImageFormats() { return Cast<size_t>(image_format::NUM); }

char const*
ImageFormatName(image_format Format);

uint32
ImageFormatBitsPerPixel(image_format Format);

uint32
ImageFormatRedMask(image_format Format);

uint32
ImageFormatGreenMask(image_format Format);

uint32
ImageFormatBlueMask(image_format Format);

uint32
ImageFormatAlphaMask(image_format Format);

image_format_type
ImageFormatType(image_format Format);

image_format
ImageFormatFromPixelMask(uint32 RedMask, uint32 GreenMask, uint32 BlueMask, uint32 AlphaMask,
                         uint32 BitsPerPixel);

uint32
ImageFormatToDxgiFormat(image_format Format);

image_format
ImageFormatFromDxgiFormat(uint32 DxgiFormat);

uint32
ImageFormatToFourCc(image_format Format);

image_format
ImageFormatFromFourCc(uint32 FourCc);
