
// This file was generated using the tool Utilities/GenerateSingleHeader.py
// from the Backbone project.

#pragma once


// ===================================
// === Source: Backbone/Common.hpp ===
// ===================================

#if !defined(BB_Platform_Windows)
  #error The Backbone is only working on windows platforms for now.
#endif

#if !defined(BB_Inline)
  #define BB_Inline inline
#endif

#if !defined(BB_ForceInline)
  #define BB_ForceInline inline
#endif

#define NoOp do{  }while(0)

#define Crash() *(int*)nullptr = 0

#if defined(BB_Enable_Assert)
  #define Assert(Expression) do{ if(!(Expression)) { Crash(); } } while(0)
#else
  #define Assert(Expression) NoOp
#endif

#if defined(BB_Enable_BoundsCheck)
  #define BoundsCheck(Expression) Assert(Expression)
#else
  #define BoundsCheck(Expression) NoOp
#endif

//
// ================
//

using int8  = char;
using int16 = short;
using int32 = int;
using int64 = long long;

using uint8  = unsigned char;
using uint16 = unsigned short;
using uint32 = unsigned int;
using uint64 = unsigned long long;

using bool32 = int32;

/// Generic unsigned integer if you don't care much about the actual size.
using uint = unsigned int;

//
// ================
//

constexpr uint64 KiB(uint64 Amount) { return Amount      * (uint64)1024; }
constexpr uint64 MiB(uint64 Amount) { return KiB(Amount) * (uint64)1024; }
constexpr uint64 GiB(uint64 Amount) { return MiB(Amount) * (uint64)1024; }
constexpr uint64 TiB(uint64 Amount) { return GiB(Amount) * (uint64)1024; }

constexpr uint64 KB(uint64 Amount) { return Amount     * (uint64)1000; }
constexpr uint64 MB(uint64 Amount) { return KB(Amount) * (uint64)1000; }
constexpr uint64 GB(uint64 Amount) { return MB(Amount) * (uint64)1000; }
constexpr uint64 TB(uint64 Amount) { return GB(Amount) * (uint64)1000; }

//
// ================
//

constexpr double Pi64 = 3.14159265359;
constexpr double E64 = 2.71828182845;

constexpr float Pi32 = (float)Pi64;
constexpr float E32  = (float)E64;

//
// ================
//

template<typename t_type, size_t N>
constexpr size_t
ArrayCount(t_type(&)[N]) { return N; }

// Used to get the size of a type with support for void where a size of 1 byte is assumed.
template<typename T> struct non_void { enum { Size = sizeof(T) }; };
template<>           struct non_void<void>       : non_void<uint8>       { };
template<>           struct non_void<void const> : non_void<uint8 const> { };

/// Reinterpretation of the given pointer in case t_pointer_type is `void`.
template<typename t_pointer_type>
inline constexpr t_pointer_type*
NonVoidPtr(t_pointer_type* Ptr)
{
  return Ptr;
}

/// Reinterpretation of the given pointer in case t_pointer_type is `void`.
inline constexpr uint8*
NonVoidPtr(void* Ptr)
{
  return reinterpret_cast<uint8*>(Ptr);
}

/// Reinterpretation of the given pointer in case t_pointer_type is `void`.
inline constexpr uint8 const*
NonVoidPtr(void const* Ptr)
{
  return reinterpret_cast<uint8 const*>(Ptr);
}

/// Advance the given pointer value by the given amount of bytes.
template<typename t_pointer_type, typename OffsetType>
inline constexpr t_pointer_type*
MemAddByteOffset(t_pointer_type* Pointer, OffsetType Offset)
{
  return reinterpret_cast<t_pointer_type*>((uint8*)Pointer + Offset);
}

/// Advance the given pointer value by the given amount times sizeof(t_pointer_type)
template<typename t_pointer_type, typename OffsetType>
inline constexpr t_pointer_type*
MemAddOffset(t_pointer_type* Pointer, OffsetType Offset)
{
  return MemAddByteOffset(Pointer, Offset * non_void<t_pointer_type>::Size);
}

template<typename t_type>
struct impl_rm_ref
{
  using Result = t_type;
};

template<typename t_type>
struct impl_rm_ref<t_type&>
{
  using Result = t_type;
};

template<typename t_type>
using rm_ref = typename impl_rm_ref<t_type>::Result;

template<class t_type> inline
typename rm_ref<t_type>&&
Move(t_type&& Argument)
{
  // forward Argument as movable
  return static_cast<typename rm_ref<t_type>&&>(Argument);
}

// TODO(Manu): Add int_trait (platform specific because of DWORD etc.)

template<typename t_dest, typename t_source>
constexpr t_dest
Cast(t_source Value)
{
  return static_cast<t_dest>(Value);
}

template<typename t_dest, typename t_source>
constexpr t_dest
Reinterpret(t_source Value)
{
  return reinterpret_cast<t_dest>(Value);
}

/// Coerce value of some type to another.
///
/// Basically just a more explicit C-style cast.
template<typename t_dest, typename t_source>
t_dest
Coerce(t_source Value)
{
  t_dest Result = (t_dest)Value;
  return Result;
}

template<typename t_type>
t_type const&
AsConst(t_type& Value)
{
  return const_cast<t_type const&>(Value);
}

template<typename t_type>
t_type const*
AsPtrToConst(t_type* Value)
{
  return const_cast<t_type const*>(Value);
}

/// Asserts on overflows and underflows when converting signed or unsigned
/// integers.
///
/// TODO(Manu): Implement what is documented below (needs int_trait).
/// In case of error in non-asserting builds, this will return the
/// corresponding min / max value, instead of letting the overflow / underflow
/// happen.
template<typename DestIntegerType, typename SrcIntegerType>
inline DestIntegerType
SafeConvertInt(SrcIntegerType Value)
{
  auto Result = (DestIntegerType)Value;
  auto RevertedResult = (SrcIntegerType)Result;
  Assert(RevertedResult == Value); // Otherwise something went wrong in the conversion step (overflow/underflow).
  return Result;
}

/// \return 1 for a positive number, -1 for a negative number, 0 otherwise.
template<typename t_type>
constexpr t_type
Sign(t_type I)
{
  return (I > 0) ? t_type(1) : (I < 0) ? t_type(-1) : t_type(0);
}

template<typename t_type>
constexpr t_type
Abs(t_type I)
{
  return Sign(I) * I;
}

template<typename t_a_type, typename t_b_type>
constexpr t_a_type
Min(t_a_type A, t_b_type B)
{
  return (B < A) ? Coerce<t_a_type>(B) : A;
}

template<typename t_a_type, typename t_b_type>
constexpr t_a_type
Max(t_a_type A, t_b_type B)
{
  return (B > A) ? Coerce<t_a_type>(B) : A;
}

template<typename t_value_type, typename t_lower_bound_type, typename t_upper_bound_type>
constexpr t_value_type
Clamp(t_value_type Value, t_lower_bound_type LowerBound, t_upper_bound_type UpperBound)
{
  return UpperBound < LowerBound ? Value : Min(UpperBound, Max(LowerBound, Value));
}

// TODO: Make this a constexpr
template<typename t_value_type, typename t_lower_bound_type, typename t_upper_bound_type>
t_value_type
Wrap(t_value_type Value, t_lower_bound_type LowerBound, t_upper_bound_type UpperBound)
{
  const auto BoundsDelta = (Coerce<t_lower_bound_type>(UpperBound) - LowerBound);
  while(Value >= UpperBound) Value -= BoundsDelta;
  while(Value < LowerBound)  Value += BoundsDelta;
  return Value;
  // return Value >= UpperBound ? Value - BoundsDelta :
  //        Value <  LowerBound ? Value + BoundsDelta :
  //                              Value;
}

// Project a value from [LowerBound, UpperBound] to [0, 1]
// Example:
//   auto Result = NormalizeValue<float>(15, 10, 30); // == 0.25f
template<typename t_result, typename t_value_type, typename t_lower_bound_type, typename t_upper_bound_type>
constexpr t_result
NormalizeValue(t_value_type Value, t_lower_bound_type LowerBound, t_upper_bound_type UpperBound)
{
  return UpperBound <= LowerBound ?
         t_result(0) : // Bogus bounds.
         Cast<t_result>(Value - LowerBound) / Cast<t_result>(UpperBound - LowerBound);
}

BB_Inline bool
AreNearlyEqual(double A, double B, double Epsilon);

BB_Inline bool
AreNearlyEqual(float A, float B, float Epsilon);

template<typename t_a, typename t_b>
inline void
Swap(t_a& A, t_b& B)
{
  auto Temp = A;
  A = B;
  B = Temp;
}

template<typename t_func_type>
struct _defer_impl
{
  t_func_type Func;

  template<typename t_in_func_type>
  _defer_impl(t_in_func_type InFunc) : Func{ Move(InFunc) } {};
  ~_defer_impl() { Func(); }
};

// Usage:
// int i = 0;
// Defer(&, printf("Foo %d\n", i); i++ );
// Defer(&, printf("Bar %d\n", i); i++ );
//
// Output:
// Bar 0
// Foo 1
//
// \param CaptureSpec The lambda capture specification.
#define Defer(CaptureSpec, ...) auto _DeferFunc##__LINE__ = [CaptureSpec](){ __VA_ARGS__; }; \
_defer_impl<decltype(_DeferFunc##__LINE__)> _Defer##__LINE__{ _DeferFunc##__LINE__ }

// ==================================
// === Source: Backbone/Slice.hpp ===
// ==================================

template<typename t_element>
struct slice
{
  using element_type = t_element;

  size_t Num;
  element_type* Data;

  /// Test whether this slice is valid or not.
  ///
  /// A slice is considered valid if it does not point to null and contains at
  /// least one element.
  operator bool() const { return Num && Data; }

  /// Index operator to access elements of the slice.
  template<typename t_index_type>
  auto
  operator[](t_index_type Index) const
    -> decltype(Data[Index])
  {
    BoundsCheck(Index >= 0 && Index < Num);
    return Data[Index];
  }
};

template<>
struct slice<void>
{
  using element_type = void;

  size_t Num;
  element_type* Data;

  /// Test whether this slice is valid or not.
  ///
  /// A slice is considered valid if it does not point to null and contains at
  /// least one element.
  operator bool() const { return Num && Data; }
};

template<>
struct slice<void const>
{
  using element_type = void const;

  size_t Num;
  element_type* Data;

  /// Test whether this slice is valid or not.
  ///
  /// A slice is considered valid if it does not point to null and contains at
  /// least one element.
  operator bool() const { return Num && Data; }
};

template<typename t_type>
typename slice<t_type>::element_type*
First(slice<t_type> const& SomeSlice)
{
  return SomeSlice.Data;
}

template<typename t_type>
typename slice<t_type>::element_type*
Last(slice<t_type> const& SomeSlice)
{
  return MemAddOffset(First(SomeSlice), Max(1, SomeSlice.Num) - 1);
}

template<typename t_type>
typename slice<t_type>::element_type*
OnePastLast(slice<t_type> const& SomeSlice)
{
  return MemAddOffset(First(SomeSlice), SomeSlice.Num);
}

/// C++11 range API
template<typename t_type>
typename slice<t_type>::element_type*
begin(slice<t_type> const& SomeSlice)
{
  return First(SomeSlice);
}

/// C++11 range API
template<typename t_type>
typename slice<t_type>::element_type*
end(slice<t_type> const& SomeSlice)
{
  return OnePastLast(SomeSlice);
}

template<typename t_target, typename t_source>
slice<t_target>
SliceReinterpret(slice<t_source> SomeSlice)
{
  return CreateSlice(Reinterpret<t_target*>(First(SomeSlice)),
                     Reinterpret<t_target*>(OnePastLast(SomeSlice)));
}

template<typename t_source>
slice<t_source const>
SliceAsConst(slice<t_source> SomeSlice)
{
  return CreateSlice(AsPtrToConst(First(SomeSlice)),
                     AsPtrToConst(OnePastLast(SomeSlice)));
}

/// Concatenate two slices together.
///
/// \return The returned slice will be a subset of the given Buffer, which is
/// \used to write the actual result in.
template<typename t_element>
slice<t_element>
SliceConcat(slice<t_element const> Head, slice<t_element const> Tail, slice<t_element> Buffer)
{
  BoundsCheck(Buffer.Num >= Head.Num + Tail.Num);
  size_t DestIndex = 0;
  for(auto Element : Head)
  {
    Buffer[DestIndex++] = Element;
  }
  for(auto Element : Tail)
  {
    Buffer[DestIndex++] = Element;
  }

  // DestIndex must now be the combined count of Head and Tail.
  Assert(DestIndex == Head.Num + Tail.Num);
  auto Result = Slice(Buffer, 0, DestIndex);
  return Result;
}

/// Create a union of both input spans. The resulting slice will contain everything
template<typename t_element>
constexpr slice<t_element>
SliceUnion(slice<t_element> SliceA, slice<t_element> SliceB)
{
  // A union only makes sense when both slices are overlapping.
  Assert(SlicesAreOverlapping(SliceA, SliceB));
  return { Min(First(SliceA), First(SliceB)), Max(OnePastLast(SliceA), OnePastLast(SliceB)) };
}

template<typename t_element_a, typename t_element_b>
constexpr bool
SlicesAreDisjoint(slice<t_element_a> SliceA, slice<t_element_b> SliceB)
{
  return Last(SliceA) < First(SliceB) || First(SliceA) > Last(SliceB);
}

/// Whether SliceA and SliceB overlap.
/// \see Contains
template<typename t_element_a, typename t_element_b>
bool
SlicesAreOverlapping(slice<t_element_a> SliceA, slice<t_element_b> SliceB)
{
  auto UnionOfAB = SliceUnion(SliceA, SliceB);
  return SliceContains(UnionOfAB, SliceA) || SliceContains(UnionOfAB, SliceA);
}

/// Whether SliceA completely contains SliceB.
/// \see AreOverlapping
template<typename t_element_a, typename t_element_b>
constexpr bool
SliceContains(slice<t_element_a> SliceA, slice<t_element_b> SliceB)
{
  return First(SliceA) <= First(SliceB) && OnePastLast(SliceA) >= OnePastLast(SliceB);
}

template<typename t_element>
constexpr slice<t_element>
CreateSlice(size_t Num, t_element* Data)
{
  return { Num, Data };
}

template<typename t_element>
slice<t_element>
CreateSlice(t_element* Begin, t_element* End)
{
  auto EndInt = Reinterpret<size_t>(End);
  auto BeginInt = Reinterpret<size_t>(Begin);

  slice<t_element> Result;
  Result.Num = EndInt < BeginInt ? 0 : BeginInt - EndInt;
  Result.Data = Begin;
  return Result;
}

template<typename t_element, size_t N>
slice<t_element>
CreateSlice(t_element (&Array)[N])
{
  return { N, &Array[0] };
}

/// Create a char slice from a static char array, excluding '\0'.
template<size_t N>
slice<char const>
CreateSliceFromString(char const(&StringLiteral)[N])
{
  return { N - 1, &StringLiteral[0] };
}

/// \param StringPtr Must be null-terminated.
inline
slice<char const>
CreateSliceFromString(char const* StringPtr)
{
  auto Seek = StringPtr;
  size_t Count = 0;
  while(*Seek++) ++Count;
  return { Count, StringPtr };
}

/// \param StringPtr Must be null-terminated.
inline
slice<char>
CreateSliceFromString(char* StringPtr)
{
  auto Constified = Coerce<char const*>(StringPtr);
  auto Result = CreateSliceFromString(Constified);
  return CreateSlice(Coerce<char*>(First(Result)),
                     Coerce<char*>(First(Result)));
}

/// Creates a new slice from an existing slice.
///
/// \param InclusiveStartIndex The index to start slicing from.
/// \param ExclusiveEndIndex The index of the first excluded element.
template<typename t_element, typename t_start_index, typename t_end_index>
slice<t_element>
Slice(slice<t_element> SomeSlice, t_start_index InclusiveStartIndex, t_end_index ExclusiveEndIndex)
{
  Assert(InclusiveStartIndex <= ExclusiveEndIndex);
  slice<t_element> Result;
  Result.Num = ExclusiveEndIndex - InclusiveStartIndex;
  Result.Data = MemAddOffset(SomeSlice.Data, InclusiveStartIndex);
  BoundsCheck(SliceContains(SomeSlice, Result));
  return Result;
}

/// Copies the contents of one slice into another.
///
/// If the number of elements don't match, the minimum of both is used.
template<typename t_type>
size_t
SliceCopy(slice<t_type> Target, slice<t_type const> Source)
{
  size_t Amount = Min(Target.Num, Source.Num);
  // TODO(Manu): memcpy?
  // size_t Bytes = Amount * sizeof(t_type);
  // ::memcpy(Target.Data, Source.Data, Bytes);
  for(size_t Index = 0; Index < Amount; ++Index)
  {
    Target.Data[Index] = Source.Data[Index];
  }
  return Amount;
}

/// Compares the contents of the two slices for equality.
///
/// Two slices are deemed equal if they have the same number of elements and
/// each individual element in A compares equal to the corresponding element
/// in B in the order they appear in.
template<typename t_element_a, typename t_element_b>
bool
operator ==(slice<t_element_a> A, slice<t_element_b> B)
{
  if(A.Num != B.Num) return false;

  auto A_ = NonVoidPtr(First(A));
  auto B_ = NonVoidPtr(First(B));
  // if(A_ == B_) return true;
  if(Coerce<size_t>(A_) == Coerce<size_t>(B_)) return true;

  auto NumElements = A.Num;
  while(NumElements)
  {
    if(*A_ == *B_)
      --NumElements;
    else
      return false;
  }

  return true;
}

template<typename t_element_a, typename t_element_b>
bool
operator !=(slice<t_element_a> A, slice<t_element_b> B)
{
  return !(A == B);
}

// ==================================
// === Source: Backbone/Angle.hpp ===
// ==================================

struct radians { float Value; };

constexpr inline bool    operator ==(radians A, radians B)   { return A.Value == B.Value;    }
constexpr inline bool    operator !=(radians A, radians B)   { return A.Value != B.Value;    }
constexpr inline bool    operator < (radians A, radians B)   { return A.Value <  B.Value;    }
constexpr inline bool    operator <=(radians A, radians B)   { return A.Value <= B.Value;    }
constexpr inline bool    operator > (radians A, radians B)   { return A.Value >  B.Value;    }
constexpr inline bool    operator >=(radians A, radians B)   { return A.Value >= B.Value;    }

constexpr inline radians operator + (radians A, radians B)   { return { A.Value + B.Value }; }
constexpr inline radians operator - (radians A, radians B)   { return { A.Value - B.Value }; }
constexpr inline radians operator * (radians A, float Scale) { return { A.Value * Scale };   }
constexpr inline radians operator * (float Scale, radians A) { return { Scale * A.Value };   }
constexpr inline radians operator / (radians A, float Scale) { return { A.Value / Scale };   }

inline void operator +=(radians& A, radians B)   { A.Value += B.Value; }
inline void operator -=(radians& A, radians B)   { A.Value -= B.Value; }
inline void operator *=(radians& A, float Scale) { A.Value *= Scale; }
inline void operator /=(radians& A, float Scale) { A.Value /= Scale; }


struct degrees { float Value; };

constexpr inline bool    operator ==(degrees A, degrees B)   { return A.Value == B.Value;    }
constexpr inline bool    operator !=(degrees A, degrees B)   { return A.Value != B.Value;    }
constexpr inline bool    operator < (degrees A, degrees B)   { return A.Value <  B.Value;    }
constexpr inline bool    operator <=(degrees A, degrees B)   { return A.Value <= B.Value;    }
constexpr inline bool    operator > (degrees A, degrees B)   { return A.Value >  B.Value;    }
constexpr inline bool    operator >=(degrees A, degrees B)   { return A.Value >= B.Value;    }

constexpr inline degrees operator + (degrees A, degrees B)   { return { A.Value + B.Value }; }
constexpr inline degrees operator - (degrees A, degrees B)   { return { A.Value - B.Value }; }
constexpr inline degrees operator * (degrees A, float Scale) { return { A.Value * Scale };   }
constexpr inline degrees operator * (float Scale, degrees A) { return { Scale * A.Value };   }
constexpr inline degrees operator / (degrees A, float Scale) { return { A.Value / Scale };   }

inline void operator +=(degrees& A, degrees B)   { A.Value += B.Value; }
inline void operator -=(degrees& A, degrees B)   { A.Value -= B.Value; }
inline void operator *=(degrees& A, float Scale) { A.Value *= Scale; }
inline void operator /=(degrees& A, float Scale) { A.Value /= Scale; }

//
// Angle Conversion
//

constexpr inline degrees
ToDegrees(radians Radians)
{
  return { Radians.Value * (180.0f / Pi32) };
}

constexpr inline radians
ToRadians(degrees Degrees)
{
  return { Degrees.Value * (Pi32 / 180.0f) };
}

//
// Angle Normalization
//

BB_Inline bool
IsNormalized(radians Radians);

BB_Inline radians
Normalized(radians Radians);

BB_Inline radians
AngleBetween(radians A, radians B);

BB_Inline bool
IsNormalized(degrees Degrees);

BB_Inline degrees
Normalized(degrees Degrees);

BB_Inline degrees
AngleBetween(degrees A, degrees B);

//
// Equality
//

/// Checks whether A and B are nearly equal with the given Epsilon.
BB_Inline bool
AreNearlyEqual(radians A, radians B, radians Epsilon = radians{ 0.0001f });

BB_Inline bool
AreNearlyEqual(degrees A, degrees B, degrees Epsilon = degrees{ 0.01f   });

// =================================
// === Source: Backbone/Path.hpp ===
// =================================

enum
{
  Win32_FileSystemPathSeparator = '\\',
  Posix_FileSystemPathSeparator = '/',

  // TODO(Manu): Cross platform.
  Sys_FileSystemPathSeparator = Win32_FileSystemPathSeparator,
};

struct path_options
{
  char Separator = Sys_FileSystemPathSeparator;
  bool32 AppendNull = false;
};

/// Excludes the path separator in Out_Directory.
///
/// \return The index of the separator.
BB_Inline size_t
ExtractPathDirectoryAndFileName(slice<char const> Path, slice<char const>* Out_Directory, slice<char const>* Out_Name, path_options Options = path_options());

BB_Inline size_t
ExtractPathDirectoryAndFileName(slice<char> Path, slice<char>* Out_Directory, slice<char>* Out_Name, path_options Options = path_options());

BB_Inline slice<char>
ConcatPaths(slice<char const> Head, slice<char const> Tail, slice<char> Buffer, path_options Options = path_options());

// =======================================
// === Source: Backbone/FixedBlock.hpp ===
// =======================================

template<size_t N, typename t_element>
struct fixed_block
{
  using element_type = t_element;

  static constexpr size_t Num = N;
  element_type Data[Num];

  template<typename t_index>
  auto
  operator[](t_index Index)
    -> decltype(Data[Index])
  {
    BoundsCheck(Index >= 0 && Index < Num);
    return Data[Index];
  }

  template<typename t_index>
  auto
  operator[](t_index Index) const
    -> decltype(Data[Index])
  {
    BoundsCheck(Index >= 0 && Index < Num);
    return Data[Index];
  }
};

/// C++11 range API
template<size_t N, typename t_element>
t_element*
begin(fixed_block<N, t_element>& Block)
{
  return &Block.Data[0];
}

/// C++11 range API
template<size_t N, typename t_element>
t_element*
end(fixed_block<N, t_element>& Block)
{
  return begin(Block) + Block.N;
}

/// C++11 range API
template<size_t N, typename t_element>
t_element*
begin(fixed_block<N, t_element> const& Block)
{
  return &Block.Data[0];
}

/// C++11 range API
template<size_t N, typename t_element>
t_element*
end(fixed_block<N, t_element> const& Block)
{
  return begin(Block) + Block.N;
}

template<size_t N, typename t_element>
slice<t_element>
Slice(fixed_block<N, t_element>& Block)
{
  return { N, begin(Block) };
}

template<size_t N, typename t_element, typename t_start_index, typename t_end_index>
slice<t_element>
Slice(fixed_block<N, t_element>& Block, t_start_index InclusiveStartIndex, t_end_index ExclusiveEndIndex)
{
  return Slice(Slice(Block), InclusiveStartIndex, ExclusiveEndIndex);
}

// ====================
// === Inline Files ===
// ====================

// ===================================
// === Source: Backbone/Common.inl ===
// ===================================

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
// === Source: Backbone/Angle.inl ===
// ==================================

auto
::IsNormalized(radians Radians)
  -> bool
{
  return Radians.Value >= 0.0f && Radians.Value < 2.0f * Pi32;
}

auto
::Normalized(radians Radians)
  -> radians
{
  return { Wrap(Radians.Value, 0.0f, 2.0f * Pi32) };
}

auto
::AngleBetween(radians A, radians B)
  -> radians
{
  // Taken from ezEngine who got it from here:
  // http://gamedev.stackexchange.com/questions/4467/comparing-angles-and-working-out-the-difference
  return { Pi32 - Abs(Abs(A.Value - B.Value) - Pi32) };
}

auto
::IsNormalized(degrees Degrees)
  -> bool
{
  return Degrees.Value >= 0.0f && Degrees.Value < 360.0f;
}

auto
::Normalized(degrees Degrees)
  -> degrees
{
  return { Wrap(Degrees.Value, 0.0f, 360.0f) };
}

auto
::AngleBetween(degrees A, degrees B)
  -> degrees
{
  // Taken from ezEngine who got it from here:
  // http://gamedev.stackexchange.com/questions/4467/comparing-angles-and-working-out-the-difference
  return { 180.0f - Abs(Abs(A.Value - B.Value) - 180.0f) };
}

auto
::AreNearlyEqual(radians A, radians B, radians Epsilon)
  -> bool
{
  return AreNearlyEqual(A.Value, B.Value, Epsilon.Value);
}

auto
::AreNearlyEqual(degrees A, degrees B, degrees Epsilon)
  -> bool
{
  return AreNearlyEqual(A.Value, B.Value, Epsilon.Value);
}

// =================================
// === Source: Backbone/Path.inl ===
// =================================

auto
::ExtractPathDirectoryAndFileName(slice<char const> Path,
                                  slice<char const>* Out_Directory,
                                  slice<char const>* Out_Name,
                                  path_options Options)
  -> size_t
{
  size_t IndexOfLastSeparator = 0;
  for(size_t Index = 0; Index < Path.Num; ++Index)
  {
    if(Path[Index] == Options.Separator)
      IndexOfLastSeparator = Index;
  }

  if(Out_Directory) *Out_Directory = Slice(Path, 0, IndexOfLastSeparator);

  // Skip the separator itself.
  size_t NameSliceIndex = IndexOfLastSeparator + 1;

  if(Out_Name)
  {
    if(NameSliceIndex < Path.Num) *Out_Name = Slice(Path, NameSliceIndex, Path.Num);
    else                          *Out_Name = Slice(Path, Path.Num, Path.Num); // Produce an empty slice.
  }

  return IndexOfLastSeparator;
}

auto
::ExtractPathDirectoryAndFileName(slice<char> Path,
                                  slice<char>* Out_Directory, slice<char>* Out_Name,
                                  path_options Options)
  -> size_t
{
  return ExtractPathDirectoryAndFileName(SliceAsConst(Path),
                                         Coerce<slice<char const>*>(Out_Directory), Coerce<slice<char const>*>(Out_Name),
                                         Options);
}

auto
::ConcatPaths(slice<char const> Head, slice<char const> Tail, slice<char> Buffer,
              path_options Options)
  -> slice<char>
{
  auto HeadTarget = Slice(Buffer, 0, Head.Num);
  auto TailTarget = Slice(Buffer, Head.Num + 1, Head.Num + 1 + Tail.Num);

  SliceCopy(HeadTarget, Head);
  Buffer[Head.Num] = Options.Separator;
  SliceCopy(TailTarget, Tail);

  auto Result = Slice(Buffer, 0, Head.Num + 1 + Tail.Num);
  if(Options.AppendNull) Buffer[Result.Num] = '\0';
  return Result;
}

