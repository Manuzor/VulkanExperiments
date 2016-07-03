
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
::Sqrt(double Value)
  -> double
{
  return std::sqrt(Value);
}

auto
::Sqrt(float Value)
  -> float
{
  return std::sqrt(Value);
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

// ==================================
// === Source: Backbone/Angle.cpp ===
// ==================================


auto
::operator +=(angle& A, angle B)
  -> void
{
  A.InternalData += B.InternalData;
}

auto
::operator -=(angle& A, angle B)
  -> void
{
  A.InternalData -= B.InternalData;
}

auto
::operator *=(angle& A, float Scale)
  -> void
{
  A.InternalData *= Scale;
}

auto
::operator /=(angle& A, float Scale)
  -> void
{
  A.InternalData /= Scale;
}

auto
::IsNormalized(angle Angle)
  -> bool
{
  return Angle.InternalData >= 0.0f && Angle.InternalData < 2.0f * Pi();
}

auto
::Normalized(angle Angle)
  -> angle
{
  return { Wrap(Angle.InternalData, 0.0f, 2.0f * Pi()) };
}

auto
::AngleBetween(angle A, angle B)
  -> angle
{
  // Taken from ezEngine who got it from here:
  // http://gamedev.stackexchange.com/questions/4467/comparing-angles-and-working-out-the-difference
  return { Pi() - Abs(Abs(A.InternalData - B.InternalData) - Pi()) };
}

auto
::AreNearlyEqual(angle A, angle B, angle Epsilon)
  -> bool
{
  return AreNearlyEqual(A.InternalData, B.InternalData, Epsilon.InternalData);
}

auto
::Sin(angle Angle)
  -> float
{
  return std::sin(ToRadians(Angle));
}

auto
::Cos(angle Angle)
  -> float
{
  return std::cos(ToRadians(Angle));
}

auto
::Tan(angle Angle)
  -> float
{
  return std::tan(ToRadians(Angle));
}

auto
::ASin(float A)
  -> angle
{
  return Radians(std::asin(A));
}

auto
::ACos(float A)
  -> angle
{
  return Radians(std::acos(A));
}

auto
::ATan(float A)
  -> angle
{
  return Radians(std::atan(A));
}

auto
::ATan2(float A, float B)
  -> angle
{
  return Radians(std::atan2(A, B));
}

