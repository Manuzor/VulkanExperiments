
// This file was generated using the tool Utilities/GenerateSingleHeader.py
// from the Backbone project.

#include "Backbone.hpp"

#include <cmath>
#include <cstring>


// ===================================
// === Source: Backbone/Common.cpp ===
// ===================================

auto
::Pow(float Base, float Exponent)
  -> float
{
  return std::pow(Base, Exponent);
}

auto
::Pow(double Base, double Exponent)
  -> double
{
  return std::pow(Base, Exponent);
}

auto
::AreNearlyEqual(double A, double B, double Epsilon)
  -> bool
{
  return Abs(A - B) <= Epsilon;
}

auto
::AreNearlyEqual(float A, float B, float Epsilon)
  -> bool
{
  return Abs(A - B) <= Epsilon;
}

// ==================================
// === Source: Backbone/Slice.cpp ===
// ==================================

auto
::SliceFromString(char const* StringPtr)
  -> slice<char const>
{
  auto Seek = StringPtr;
  size_t Count = 0;
  while(*Seek++) ++Count;
  return { Count, StringPtr };
}

auto
::SliceFromString(char* StringPtr)
  -> slice<char>
{
  auto Constified = Coerce<char const*>(StringPtr);
  auto ConstResult = SliceFromString(Constified);
  slice<char> Result;
  Result.Num = ConstResult.Num;
  Result.Ptr = Coerce<char*>(ConstResult.Ptr);
  return Result;
}

// ===================================
// === Source: Backbone/Memory.cpp ===
// ===================================

auto
::MemCopyBytes(size_t NumBytes, void* Destination, void const* Source)
  -> void
{
  // Using memmove so that Destination and Source may overlap.
  std::memmove(Destination, Source, NumBytes);
}

auto
::MemSetBytes(size_t NumBytes, void* Destination, int Value)
  -> void
{
  std::memset(Destination, Value, NumBytes);
}

auto
::MemEqualBytes(size_t NumBytes, void const* A, void const* B)
  -> bool
{
  return MemCompareBytes(NumBytes, A, B) == 0;
}

auto
::MemCompareBytes(size_t NumBytes, void const* A, void const* B)
  -> int
{
  return std::memcmp(A, B, NumBytes);
}

