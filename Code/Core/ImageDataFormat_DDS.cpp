#include "ImageDataFormat_DDS.hpp"
#include "Image.hpp"

#include "Log.hpp"


struct dds_pixel_format {
  uint32 Size;
  uint32 Flags;
  uint32 FourCC;
  uint32 RGBBitCount;
  uint32 RBitMask;
  uint32 GBitMask;
  uint32 BBitMask;
  uint32 ABitMask;
};

struct dds_header {
  uint32           Magic;
  uint32           Size;
  uint32           Flags;
  uint32           Height;
  uint32           Width;
  uint32           PitchOrLinearSize;
  uint32           Depth;
  uint32           MipMapCount;
  uint32           Reserved1[11];
  dds_pixel_format Ddspf;
  uint32           Caps;
  uint32           Caps2;
  uint32           Caps3;
  uint32           Caps4;
  uint32           Reserved2;
};

struct dds_resource_dimension {
  enum Enum
  {
    TEXTURE1D = 2,
    TEXTURE2D = 3,
    TEXTURE3D = 4,
  };
};

struct dds_resource_misc_flags {
  enum Enum
  {
    TEXTURECUBE = 0x4,
  };
};

struct dds_header_dxt10 {
  uint32 DxgiFormat;
  uint32 ResourceDimension;
  uint32 MiscFlag;
  uint32 ArraySize;
  uint32 MiscFlags2;
};

struct ddsd_flags
{
  enum Enum {
    CAPS        = 0x000001,
    HEIGHT      = 0x000002,
    WIDTH       = 0x000004,
    PITCH       = 0x000008,
    PIXELFORMAT = 0x001000,
    MIPMAPCOUNT = 0x020000,
    LINEARSIZE  = 0x080000,
    DEPTH       = 0x800000,
  };
};

struct ddpf_flags
{
  enum Enum {
    ALPHAPIXELS = 0x00001,
    ALPHA       = 0x00002,
    FOURCC      = 0x00004,
    RGB         = 0x00040,
    YUV         = 0x00200,
    LUMINANCE   = 0x20000,
  };
};

struct dds_caps
{
  enum Enum {
    COMPLEX = 0x000008,
    MIPMAP  = 0x400000,
    TEXTURE = 0x001000,
  };
};

struct dds_caps2
{
  enum Enum {
    CUBEMAP           = 0x000200,
    CUBEMAP_POSITIVEX = 0x000400,
    CUBEMAP_NEGATIVEX = 0x000800,
    CUBEMAP_POSITIVEY = 0x001000,
    CUBEMAP_NEGATIVEY = 0x002000,
    CUBEMAP_POSITIVEZ = 0x004000,
    CUBEMAP_NEGATIVEZ = 0x008000,
    VOLUME            = 0x200000,
  };
};

constexpr uint32 DdsMagic       = 0x20534444;
constexpr uint32 DdsDxt10FourCc = 0x30315844;


static size_t
ConsumeAndReadInto(slice<void const>* Data, slice<void> Result)
{
  auto Amount = Min(Result.Num, Data->Num);
  if(Amount == 0)
    return 0;

  // Blit the data over.
  MemCopyBytes(Amount, Result.Ptr, Data->Ptr);

  // Trim the data.
  *Data = Slice(*Data, Amount, Data->Num);

  return Amount;
}

template<typename T>
bool
ConsumeAndReadInto(slice<void const>* Data, T* Result)
{
  if(Data->Num < sizeof(T))
    return false;

  auto RawResult = Slice<void>(sizeof(T), Result);
  auto NumBytesRead = ConsumeAndReadInto(Data, RawResult);
  Assert(NumBytesRead == sizeof(T));

  return true;
}

auto
image_loader_dds::LoadImageFromData(slice<void const> RawImageData, image* ResultImage)
  -> bool
{
  if(!RawImageData)
    return false;

  if(ResultImage == nullptr)
    return false;

  dds_header FileHeader;
  if(!ConsumeAndReadInto(&RawImageData, &FileHeader))
  {
    LogError("Failed to read file header.");
    return false;
  }

  if(FileHeader.Magic != DdsMagic)
  {
    LogError("The file is not a recognized DDS file.");
    return false;
  }

  if(FileHeader.Size != 124)
  {
    LogError("The file header size %u doesn't match the expected size of 124.", FileHeader.Size);
    return false;
  }

  // Required in every .dds file. According to the spec, CAPS and PIXELFORMAT are also required, but D3DX outputs
  // files not conforming to this.
  if(
    (FileHeader.Flags & ddsd_flags::WIDTH) == 0 ||
    (FileHeader.Flags & ddsd_flags::HEIGHT) == 0)
  {
    LogError("The file header doesn't specify the mandatory WIDTH or HEIGHT flag.");
    return false;
  }

  if((FileHeader.Caps & dds_caps::TEXTURE) == 0)
  {
    LogError("The file header doesn't specify the mandatory TEXTURE flag.");
    return false;
  }

  bool HasPitch = (FileHeader.Flags & ddsd_flags::PITCH) != 0;

  ResultImage->Width = FileHeader.Width;
  ResultImage->Height = FileHeader.Height;

  if(FileHeader.Ddspf.Size != 32)
  {
    LogError("The pixel format size %u doesn't match the expected value of 32.", FileHeader.Ddspf.Size);
    return false;
  }

  bool IsDxt10 = false;
  dds_header_dxt10 HeaderDxt10;

  image_format format = image_format::UNKNOWN;

  // Data format specified in RGBA masks
  if((FileHeader.Ddspf.Flags & ddpf_flags::ALPHAPIXELS) != 0 || (FileHeader.Ddspf.Flags & ddpf_flags::RGB) != 0)
  {
    format = ImageFormatFromPixelMask(
      FileHeader.Ddspf.RBitMask, FileHeader.Ddspf.GBitMask,
      FileHeader.Ddspf.BBitMask, FileHeader.Ddspf.ABitMask);

    if(format == image_format::UNKNOWN)
    {
      LogError("The pixel mask specified was not recognized (R: %x, G: %x, B: %x, A: %x).",
        FileHeader.Ddspf.RBitMask, FileHeader.Ddspf.GBitMask, FileHeader.Ddspf.BBitMask, FileHeader.Ddspf.ABitMask);
      return false;
    }

    // Verify that the format we found is correct
    if(ImageFormatBitsPerPixel(format) != FileHeader.Ddspf.RGBBitCount)
    {
      LogError("The number of bits per pixel specified in the file (%d) does not match the expected value of %d for the format '%s'.",
        FileHeader.Ddspf.RGBBitCount, ImageFormatBitsPerPixel(format), ImageFormatName(format));
      return false;
    }
  }
  else if((FileHeader.Ddspf.Flags & ddpf_flags::FOURCC) != 0)
  {
    if(FileHeader.Ddspf.FourCC == DdsDxt10FourCc)
    {
      if(ConsumeAndReadInto(&RawImageData, &HeaderDxt10))
      {
        LogError("Failed to read file header.");
        return false;
      }
      IsDxt10 = true;

      format = ImageFormatFromDxgiFormat(HeaderDxt10.DxgiFormat);

      if(format == image_format::UNKNOWN)
      {
        LogError("The DXGI format %u has no equivalent image format.", HeaderDxt10.DxgiFormat);
        return false;
      }
    }
    else
    {
      format = ImageFormatFromFourCc(FileHeader.Ddspf.FourCC);

      if(format == image_format::UNKNOWN)
      {
        LogError("The FourCC code '%c%c%c%c' was not recognized.",
          (FileHeader.Ddspf.FourCC >> 0) & 0xFF,
          (FileHeader.Ddspf.FourCC >> 8) & 0xFF,
          (FileHeader.Ddspf.FourCC >> 16) & 0xFF,
          (FileHeader.Ddspf.FourCC >> 24) & 0xFF);
        return false;
      }
    }
  }
  else
  {
    LogError("The image format is neither specified as a pixel mask nor as a FourCC code.");
    return false;
  }

  ResultImage->Format = format;

  bool IsComplex  = (FileHeader.Caps  & dds_caps::COMPLEX)  != 0;
  bool HasMipMaps = (FileHeader.Caps  & dds_caps::MIPMAP)   != 0;
  bool IsCubeMap  = (FileHeader.Caps2 & dds_caps2::CUBEMAP) != 0;
  bool IsVolume   = (FileHeader.Caps2 & dds_caps2::VOLUME)  != 0;

  // Complex flag must match cubemap or volume flag
  if(IsComplex != (IsCubeMap || IsVolume || HasMipMaps))
  {
    LogError("The header specifies the COMPLEX flag, but has neither mip levels, cubemap faces or depth slices.");
    return false;
  }

  if(HasMipMaps)
  {
    ResultImage->NumMipLevels = FileHeader.MipMapCount;
  }

  // Cubemap and volume texture are mutually exclusive
  if(IsVolume && IsCubeMap)
  {
    LogError("The header specifies both the VOLUME and CUBEMAP flags.");
    return false;
  }

  if(IsCubeMap)
  {
    ResultImage->NumFaces = 6;
  }
  else if(IsVolume)
  {
    ResultImage->Depth = FileHeader.Depth;
  }

  ImageAllocateData(ResultImage);

  // If pitch is specified, it must match the computed value
  if(HasPitch && ImageRowPitch(ResultImage, 0) != FileHeader.PitchOrLinearSize)
  {
    LogError("The row pitch specified in the header doesn't match the expected pitch.");
    return false;
  }

  uint32 DataSize = ImageDataSize(ResultImage);

  if(!ConsumeAndReadInto(&RawImageData, Slice(DataSize, ImageDataPointer<void>(ResultImage))))
  {
    LogError("Failed to read image data.");
    return false;
  }

  return true;

}

auto
image_loader_dds::WriteImageToArray(image* Image, dynamic_array<uint8> const* RawImageData)
  -> bool
{
  return false;
}

auto
::CreateImageLoader_DDS(allocator_interface* Allocator)
  -> image_loader_interface*
{
  auto Loader = Allocate<image_loader_dds>(Allocator);
  MemDefaultConstruct(1, Loader);
  return Loader;
}

auto
::DestroyImageLoader_DDS(allocator_interface* Allocator, image_loader_interface* Loader)
  -> void
{
  MemDestruct(1, Loader);
  Deallocate(Allocator, Loader);
}
