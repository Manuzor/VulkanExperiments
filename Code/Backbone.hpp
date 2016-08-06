
// This file was generated using the tool Utilities/GenerateSingleHeader.py
// from the Backbone project.

#pragma once

// For placement-new.
#include <new>

// For std::numeric_limits
#include <limits>


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
  #define BoundsCheck(...) Assert(__VA_ARGS__)
#else
  #define BoundsCheck(...) NoOp
#endif

#ifdef DEBUG
  #define BB_Debugging 1
#endif

#if BB_Debugging
  #define DebugCode(...) __VA_ARGS__
#else
  #define DebugCode(...) /* Empty */
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


/// Defines some array variants of types for better readability when used as
/// function parameters.
///
/// For example, a function `Foo` that accepts and array of 4 floats by
/// reference-to-const looks like this:
/// \code
/// void Foo(float const (&ParamName)[4]);
/// \endcode
///
/// Using these typedefs, this can be transformed into:
/// \code
/// void Foo(float_4 const& ParamName);
/// \endcode
#define DefineArrayTypes(TheType)\
  using TheType##_2   = TheType[2];\
  using TheType##_3   = TheType[3];\
  using TheType##_4   = TheType[4];\
  using TheType##_2x2 = TheType##_2[2];\
  using TheType##_2x3 = TheType##_2[3];\
  using TheType##_2x4 = TheType##_2[4];\
  using TheType##_3x2 = TheType##_3[2];\
  using TheType##_3x3 = TheType##_3[3];\
  using TheType##_3x4 = TheType##_3[4];\
  using TheType##_4x2 = TheType##_4[2];\
  using TheType##_4x3 = TheType##_4[3];\
  using TheType##_4x4 = TheType##_4[4];

DefineArrayTypes(float);
DefineArrayTypes(double);

DefineArrayTypes(int8);
DefineArrayTypes(int16);
DefineArrayTypes(int32);
DefineArrayTypes(int64);

DefineArrayTypes(uint8);
DefineArrayTypes(uint16);
DefineArrayTypes(uint32);
DefineArrayTypes(uint64);

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

constexpr uint32 SetBit(uint32 Bits, uint32 Position) { return Bits | (uint32(1) << Position); }
constexpr uint32 UnsetBit(uint32 Bits, uint32 Position) { return Bits & ~(uint32(1) << Position); }
constexpr bool IsBitSet(uint32 Bits, uint32 Position) { return !!(Bits & (uint32(1) << Position)); }

constexpr uint64 SetBit(uint64 Bits, uint64 Position) { return Bits | (uint64(1) << Position); }
constexpr uint64 UnsetBit(uint64 Bits, uint64 Position) { return Bits & ~(uint64(1) << Position); }
constexpr bool IsBitSet(uint64 Bits, uint64 Position) { return !!(Bits & (uint64(1) << Position)); }

//
// ================
//

template<typename T = float>
constexpr T
Pi() { return (T)3.14159265359; }

template<typename T = float>
constexpr T
E() { return (T)2.71828182845; }

//
// ================
//

template<typename t_type, size_t N>
constexpr size_t
ArrayCount(t_type(&)[N]) { return N; }

template<typename T> struct impl_size_of { enum { Value = sizeof(T) }; };
template<>           struct impl_size_of<void>          : impl_size_of<uint8>          { };
template<>           struct impl_size_of<void const>    : impl_size_of<uint8 const>    { };
template<>           struct impl_size_of<void volatile> : impl_size_of<uint8 volatile> { };

/// Get the size of type T in bytes.
///
/// Same as sizeof(T) except it works also with 'void' (possibly cv-qualified) where a size of 1 byte is assumed.
template<typename T>
constexpr size_t
SizeOf() { return impl_size_of<T>::Value; }

/// Reinterpretation of the given pointer in case t_pointer_type is `void`.
template<typename t_pointer_type>
constexpr t_pointer_type*
NonVoidPtr(t_pointer_type* Ptr)
{
  return Ptr;
}

/// Reinterpretation of the given pointer in case t_pointer_type is `void`.
constexpr uint8*
NonVoidPtr(void* Ptr)
{
  return reinterpret_cast<uint8*>(Ptr);
}

/// Reinterpretation of the given pointer in case t_pointer_type is `void`.
constexpr uint8 const*
NonVoidPtr(void const* Ptr)
{
  return reinterpret_cast<uint8 const*>(Ptr);
}

/// Reinterpretation of the given pointer in case t_pointer_type is `void`.
constexpr uint8 volatile*
NonVoidPtr(void volatile* Ptr)
{
  return reinterpret_cast<uint8 volatile*>(Ptr);
}

/// Advance the given pointer value by the given amount of bytes.
template<typename t_pointer_type, typename OffsetType>
constexpr t_pointer_type*
MemAddByteOffset(t_pointer_type* Pointer, OffsetType Offset)
{
  return reinterpret_cast<t_pointer_type*>((uint8*)Pointer + Offset);
}

/// Advance the given pointer value by the given amount times sizeof(t_pointer_type)
template<typename t_pointer_type, typename OffsetType>
constexpr t_pointer_type*
MemAddOffset(t_pointer_type* Pointer, OffsetType Offset)
{
  return MemAddByteOffset(Pointer, Offset * SizeOf<t_pointer_type>());
}

// TODO: This is MSVC specific right now.
template<typename T> struct impl_is_pod { static constexpr bool Value = __is_pod(T); };
template<>           struct impl_is_pod<void>          : public impl_is_pod<uint8>          {};
template<>           struct impl_is_pod<void const>    : public impl_is_pod<uint8 const>    {};
template<>           struct impl_is_pod<void volatile> : public impl_is_pod<uint8 volatile> {};

/// Whether the given type T is a "plain old data" (POD) type.
///
/// The type 'void' is also considered POD.
template<typename T>
constexpr bool
IsPOD() { return impl_is_pod<T>::Value; }

/// Get the number of bits of a given type.
///
/// Note: The type 'void' is not supported.
template<typename T>
constexpr size_t
NumBits() { return sizeof(T) * 8; }

template<typename T>
constexpr bool
IntIsSigned() { return ((T)-1) < 0; }

template<typename T>
constexpr T
IntMaxValue()
{
  return IntIsSigned<T>() ? (T(1) << (NumBits<T>() - 1)) - T(1)
                          : T(-1);
}

template<typename T>
constexpr T
IntMinValue()
{
  return IntIsSigned<T>() ? -(T(1) << (NumBits<T>() - 1))
                          : T(0);
}

template<typename CharType> struct impl_is_digit_helper { static constexpr bool Do(CharType Char) { return Char >= '0' && Char <= '9'; } };
template<typename CharType> struct impl_is_digit;
template<> struct impl_is_digit<char> : public impl_is_digit_helper<char> {};

template<typename CharType>
constexpr bool
IsDigit(CharType Char)
{
  return impl_is_digit<rm_ref_const<CharType>>::Do(Char);
}

template<typename CharType>
struct impl_is_whitespace_helper
{
  static constexpr bool
  Do(CharType Char)
  {
    return Char == ' '  ||
           Char == '\n' ||
           Char == '\r' ||
           Char == '\t' ||
           Char == '\b';
  }
};

template<typename CharType> struct impl_is_whitespace;
template<> struct impl_is_whitespace<char> : public impl_is_whitespace_helper<char> {};

template<typename CharType>
constexpr bool
IsWhitespace(CharType Char)
{
  return impl_is_whitespace<rm_ref_const<CharType>>::Do(Char);
}

template<typename T> struct impl_nan;
// TODO: Cross-platform.
template<> struct impl_nan<float>  { static constexpr float  Value = std::numeric_limits<float>::quiet_NaN(); };
template<> struct impl_nan<double> { static constexpr double Value = std::numeric_limits<double>::quiet_NaN(); };

/// Returns a quiet Not-A-Number value of the given type.
template<typename T>
constexpr T
NaN()
{
  return impl_nan<T>::Value;
}

template<typename T> struct impl_is_nan;
template<> struct impl_is_nan<float>  { static constexpr bool Do(float  Value) { return Value != Value; } };
template<> struct impl_is_nan<double> { static constexpr bool Do(double Value) { return Value != Value; } };

template<typename T>
constexpr bool
IsNaN(T Value)
{
  return impl_is_nan<T>::Do(Value);
}

template<typename T> struct impl_rm_ref     { using Type = T; };
template<typename T> struct impl_rm_ref<T&> { using Type = T; };

template<typename T>
using rm_ref = typename impl_rm_ref<T>::Type;

template<typename T> struct impl_rm_const          { using Type = T; };
template<typename T> struct impl_rm_const<T const> { using Type = T; };

template<typename T>
using rm_const = typename impl_rm_const<T>::Type;

template<typename T>
using rm_ref_const = rm_const<rm_ref<T>>;

template<class t_type>
constexpr typename rm_ref<t_type>&&
Move(t_type&& Argument)
{
  // forward Argument as movable
  return static_cast<typename rm_ref<t_type>&&>(Argument);
}

template<typename t_type>
constexpr t_type&&
Forward(typename rm_ref<t_type>& Argument)
{
  return static_cast<t_type&&>(Argument);
}

template<typename t_type>
constexpr t_type&&
Forward(rm_ref<t_type>&& Argument)
{
  return static_cast<t_type&&>(Argument);
}

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

template<typename ToType, typename FromType>
struct impl_convert
{
  static constexpr ToType
  Do(FromType const& Value)
  {
    return Cast<ToType>(Value);
  }
};

template<typename ToType, typename FromType, typename... ExtraTypes>
ToType
Convert(FromType const& From, ExtraTypes&&... Extra)
{
  using UnqualifiedToType   = rm_ref_const<ToType>;
  using UnqualifiedFromType = rm_ref_const<FromType>;
  using Impl = impl_convert<UnqualifiedToType, UnqualifiedFromType>;
  return Impl::Do(From, Forward<ExtraTypes>(Extra)...);
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

double
Pow(double Base, double Exponent);

float
Pow(float Base, float Exponent);

template<typename ReturnType = double, typename BaseType, typename ExponentType>
constexpr ReturnType
Pow(BaseType Base, ExponentType Exponent) { return (ReturnType)Pow((double)Base, (double)Exponent); }

double
Mod(double Value, double Divisor);

float
Mod(float Value, float Divisor);

double
Sqrt(double Value);

float
Sqrt(float Value);

template<typename ReturnType = double, typename T>
constexpr ReturnType
Sqrt(T Value) { return (ReturnType)Sqrt((double)Value); }

float
InvSqrt(float Value);


template<typename NumberType> struct impl_negate;
template<> struct impl_negate<float>  { static constexpr float  Do(float  Value) { return -Value; } };
template<> struct impl_negate<double> { static constexpr double Do(double Value) { return -Value; } };
template<> struct impl_negate<int8>   { static constexpr int8   Do(int8  Value)  { return -Value; } };
template<> struct impl_negate<int16>  { static constexpr int16  Do(int16 Value)  { return -Value; } };
template<> struct impl_negate<int32>  { static constexpr int32  Do(int32 Value)  { return -Value; } };
template<> struct impl_negate<int64>  { static constexpr int64  Do(int64 Value)  { return -Value; } };
template<> struct impl_negate<uint8>  { static constexpr uint8  Do(uint8  Value) { return  Value; } };
template<> struct impl_negate<uint16> { static constexpr uint16 Do(uint16 Value) { return  Value; } };
template<> struct impl_negate<uint32> { static constexpr uint32 Do(uint32 Value) { return  Value; } };
template<> struct impl_negate<uint64> { static constexpr uint64 Do(uint64 Value) { return  Value; } };

template<typename NumberType>
NumberType
Negate(NumberType Value)
{
  return impl_negate<NumberType>::Do(Value);
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

bool
AreNearlyEqual(double A, double B, double Epsilon = 1e-4);

bool
AreNearlyEqual(float A, float B, float Epsilon = 1e-4f);

inline bool
IsNearlyZero(double A, double Epsilon = 1e-4) { return AreNearlyEqual(A, 0, Epsilon); }

inline bool
IsNearlyZero(float A, float Epsilon = 1e-4f) { return AreNearlyEqual(A, 0, Epsilon); }

template<typename t_a, typename t_b>
inline void
Swap(t_a& A, t_b& B)
{
  auto Temp = A;
  A = B;
  B = Temp;
}

/// Maps the given float Value from [0, 1] to [0, MaxValueOf(UNormType)]
template<typename UNormType>
UNormType constexpr
FloatToUNorm(float Value)
{
  return Cast<UNormType>(Clamp((Value * IntMaxValue<UNormType>()) + 0.5f, 0.0f, IntMaxValue<UNormType>()));
}

/// Maps the given unsigned byte Value from [0, 255] to [0, 1]
template<typename UNormType>
float constexpr
UNormToFloat(UNormType Value)
{
  return Clamp(Cast<float>(Value) / IntMaxValue<UNormType>(), 0.0f, 1.0f);
}

/// \see InitStruct
template<typename T>
struct impl_init_struct
{
  // Return an initialized instance of T.
  template<typename... ArgTypes>
  static constexpr T
  Create(ArgTypes&&... Args) { return { Forward<ArgTypes>(Args)... }; }
};

/// Utility function to initialize a struct of the given type with a chance
/// for centralized specialization.
///
/// To control the default or non-default construction behavior of a certain
/// struct the template \c impl_init_struct can be specialized and a Create()
/// function must be provided.
///
/// \see impl_init_struct
template<typename T, typename... ArgTypes>
inline auto
InitStruct(ArgTypes&&... Args)
  -> decltype(impl_init_struct<rm_ref<T>>::Create(Forward<ArgTypes>(Args)...))
{
  // Note: specializations for impl_init_struct are found in VulkanHelper.inl
  return impl_init_struct<rm_ref<T>>::Create(Forward<ArgTypes>(Args)...);
}

struct impl_defer
{
  template<typename LambdaType>
  struct defer
  {
    LambdaType Lambda;
    defer(LambdaType InLambda) : Lambda{ Move(InLambda) } {}
    ~defer() { Lambda(); }
  };

  template<typename t_in_func_type>
  defer<t_in_func_type> operator =(t_in_func_type InLambda) { return { Move(InLambda) }; }
};

#define PRE_Concat2_Impl(A, B)  A ## B
#define PRE_Concat2(A, B)       PRE_Concat2_Impl(A, B)
#define PRE_Concat3(A, B, C)    PRE_Concat2(PRE_Concat2(A, B), C)
#define PRE_Concat4(A, B, C, D) PRE_Concat2(PRE_Concat2(A, B), PRE_Concat2(C, D))

/// Defers execution of code until the end of the current scope.
///
/// Usage:
///   int i = 0;
///   Defer [&](){ i++; printf("Foo %d\n", i); };
///   Defer [&](){ i++; printf("Bar %d\n", i); };
///   Defer [=](){      printf("Baz %d\n", i); };
///
/// Output:
///   Baz 0
///   Bar 1
///   Foo 2
///
/// \param CaptureSpec The lambda capture specification.
//#define Defer(Lambda) impl_defer<decltype(Lambda)> PRE_Concat2(_Defer, __LINE__){ Lambda }
#define Defer auto PRE_Concat2(_Defer, __LINE__) = impl_defer() =


/// Helper macro to define an opaque handle type.
///
/// Usage:
///   DefineOpaqueHandle(Foo);
///   /*...*/
///   Foo CreateFoo(); // Returns a Foo.
#define DefineOpaqueHandle(Name) using Name = struct Name ## _OPAQUE*

// ===================================
// === Source: Backbone/Memory.hpp ===
// ===================================

//
// Untyped / Raw Memory Functions
//

void
MemCopyBytes(size_t NumBytes, void* Destination, void const* Source);

void
MemSetBytes(size_t NumBytes, void* Destination, int Value);

bool
MemEqualBytes(size_t NumBytes, void const* A, void const* B);

int
MemCompareBytes(size_t NumBytes, void const* A, void const* B);

//
// Typed Copy
//

template<typename T, bool IsPlainOldData = false>
struct impl_mem_copy
{
  static void Do(size_t Num, T* Destination, T const* Source)
  {
    for(size_t Index = 0; Index < Num; ++Index)
    {
      Destination[Index] = Source[Index];
    }
  }
};

template<typename T>
struct impl_mem_copy<T, true>
{
  static constexpr void Do(size_t Num, T* Destination, T const* Source)
  {
    MemCopyBytes(Num * SizeOf<T>(), Reinterpret<void*>(Destination), Reinterpret<void const*>(Source));
  }
};

template<typename T>
constexpr void
MemCopy(size_t Num, T* Destination, T const* Source)
{
  impl_mem_copy<T, IsPOD<T>()>::Do(Num, Destination, Source);
}

//
// Typed Move
//

template<typename T, bool IsPlainOldData = false>
struct impl_mem_move
{
  static void Do(size_t Num, T* Destination, T const* Source)
  {
    for(size_t Index = 0; Index < Num; ++Index)
    {
      Destination[Index] = Source[Index];
    }
  }
};

template<typename T>
struct impl_mem_move<T, false>
{
  static constexpr void Do(size_t Num, T* Destination, T const* Source)
  {
    MemCopyBytes(Num * sizeof(T), Reinterpret<void*>(Destination), Reinterpret<void const*>(Source));
  }
};

template<typename T>
constexpr void
MemMove(size_t Num, T* Destination, T const* Source)
{
  impl_mem_move<T, IsPOD<T>()>::Do(Num, Destination, Source);
}

//
// Typed Set
//

template<typename T, bool IsPlainOldData = false>
struct impl_mem_set
{
  static void Do(size_t Num, T* Destination, T Value)
  {
    for(size_t Index = 0; Index < Num; ++Index)
    {
      Destination[Index] = Value;
    }
  }
};

template<typename T>
struct impl_mem_set<T, false>
{
  static constexpr void Do(size_t Num, T* Destination, T Value)
  {
    for(size_t Index = 0; Index < Num; ++Index)
    {
      MemCopyBytes(Destination + Index, &Value, sizeof(T));
    }
  }
};

template<typename T>
void
MemSet(size_t Num, T* Destination, T Value)
{
  impl_mem_set<T, IsPOD<T>()>::Do(Num, Destination, Move(Value));
}


//
// Typed Default Construction
//

template<typename T, bool IsPOD = false>
struct impl_mem_default_construct
{
  static void Do(size_t Num, T* Elements)
  {
    for(size_t Index = 0; Index < Num; ++Index)
    {
      new (Elements + Index) T;
    }
  }
};

template<typename T>
struct impl_mem_default_construct<T, true>
{
  static void Do(size_t Num, T* Elements)
  {
    MemSetBytes(Num * sizeof(T), Elements, 0);
  }
};

template<typename T>
constexpr void
MemDefaultConstruct(size_t Num, T* Elements)
{
  impl_mem_default_construct<T, IsPOD<T>()>::Do(Num, Elements);
}

//
// Typed Construction With Arguments
//

template<typename T, bool IsPOD = false>
struct impl_mem_construct
{
  template<typename... ArgTypes>
  static void Do(T* Ptr, size_t Num, ArgTypes&&... Args)
  {
    for(size_t Index = 0; Index < Num; ++Index)
    {
      new (Ptr + Index) T(Forward<ArgTypes>(Args)...);
    }
  }
};

template<typename T>
struct impl_mem_construct<T, true>
{
  static void Do(size_t Num, T* Ptr, T Value)
  {
    MemSet(Num, Ptr, Move(Value));
  }
};

template<typename T, typename... ArgTypes>
constexpr void
MemConstruct(size_t Num, T* Elements, ArgTypes&&... Args)
{
  impl_mem_construct<T, IsPOD<T>()>::Do(Num, Elements, Forward<ArgTypes>(Args)...);
}

//
// Typed Destruction
//

template<typename T, bool IsPOD = false>
struct impl_mem_destruct
{
  static void Do(size_t Num, T* Elements)
  {
    for(size_t Index = 0; Index < Num; ++Index)
    {
      Elements[Index].~T();
    }
  }
};

template<typename T>
struct impl_mem_destruct<T, true>
{
  static void Do(size_t Num, T* Elements)
  {
    // Nothing to do for PODs
  }
};

template<typename T>
constexpr void
MemDestruct(size_t Num, T* Elements)
{
  impl_mem_destruct<T, IsPOD<T>()>::Do(Num, Elements);
}

// ==================================
// === Source: Backbone/Slice.hpp ===
// ==================================

constexpr size_t INVALID_INDEX = (size_t)-1;

template<typename ElementType>
struct slice
{
  using element_type = ElementType;

  size_t Num;
  element_type* Ptr;

  /// Test whether this slice is valid or not.
  ///
  /// A slice is considered valid if it does not point to null and contains at
  /// least one element. If `Num` is 0 or `Ptr` is `nullptr`, the slice is
  /// considered invalid (`false`).
  operator bool() const { return Num && Ptr; }

  /// Implicit conversion to const version.
  operator slice<ElementType const>() const { return { Num, Ptr }; }

  /// Index operator to access elements of the slice.
  template<typename IndexType>
  auto
  operator[](IndexType Index) const
    -> decltype(Ptr[Index])
  {
    BoundsCheck(Index >= 0 && Index < Num);
    return Ptr[Index];
  }
};

template<>
struct slice<void const>
{
  using element_type = void const;

  size_t Num;
  element_type* Ptr;

  /// Test whether this slice is valid or not.
  ///
  /// A slice is considered valid if it does not point to null and contains at
  /// least one element.
  operator bool() const { return Num && Ptr; }
};

template<>
struct slice<void>
{
  using element_type = void;

  size_t Num;
  element_type* Ptr;

  /// Test whether this slice is valid or not.
  ///
  /// A slice is considered valid if it does not point to null and contains at
  /// least one element.
  operator bool() const { return Num && Ptr; }

  /// Implicit conversion to const version.
  operator slice<void const>() const { return { Num, Ptr }; }
};

template<typename T>
typename slice<T>::element_type*
First(slice<T> const& SomeSlice)
{
  return SomeSlice.Ptr;
}

template<typename T>
typename slice<T>::element_type*
Last(slice<T> const& SomeSlice)
{
  return MemAddOffset(First(SomeSlice), Max(size_t(1), SomeSlice.Num) - 1);
}

template<typename T>
typename slice<T>::element_type*
OnePastLast(slice<T> const& SomeSlice)
{
  return MemAddOffset(First(SomeSlice), SomeSlice.Num);
}

/// C++11 range API
template<typename T>
typename slice<T>::element_type*
begin(slice<T> const& SomeSlice)
{
  return First(SomeSlice);
}

/// C++11 range API
template<typename T>
typename slice<T>::element_type*
end(slice<T> const& SomeSlice)
{
  return OnePastLast(SomeSlice);
}

template<typename TargetType, typename SourceType>
slice<TargetType>
SliceReinterpret(slice<SourceType> SomeSlice)
{
  return Slice(Reinterpret<TargetType*>(First(SomeSlice)),
               Reinterpret<TargetType*>(OnePastLast(SomeSlice)));
}

template<typename SourceType>
slice<SourceType const>
AsConst(slice<SourceType> SomeSlice)
{
  return Slice(AsPtrToConst(First(SomeSlice)),
               AsPtrToConst(OnePastLast(SomeSlice)));
}

/// Concatenate two slices together.
///
/// \return The returned slice will be a subset of the given Buffer, which is
/// \used to write the actual result in.
template<typename ElementType>
slice<ElementType>
SliceConcat(slice<ElementType const> Head, slice<ElementType const> Tail, slice<ElementType> Buffer)
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
template<typename ElementType>
constexpr slice<ElementType>
SliceUnion(slice<ElementType> SliceA, slice<ElementType> SliceB)
{
  // A union only makes sense when both slices are overlapping.
  Assert(SlicesAreOverlapping(SliceA, SliceB));
  return { Min(First(SliceA), First(SliceB)), Max(OnePastLast(SliceA), OnePastLast(SliceB)) };
}

template<typename ElementTypeA, typename ElementTypeB>
constexpr bool
SlicesAreDisjoint(slice<ElementTypeA> SliceA, slice<ElementTypeB> SliceB)
{
  return Last(SliceA) < First(SliceB) || First(SliceA) > Last(SliceB);
}

/// Whether SliceA and SliceB overlap.
/// \see Contains
template<typename ElementTypeA, typename ElementTypeB>
bool
SlicesAreOverlapping(slice<ElementTypeA> SliceA, slice<ElementTypeB> SliceB)
{
  auto UnionOfAB = SliceUnion(SliceA, SliceB);
  return SliceContains(UnionOfAB, SliceA) || SliceContains(UnionOfAB, SliceA);
}

/// Whether SliceA completely contains SliceB.
/// \see AreOverlapping
template<typename ElementTypeA, typename ElementTypeB>
constexpr bool
SliceContains(slice<ElementTypeA> SliceA, slice<ElementTypeB> SliceB)
{
  return First(SliceA) <= First(SliceB) && OnePastLast(SliceA) >= OnePastLast(SliceB);
}

template<typename ElementType>
constexpr slice<ElementType>
Slice(size_t Num, ElementType* Ptr)
{
  return { Num, Ptr };
}

template<typename ElementType>
slice<ElementType>
Slice(ElementType* FirstPtr, ElementType* OnePastLastPtr)
{
  auto OnePastLastPtr_ = NonVoidPtr(OnePastLastPtr);
  auto FirstPtr_       = NonVoidPtr(FirstPtr);

  DebugCode(
  {
    auto A = Reinterpret<size_t>(FirstPtr);
    auto B = Reinterpret<size_t>(OnePastLastPtr);
    auto Delta = Max(A, B) - Min(A, B);
    Assert(Delta % SizeOf<ElementType>() == 0);
  });

  slice<ElementType> Result;
  Result.Num = OnePastLastPtr_ <= FirstPtr_ ? 0 : OnePastLastPtr_ - FirstPtr_;
  Result.Ptr = FirstPtr;
  return Result;
}

template<typename ElementType, size_t N>
constexpr slice<ElementType>
Slice(ElementType (&Array)[N])
{
  return { N, &Array[0] };
}

/// Create a char slice from a static char array, excluding '\0'.
template<size_t N>
constexpr slice<char const>
SliceFromString(char const(&StringLiteral)[N])
{
  return { N - 1, &StringLiteral[0] };
}

/// \param StringPtr Must be null-terminated.
slice<char const>
SliceFromString(char const* StringPtr);

/// \param StringPtr Must be null-terminated.
slice<char>
SliceFromString(char* StringPtr);

/// Custom string literal suffix.
/// Usage: slice<char const> Foo = "Foo"_S;
inline slice<char const>
operator "" _S(char const* StringPtr, size_t Num) { return Slice(Num, StringPtr); }

/// Creates a new slice from an existing slice.
///
/// \param InclusiveStartIndex The index to start slicing from.
/// \param ExclusiveEndIndex The index of the first excluded element.
template<typename ElementType, typename StartIndexType, typename EndIndexType>
slice<ElementType>
Slice(slice<ElementType> SomeSlice, StartIndexType InclusiveStartIndex, EndIndexType ExclusiveEndIndex)
{
  Assert(InclusiveStartIndex <= ExclusiveEndIndex);
  slice<ElementType> Result;
  Result.Num = ExclusiveEndIndex - InclusiveStartIndex;
  Result.Ptr = MemAddOffset(SomeSlice.Ptr, InclusiveStartIndex);
  BoundsCheck(SliceContains(SomeSlice, Result));
  return Result;
}

/// Creates a new slice from an existing one, trimming elements at the beginning.
template<typename ElementType, typename AmountType>
constexpr slice<ElementType>
SliceTrimFront(slice<ElementType> SomeSlice, AmountType Amount)
{
  return {
    Amount > SomeSlice.Num ? 0 : SomeSlice.Num - Amount,
    MemAddOffset(SomeSlice.Ptr, Amount)
  };
}

/// Creates a new slice from an existing one, trimming elements at the beginning.
template<typename ElementType, typename AmountType>
constexpr slice<ElementType>
SliceTrimBack(slice<ElementType> SomeSlice, AmountType Amount)
{
  return {
    Amount > SomeSlice.Num ? 0 : SomeSlice.Num - Amount,
    SomeSlice.Ptr
  };
}

/// Copies the contents of one slice into another.
///
/// If the number of elements don't match, the minimum of both is used.
template<typename T>
size_t
SliceCopy(slice<T> Target, slice<T const> Source)
{
  size_t Amount = Min(Target.Num, Source.Num);
  MemCopy(Amount, Target.Ptr, Source.Ptr);
  return Amount;
}

/// Set each element of Target to Value.
template<typename T>
void
SliceSet(slice<T> Target, T const Value)
{
  MemSet(Target.Num, Target.Ptr, Forward<T const>(Value));
}

template<typename T>
size_t
SliceMove(slice<T> Target, slice<T const> Source)
{
  // TODO Optimize for PODs

  size_t Amount = Min(Target.Num, Source.Num);
  for(size_t Index = 0; Index < Amount; ++Index)
  {
    Target.Ptr[Index] = Move(Source.Ptr[Index]);
  }
  return Amount;
}

template<typename T>
void
SliceDefaultConstruct(slice<T> Target)
{
  MemDefaultConstruct(Target.Num, Target.Ptr);
}

template<typename T>
size_t
SliceCopyConstruct(slice<T> Target, slice<T const> Source)
{
  size_t Amount = Min(Target.Num, Source.Num);
  MemCopy(Amount, Target.Ptr, SourcePtr);
  return Amount;
}

template<typename T>
size_t
SliceMoveConstruct(slice<T> Target, slice<T const> Source)
{
  size_t Amount = Min(Target.Num, Source.Num);
  MemMove(Amount, Target.Ptr, SourcePtr);
  return Amount;
}

template<typename T>
void
SliceDestruct(slice<T> Target)
{
  MemDestruct(Target.Num, Target.Ptr);
}

template<typename T, typename NeedleType>
size_t
SliceCountUntil(slice<T const> Haystack, NeedleType const& Needle)
{
  size_t Index = 0;

  for(auto& Straw : Haystack)
  {
    if(Straw == Needle)
      return Index;
    ++Index;
  }

  return INVALID_INDEX;
}

/// Counts up until \c Predicate(ElementOfHaystack, Needle) returns \c true.
template<typename T, typename NeedleType, typename PredicateType>
size_t
SliceCountUntil(slice<T const> Haystack, NeedleType const& Needle, PredicateType Predicate)
{
  size_t Index = 0;

  for(auto& Straw : Haystack)
  {
    if(Predicate(Straw, Needle))
      return Index;
    ++Index;
  }

  return INVALID_INDEX;
}

template<typename T, typename U>
bool
SliceStartsWith(slice<T const> Slice, slice<U const> Sequence)
{
  size_t const Amount = Min(Slice.Num, Sequence.Num);

  for(size_t Index = 0; Index < Amount; ++Index)
  {
    if(Slice[Index] != Sequence[Index])
      return false;
  }

  return true;
}

template<typename T, typename NeedleType>
slice<T>
SliceFind(slice<T> Haystack, NeedleType const& Needle)
{
  while(Haystack.Num)
  {
    if(Haystack[0] == Needle)
      return Haystack;
    Haystack = SliceTrimFront(Haystack, 1);
  }

  return Haystack;
}

template<typename T, typename NeedleType, typename PredicateType>
slice<T>
SliceFind(slice<T> Haystack, NeedleType const& Needle, PredicateType Predicate)
{
  while(Haystack.Num)
  {
    if(Predicate(Haystack[0], Needle))
      return Haystack;
    Haystack = SliceTrimFront(Haystack, 1);
  }

  return Haystack;
}

template<typename T, typename NeedleType>
slice<T>
SliceFind(slice<T> Haystack, slice<NeedleType> const& NeedleSequence)
{
  while(Haystack.Num)
  {
    if(SliceStartsWith(Haystack, NeedleSequence))
      return Haystack;
    Haystack = SliceTrimFront(Haystack, 1);
  }

  return Haystack;
}

template<typename T>
void
SliceReverseElements(slice<T> SomeSlice)
{
  auto const NumSwaps = SomeSlice.Num / 2;
  for(size_t FrontIndex = 0; FrontIndex < NumSwaps; ++FrontIndex)
  {
    auto const BackIndex = SomeSlice.Num - FrontIndex - 1;
    Swap(SomeSlice[FrontIndex], SomeSlice[BackIndex]);
  }
}


/// Compares the contents of the two slices for equality.
///
/// Two slices are deemed equal if they have the same number of elements and
/// each individual element in A compares equal to the corresponding element
/// in B in the order they appear in.
template<typename ElementTypeA, typename ElementTypeB>
bool
operator ==(slice<ElementTypeA> A, slice<ElementTypeB> B)
{
  if(A.Num != B.Num) return false;

  auto A_ = NonVoidPtr(First(A));
  auto B_ = NonVoidPtr(First(B));
  // if(A_ == B_) return true;
  if(Coerce<size_t>(A_) == Coerce<size_t>(B_)) return true;

  auto NumElements = A.Num;
  while(NumElements)
  {
    if(*A_ != *B_)
      return false;

    ++A_;
    ++B_;
    --NumElements;
  }

  return true;
}


template<typename ElementType>
bool
operator ==(slice<ElementType> Slice, nullptr_t)
{
  return !Cast<bool>(Slice);
}
template<typename ElementType>
bool
operator !=(slice<ElementType> Slice, nullptr_t)
{
  return Cast<bool>(Slice);
}

template<typename ElementType>
bool
operator ==(nullptr_t, slice<ElementType> Slice)
{
  return !(Slice == nullptr);
}
template<typename ElementType>
bool
operator !=(nullptr_t, slice<ElementType> Slice)
{
  return Slice != nullptr;
}

template<typename ElementTypeA, typename ElementTypeB>
bool
operator !=(slice<ElementTypeA> A, slice<ElementTypeB> B)
{
  return !(A == B);
}

template<typename ElementType>
constexpr size_t
SliceByteSize(slice<ElementType> S)
{
  return S.Num * SizeOf<ElementType>();
}

// ==================================
// === Source: Backbone/Angle.hpp ===
// ==================================

struct angle { float InternalData; };

constexpr bool operator ==(angle A, angle B) { return A.InternalData == B.InternalData; }
constexpr bool operator !=(angle A, angle B) { return A.InternalData != B.InternalData; }
constexpr bool operator < (angle A, angle B) { return A.InternalData <  B.InternalData; }
constexpr bool operator <=(angle A, angle B) { return A.InternalData <= B.InternalData; }
constexpr bool operator > (angle A, angle B) { return A.InternalData >  B.InternalData; }
constexpr bool operator >=(angle A, angle B) { return A.InternalData >= B.InternalData; }

constexpr angle operator +(angle A, angle B)     { return { A.InternalData + B.InternalData }; }
constexpr angle operator -(angle A, angle B)     { return { A.InternalData - B.InternalData }; }
constexpr angle operator *(angle A, float Scale) { return { A.InternalData * Scale };   }
constexpr angle operator *(float Scale, angle A) { return { Scale * A.InternalData };   }
constexpr angle operator /(angle A, float Scale) { return { A.InternalData / Scale };   }

void operator +=(angle& A, angle B);
void operator -=(angle& A, angle B);
void operator *=(angle& A, float Scale);
void operator /=(angle& A, float Scale);


//
// Angle Conversion
//

constexpr angle
Radians(float RadiansValue)
{
  return { RadiansValue };
}

constexpr float
ToRadians(angle Angle)
{
  return Angle.InternalData;
}

constexpr angle
Degrees(float DegreesValue)
{
  return { DegreesValue * (Pi() / 180.0f) };
}

constexpr float
ToDegrees(angle Angle)
{
  return { Angle.InternalData * (180.0f / Pi()) };
}

//
// Angle Normalization
//

bool
IsNormalized(angle Angle);

angle
Normalized(angle Angle);

angle
AngleBetween(angle A, angle B);

//
// Equality
//

/// Checks whether A and B are nearly equal with the given Epsilon.
bool
AreNearlyEqual(angle A, angle B, angle Epsilon = angle{ 1e-4f });

inline bool
IsNearlyZero(angle A, angle Epsilon = angle{ 1e-4f }) { return AreNearlyEqual(A, angle{ 0 }, Epsilon); }

//
// Trigonometric Functions
//

float
Sin(angle Angle);

float
Cos(angle Angle);

float
Tan(angle Angle);

inline float
Cot(angle Angle) { return 1 / Tan(Angle); }

angle
ASin(float A);

angle
ACos(float A);

angle
ATan(float A);

angle
ATan2(float A, float B);

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
inline size_t
ExtractPathDirectoryAndFileName(slice<char const> Path, slice<char const>* Out_Directory, slice<char const>* Out_Name, path_options Options = {});

inline size_t
ExtractPathDirectoryAndFileName(slice<char> Path, slice<char>* Out_Directory, slice<char>* Out_Name, path_options Options = {});

inline slice<char>
ConcatPaths(slice<char const> Head, slice<char const> Tail, slice<char> Buffer, path_options Options = {});

inline slice<char>
FindFileExtension(slice<char> FileName, path_options Options = {});

inline slice<char const>
FindFileExtension(slice<char const> FileName, path_options Options = {});

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
  return begin(Block) + N;
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
  return begin(Block) + N;
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

// =============================================
// === Source: Backbone/StringConversion.hpp ===
// =============================================

//
// Conversion: String -> Floating Point
//

double
ImplConvertStringToDouble(slice<char const>* Source, bool* Success, double Fallback);

template<typename FloatType>
struct impl_convert_string_to_floating_point_helper
{
  template<typename CharType>
  static inline FloatType
  Do(slice<CharType> String, bool* Success = nullptr, FloatType Fallback = NaN<FloatType>())
  {
    auto Result = ImplConvertStringToDouble(Coerce<slice<char const>*>(&String), Success, Cast<double>(Fallback));
    return Cast<FloatType>(Result);
  }

  template<typename CharType>
  static inline FloatType
  Do(slice<CharType>* String, bool* Success = nullptr, FloatType Fallback = NaN<FloatType>())
  {
    auto Result = ImplConvertStringToDouble(Coerce<slice<char const>*>(String), Success, Cast<double>(Fallback));
    return Cast<FloatType>(Result);
  }
};

template<> struct impl_convert<double, slice<char>>        : public impl_convert_string_to_floating_point_helper<double> {};
template<> struct impl_convert<double, slice<char>*>       : public impl_convert_string_to_floating_point_helper<double> {};
template<> struct impl_convert<double, slice<char const>>  : public impl_convert_string_to_floating_point_helper<double> {};
template<> struct impl_convert<double, slice<char const>*> : public impl_convert_string_to_floating_point_helper<double> {};
template<> struct impl_convert<float,  slice<char>>        : public impl_convert_string_to_floating_point_helper<float>  {};
template<> struct impl_convert<float,  slice<char>*>       : public impl_convert_string_to_floating_point_helper<float>  {};
template<> struct impl_convert<float,  slice<char const>>  : public impl_convert_string_to_floating_point_helper<float>  {};
template<> struct impl_convert<float,  slice<char const>*> : public impl_convert_string_to_floating_point_helper<float>  {};

//
// Conversion: String -> Integer
//

uint64
ImplConvertStringToInteger(slice<char const>* Source, bool* Success, uint64 Fallback);

int64
ImplConvertStringToInteger(slice<char const>* Source, bool* Success, int64 Fallback);

template<typename IntegerType>
struct impl_convert_string_to_integer_helper
{
  template<bool IsSigned> struct impl_max_integer_type;
  template<> struct impl_max_integer_type<true>  { using Type = int64;  };
  template<> struct impl_max_integer_type<false> { using Type = uint64; };

  using MaxIntegerType = typename impl_max_integer_type<IntIsSigned<IntegerType>()>::Type;

  template<typename CharType>
  static inline IntegerType
  Do(slice<CharType> String, bool* Success = nullptr, IntegerType Fallback = 0)
  {
    auto Result = ImplConvertStringToInteger(Coerce<slice<char const>*>(&String), Success, Cast<MaxIntegerType>(Fallback));
    return Convert<IntegerType>(Result);
  }

  template<typename CharType>
  static inline IntegerType
  Do(slice<CharType>* String, bool* Success = nullptr, IntegerType Fallback = 0)
  {
    auto Result = ImplConvertStringToInteger(Coerce<slice<char const>*>(String), Success, Cast<MaxIntegerType>(Fallback));
    return Convert<IntegerType>(Result);
  }
};

template<> struct impl_convert<int8,   slice<char>>        : public impl_convert_string_to_integer_helper<int8>   {};
template<> struct impl_convert<int8,   slice<char>*>       : public impl_convert_string_to_integer_helper<int8>   {};
template<> struct impl_convert<int8,   slice<char const>>  : public impl_convert_string_to_integer_helper<int8>   {};
template<> struct impl_convert<int8,   slice<char const>*> : public impl_convert_string_to_integer_helper<int8>   {};
template<> struct impl_convert<int16,  slice<char>>        : public impl_convert_string_to_integer_helper<int16>  {};
template<> struct impl_convert<int16,  slice<char>*>       : public impl_convert_string_to_integer_helper<int16>  {};
template<> struct impl_convert<int16,  slice<char const>>  : public impl_convert_string_to_integer_helper<int16>  {};
template<> struct impl_convert<int16,  slice<char const>*> : public impl_convert_string_to_integer_helper<int16>  {};
template<> struct impl_convert<int32,  slice<char>>        : public impl_convert_string_to_integer_helper<int32>  {};
template<> struct impl_convert<int32,  slice<char>*>       : public impl_convert_string_to_integer_helper<int32>  {};
template<> struct impl_convert<int32,  slice<char const>>  : public impl_convert_string_to_integer_helper<int32>  {};
template<> struct impl_convert<int32,  slice<char const>*> : public impl_convert_string_to_integer_helper<int32>  {};
template<> struct impl_convert<int64,  slice<char>>        : public impl_convert_string_to_integer_helper<int64>  {};
template<> struct impl_convert<int64,  slice<char>*>       : public impl_convert_string_to_integer_helper<int64>  {};
template<> struct impl_convert<int64,  slice<char const>>  : public impl_convert_string_to_integer_helper<int64>  {};
template<> struct impl_convert<int64,  slice<char const>*> : public impl_convert_string_to_integer_helper<int64>  {};

template<> struct impl_convert<uint8,  slice<char>>        : public impl_convert_string_to_integer_helper<uint8>  {};
template<> struct impl_convert<uint8,  slice<char>*>       : public impl_convert_string_to_integer_helper<uint8>  {};
template<> struct impl_convert<uint8,  slice<char const>>  : public impl_convert_string_to_integer_helper<uint8>  {};
template<> struct impl_convert<uint8,  slice<char const>*> : public impl_convert_string_to_integer_helper<uint8>  {};
template<> struct impl_convert<uint16, slice<char>>        : public impl_convert_string_to_integer_helper<uint16> {};
template<> struct impl_convert<uint16, slice<char>*>       : public impl_convert_string_to_integer_helper<uint16> {};
template<> struct impl_convert<uint16, slice<char const>>  : public impl_convert_string_to_integer_helper<uint16> {};
template<> struct impl_convert<uint16, slice<char const>*> : public impl_convert_string_to_integer_helper<uint16> {};
template<> struct impl_convert<uint32, slice<char>>        : public impl_convert_string_to_integer_helper<uint32> {};
template<> struct impl_convert<uint32, slice<char>*>       : public impl_convert_string_to_integer_helper<uint32> {};
template<> struct impl_convert<uint32, slice<char const>>  : public impl_convert_string_to_integer_helper<uint32> {};
template<> struct impl_convert<uint32, slice<char const>*> : public impl_convert_string_to_integer_helper<uint32> {};
template<> struct impl_convert<uint64, slice<char>>        : public impl_convert_string_to_integer_helper<uint64> {};
template<> struct impl_convert<uint64, slice<char>*>       : public impl_convert_string_to_integer_helper<uint64> {};
template<> struct impl_convert<uint64, slice<char const>>  : public impl_convert_string_to_integer_helper<uint64> {};
template<> struct impl_convert<uint64, slice<char const>*> : public impl_convert_string_to_integer_helper<uint64> {};

//
// Conversion: Integer -> String
//

slice<char> ImplConvertIntegerToString(int8  Integer, slice<char> Buffer, bool* Success);
slice<char> ImplConvertIntegerToString(int16 Integer, slice<char> Buffer, bool* Success);
slice<char> ImplConvertIntegerToString(int32 Integer, slice<char> Buffer, bool* Success);
slice<char> ImplConvertIntegerToString(int64 Integer, slice<char> Buffer, bool* Success);
slice<char> ImplConvertIntegerToString(uint8  Integer, slice<char> Buffer, bool* Success);
slice<char> ImplConvertIntegerToString(uint16 Integer, slice<char> Buffer, bool* Success);
slice<char> ImplConvertIntegerToString(uint32 Integer, slice<char> Buffer, bool* Success);
slice<char> ImplConvertIntegerToString(uint64 Integer, slice<char> Buffer, bool* Success);

template<typename CharType, typename IntegerType>
struct impl_convert_integer_to_string_helper
{
  using MutableCharType = rm_ref<CharType>;

  static inline slice<CharType>
  Do(IntegerType Integer, slice<MutableCharType> Buffer, bool* Success = nullptr)
  {
    return ImplConvertIntegerToString(Integer, Buffer, Success);
  }
};

template<> struct impl_convert<slice<char>,        int8>     : public impl_convert_integer_to_string_helper<char,       int8>   {};
template<> struct impl_convert<slice<char>*,       int8>     : public impl_convert_integer_to_string_helper<char,       int8>   {};
template<> struct impl_convert<slice<char const>,  int8>     : public impl_convert_integer_to_string_helper<char const, int8>   {};
template<> struct impl_convert<slice<char const>*, int8>     : public impl_convert_integer_to_string_helper<char const, int8>   {};
template<> struct impl_convert<slice<char>,        int16>    : public impl_convert_integer_to_string_helper<char,       int16>  {};
template<> struct impl_convert<slice<char>*,       int16>    : public impl_convert_integer_to_string_helper<char,       int16>  {};
template<> struct impl_convert<slice<char const>,  int16>    : public impl_convert_integer_to_string_helper<char const, int16>  {};
template<> struct impl_convert<slice<char const>*, int16>    : public impl_convert_integer_to_string_helper<char const, int16>  {};
template<> struct impl_convert<slice<char>,        int32>    : public impl_convert_integer_to_string_helper<char,       int32>  {};
template<> struct impl_convert<slice<char>*,       int32>    : public impl_convert_integer_to_string_helper<char,       int32>  {};
template<> struct impl_convert<slice<char const>,  int32>    : public impl_convert_integer_to_string_helper<char const, int32>  {};
template<> struct impl_convert<slice<char const>*, int32>    : public impl_convert_integer_to_string_helper<char const, int32>  {};
template<> struct impl_convert<slice<char>,        int64>    : public impl_convert_integer_to_string_helper<char,       int64>  {};
template<> struct impl_convert<slice<char>*,       int64>    : public impl_convert_integer_to_string_helper<char,       int64>  {};
template<> struct impl_convert<slice<char const>,  int64>    : public impl_convert_integer_to_string_helper<char const, int64>  {};
template<> struct impl_convert<slice<char const>*, int64>    : public impl_convert_integer_to_string_helper<char const, int64>  {};

template<> struct impl_convert<slice<char>,          uint8>  : public impl_convert_integer_to_string_helper<char,       uint8>  {};
template<> struct impl_convert<slice<char>*,         uint8>  : public impl_convert_integer_to_string_helper<char,       uint8>  {};
template<> struct impl_convert<slice<char const>,    uint8>  : public impl_convert_integer_to_string_helper<char const, uint8>  {};
template<> struct impl_convert<slice<char const>*,   uint8>  : public impl_convert_integer_to_string_helper<char const, uint8>  {};
template<> struct impl_convert<slice<char>,          uint16> : public impl_convert_integer_to_string_helper<char,       uint16> {};
template<> struct impl_convert<slice<char>*,         uint16> : public impl_convert_integer_to_string_helper<char,       uint16> {};
template<> struct impl_convert<slice<char const>,    uint16> : public impl_convert_integer_to_string_helper<char const, uint16> {};
template<> struct impl_convert<slice<char const>*,   uint16> : public impl_convert_integer_to_string_helper<char const, uint16> {};
template<> struct impl_convert<slice<char>,          uint32> : public impl_convert_integer_to_string_helper<char,       uint32> {};
template<> struct impl_convert<slice<char>*,         uint32> : public impl_convert_integer_to_string_helper<char,       uint32> {};
template<> struct impl_convert<slice<char const>,    uint32> : public impl_convert_integer_to_string_helper<char const, uint32> {};
template<> struct impl_convert<slice<char const>*,   uint32> : public impl_convert_integer_to_string_helper<char const, uint32> {};
template<> struct impl_convert<slice<char>,          uint64> : public impl_convert_integer_to_string_helper<char,       uint64> {};
template<> struct impl_convert<slice<char>*,         uint64> : public impl_convert_integer_to_string_helper<char,       uint64> {};
template<> struct impl_convert<slice<char const>,    uint64> : public impl_convert_integer_to_string_helper<char const, uint64> {};
template<> struct impl_convert<slice<char const>*,   uint64> : public impl_convert_integer_to_string_helper<char const, uint64> {};

// ====================
// === Inline Files ===
// ====================

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
  return ExtractPathDirectoryAndFileName(AsConst(Path),
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

template<typename CharType>
struct impl_find_file_extension
{
  static slice<CharType>
  Do(slice<CharType> FileName, path_options Options)
  {
    auto Seek = OnePastLast(FileName);
    size_t Num = 0;
    while(Num < FileName.Num)
    {
      ++Num;
      --Seek;

      auto const Character = *Seek;

      // Found the path separator before the extension marker?
      if(Character == Options.Separator)
        break;

      if(Character == '.')
        return Slice(Num, Seek);
    }

    // Return an empty slice that still points to the correct memory.
    return { 0, OnePastLast(FileName) };
  }
};

auto
::FindFileExtension(slice<char> FileName, path_options Options)
  -> slice<char>
{
  return impl_find_file_extension<char>::Do(FileName, Options);
}

auto
::FindFileExtension(slice<char const> FileName, path_options Options)
  -> slice<char const>
{
  return impl_find_file_extension<char const>::Do(FileName, Options);
}

