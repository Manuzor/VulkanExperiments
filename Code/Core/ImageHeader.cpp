#include "ImageHeader.hpp"

#include "Log.hpp"

auto
::ImageWidth(image_header const* Header, uint32 MipLevel)
  -> uint32
{
  if(MipLevel >= Header->NumMipLevels)
  {
    LogError("Invalid mip level: %d / %d", MipLevel, Header->NumMipLevels);
    Assert(false);
  }
  return Max(Header->Width >> MipLevel, 1U);
}

auto
::ImageHeight(image_header const* Header, uint32 MipLevel)
  -> uint32
{
  if(MipLevel >= Header->NumMipLevels)
  {
    LogError("Invalid mip level: %d / %d", MipLevel, Header->NumMipLevels);
    Assert(false);
  }
  return Max(Header->Height >> MipLevel, 1U);
}

auto
::ImageDepth(image_header const* Header, uint32 MipLevel)
  -> uint32
{
  if(MipLevel >= Header->NumMipLevels)
  {
    LogError("Invalid mip level: %d / %d", MipLevel, Header->NumMipLevels);
    Assert(false);
  }
  return Max(Header->Depth >> MipLevel, 1U);
}
