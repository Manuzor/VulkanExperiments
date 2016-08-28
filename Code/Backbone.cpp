
// This file was generated using the tool Utilities/GenerateSingleHeader.py
// from the Backbone project.

#include "Backbone.hpp"

#include <cmath>
#include <cstring>


// ===================================
// === Source: Backbone/Common.cpp ===
// ===================================

auto
::operator +=(memory_size& A, memory_size B)
  -> void
{
  A.InternalBytes += B.InternalBytes;
}

auto
::operator -=(memory_size& A, memory_size B)
  -> void
{
  A.InternalBytes -= B.InternalBytes;
}

auto
::operator *=(memory_size& A, uint64 Scale)
  -> void
{
  A.InternalBytes *= Scale;
}

auto
::operator /=(memory_size& A, uint64 Scale)
  -> void
{
  A.InternalBytes /= Scale;
}

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
::Mod(double Value, double Divisor)
  -> double
{
  return std::fmod(Value, Divisor);
}

auto
::Mod(float Value, float Divisor)
  -> float
{
  return std::fmod(Value, Divisor);
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
::InvSqrt(float Value)
  -> float
{
  union FloatInt
  {
    float Float;
    int Int;
  };
  FloatInt MagicNumber;
  float HalfValue;
  float Result;
  const float ThreeHalfs = 1.5f;

  HalfValue = Value * 0.5f;
  Result = Value;
  MagicNumber.Float = Result;                               // evil floating point bit level hacking
  MagicNumber.Int  = 0x5f3759df - ( MagicNumber.Int >> 1 ); // what the fuck?
  Result = MagicNumber.Float;
  Result = Result * ( ThreeHalfs - ( HalfValue * Result * Result ) ); // 1st iteration

  return Result;
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
::MemCopyBytes(memory_size Size, void* Destination, void const* Source)
  -> void
{
  // Using memmove so that Destination and Source may overlap.
  std::memmove(Destination, Source, ToBytes(Size));
}

auto
::MemSetBytes(memory_size Size, void* Destination, int Value)
  -> void
{
  std::memset(Destination, Value, ToBytes(Size));
}

auto
::MemEqualBytes(memory_size Size, void const* A, void const* B)
  -> bool
{
  return MemCompareBytes(Size, A, B) == 0;
}

auto
::MemCompareBytes(memory_size Size, void const* A, void const* B)
  -> int
{
  return std::memcmp(A, B, ToBytes(Size));
}

auto
::MemAreOverlapping(memory_size SizeA, void const* A, memory_size SizeB, void const* B)
  -> bool
{
  auto LeftA = Reinterpret<size_t const>(A);
  auto RightA = LeftA + ToBytes(SizeA);

  auto LeftB = Reinterpret<size_t const>(B);
  auto RightB = LeftB + ToBytes(SizeB);

  return LeftB  >= LeftA && LeftB  <= RightA || // Check if LeftB  is in [A, A+NumBytesA]
         RightB >= LeftA && RightB <= RightA || // Check if RightB is in [A, A+NumBytesA]
         LeftA  >= LeftB && LeftA  <= RightB || // Check if LeftA  is in [B, B+NumBytesB]
         RightA >= LeftB && RightA <= RightB;   // Check if RightA is in [B, B+NumBytesB]
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

// =============================================
// === Source: Backbone/StringConversion.cpp ===
// =============================================

static slice<char const>
TrimWhitespaceFront(slice<char const> String)
{
  while(true)
  {
    if(String.Num == 0)
      return String;

    if(!IsWhitespace(String[0]))
      return String;

    String = SliceTrimFront(String, 1);
  }
}

auto
::ImplConvertStringToDouble( slice<char const>* Source, bool* Success, double Fallback)
  -> double
{
  if(Source == nullptr)
  {
    if(Success)
      *Success = false;

    return Fallback;
  }

  auto String = *Source;
  String = TrimWhitespaceFront(String);

  bool Sign = false;

  if(String.Num == 0)
  {
    if(Success)
      *Success = false;

    return Fallback;
  }

  switch(String[0])
  {
  case '+':
    String = SliceTrimFront(String, 1);
    break;
  case '-':
    Sign = true;
    String = SliceTrimFront(String, 1);
    break;
  default:
    break;
  }

  uint64 NumericalPart = 0;
  bool HasNumericalPart = false;

  while(String.Num > 0 && IsDigit(String[0]))
  {
    NumericalPart *= 10;
    NumericalPart += String[0] - '0';
    HasNumericalPart = true;
    String = SliceTrimFront(String, 1);
  }

  if (!HasNumericalPart)
  {
    if(Success)
      *Success = false;

    return Fallback;
  }

  auto Value = (double)NumericalPart;

  bool HasDecimalPoint = String.Num > 0 && String[0] == '.';

  if(!HasDecimalPoint && String.Num == 0 || (String[0] != '.' && String[0] != 'e' && String[0] != 'E'))
  {
    if(Success)
      *Success = true;

    *Source = String;
    return Sign ? -Value : Value;
  }

  if(HasDecimalPoint)
  {
    String = SliceTrimFront(String, 1);
    uint64 DecimalPart = 0;
    uint64 DecimalDivider = 1;

    while(String.Num > 0 && IsDigit(String[0]))
    {
        DecimalPart *= 10;
        DecimalPart += String[0] - '0';
        DecimalDivider *= 10;
        String = SliceTrimFront(String, 1);
    }

    Value += (double)DecimalPart / (double)DecimalDivider;
  }

  if(String.Num == 0 || (String[0] != 'e' && String[0] != 'E'))
  {
    if(Success)
      *Success = true;

    *Source = String;
    return Sign ? -Value : Value;
  }
  else if(String[0] == 'e' || String[0] == 'E')
  {
    String = SliceTrimFront(String, 1);
    bool ExponentSign = false;

    switch(String[0])
    {
    case '+':
      String = SliceTrimFront(String, 1);
      break;
    case '-':
      ExponentSign = true;
      String = SliceTrimFront(String, 1);
      break;
    default:
      break;
    }

    uint64 ExponentPart = 0;
    while(String.Num > 0 && IsDigit(String[0]))
    {
        ExponentPart *= 10;
        ExponentPart += String[0] - '0';
        String = SliceTrimFront(String, 1);
    }

    int64 ExponentValue = 1;
    while(ExponentPart > 0)
    {
      ExponentValue *= 10;
      --ExponentPart;
    }

    Value = (ExponentSign ? (Value / ExponentValue) : (Value * ExponentValue));
  }

  if(Success)
    *Success = true;

  *Source = String;
  return Sign ? -Value : Value;
}

template<typename IntegerType>
IntegerType
ImplConvertStringToIntegerHelper(slice<char const>* Source, bool* Success, IntegerType Fallback)
{
  if(Source == nullptr)
  {
    if(Success)
      *Success = false;

    return Fallback;
  }

  auto String = *Source;
  String = TrimWhitespaceFront(String);

  bool Sign = false;

  if(String.Num == 0)
  {
    if(Success)
      *Success = false;

    return Fallback;
  }

  switch(String[0])
  {
  case '+':
    String = SliceTrimFront(String, 1);
    break;
  case '-':
    Sign = true;
    String = SliceTrimFront(String, 1);
    break;
  default:
    break;
  }

  uint64 NumericalPart = 0;
  bool HasNumericalPart = false;

  while(String.Num > 0 && IsDigit(String[0]))
  {
    NumericalPart *= 10;
    NumericalPart += String[0] - '0';
    HasNumericalPart = true;
    String = SliceTrimFront(String, 1);
  }

  if (!HasNumericalPart)
  {
    if(Success)
      *Success = false;

    return Fallback;
  }

  auto Value = Convert<IntegerType>(NumericalPart);

  if(Sign)
  {
    if(IntIsSigned<IntegerType>())
    {
      Value = Negate(Value);
    }
    else
    {
      // Unsigned types cannot have a '-' sign.

      if(Success)
        *Success = false;

      return Fallback;
    }
  }

  if(Success)
    *Success = true;

  *Source = String;
  return Value;
}

auto
::ImplConvertStringToInteger(slice<char const>* Source, bool* Success, uint64 Fallback)
  -> uint64
{
  return ImplConvertStringToIntegerHelper<uint64>(Source, Success, Fallback);
}

auto
::ImplConvertStringToInteger(slice<char const>* Source, bool* Success, int64 Fallback)
  -> int64
{
  return ImplConvertStringToIntegerHelper<int64>(Source, Success, Fallback);
}

template<typename IntegerType>
slice<char>
ImplConvertIntegerToStringHelper(IntegerType Integer, slice<char> Buffer, bool* Success)
{
  if(Integer == 0)
  {
    Buffer[0] = '0';

    if(Success)
      *Success = true;

    return Slice(Buffer, 0, 1);
  }

  size_t NumChars = 0;

  if(Integer < 0)
  {
    Buffer[NumChars] = '-';
    ++NumChars;
    Integer = Negate(Integer);
  }

  while(Integer > 0)
  {
    auto const FirstDigit = Integer % IntegerType(10);
    Buffer[NumChars] = '0' + Cast<char>(FirstDigit);
    ++NumChars;
    Integer /= 10;
  }

  auto Result = Slice(Buffer, 0, NumChars);

  // Result now contains the digits in reverse order, so we swap them around.
  SliceReverseElements(Result);

  if(Success)
    *Success = true;

  return Result;
}

auto
::ImplConvertIntegerToString(int8 Integer, slice<char> Buffer, bool* Success)
  -> slice<char>
{
  return ImplConvertIntegerToStringHelper<int8>(Integer, Buffer, Success);
}

auto
::ImplConvertIntegerToString(int16 Integer, slice<char> Buffer, bool* Success)
  -> slice<char>
{
  return ImplConvertIntegerToStringHelper<int16>(Integer, Buffer, Success);
}

auto
::ImplConvertIntegerToString(int32 Integer, slice<char> Buffer, bool* Success)
  -> slice<char>
{
  return ImplConvertIntegerToStringHelper<int32>(Integer, Buffer, Success);
}

auto
::ImplConvertIntegerToString(int64 Integer, slice<char> Buffer, bool* Success)
  -> slice<char>
{
  return ImplConvertIntegerToStringHelper<int64>(Integer, Buffer, Success);
}

auto
::ImplConvertIntegerToString(uint8 Integer, slice<char> Buffer, bool* Success)
  -> slice<char>
{
  return ImplConvertIntegerToStringHelper<uint8>(Integer, Buffer, Success);
}

auto
::ImplConvertIntegerToString(uint16 Integer, slice<char> Buffer, bool* Success)
  -> slice<char>
{
  return ImplConvertIntegerToStringHelper<uint16>(Integer, Buffer, Success);
}

auto
::ImplConvertIntegerToString(uint32 Integer, slice<char> Buffer, bool* Success)
  -> slice<char>
{
  return ImplConvertIntegerToStringHelper<uint32>(Integer, Buffer, Success);
}

auto
::ImplConvertIntegerToString(uint64 Integer, slice<char> Buffer, bool* Success)
  -> slice<char>
{
  return ImplConvertIntegerToStringHelper<uint64>(Integer, Buffer, Success);
}

