#pragma once

#include "CoreAPI.hpp"
#include "Array.hpp"

RESERVE_PREFIX(Str);


// Forward declaration of internally used type.
class string_allocator;

CORE_API
allocator_interface*
StringDefaultAllocator();

CORE_API
void
SetStringDefaultAllocator(allocator_interface* Allocator);


/// \note If no allocator is supplied to this string, the default string
/// \allocator will be used.
/// \note ARC: Automatic Reference Counting
class CORE_API arc_string
{
public:

  struct internal
  {
    array<char> Data;
    int RefCount;
  };

  allocator_interface* Allocator{};
  internal* Internal{};

  //
  // Ctor
  //
  arc_string() = default;
  arc_string(allocator_interface& Allocator);

  arc_string(char const* StringPtr);
  arc_string(char const* StringPtr, allocator_interface& Allocator);

  arc_string(slice<char const> Content);
  arc_string(slice<char const> Content, allocator_interface& Allocator);

  arc_string(slice<char> Content);
  arc_string(slice<char> Content, allocator_interface& Allocator);

  //
  // Copy
  //
  arc_string(arc_string const& Copy);

  //
  // Move
  //
  arc_string(arc_string&& Copy);

  //
  // Dtor
  //
  ~arc_string();

  //
  // Assignment
  //
  void operator=(arc_string const& Copy);
  void operator=(arc_string&& Copy);

  void operator=(char const* StringPtr);
  void operator=(slice<char const> Content);
  void operator=(slice<char> Content);
};

/// \note Usually you don't have to call this.
CORE_API
void
StrEnsureInitialized(arc_string& String);

/// Ensures the given string has unique data.
///
/// This is used to implement copy-on-write functionality.
CORE_API
void
StrEnsureUnique(arc_string& String);

/// \note Usually you don't have to call this.
CORE_API
void
StrEnsureZeroTerminated(arc_string& String);

/// Use at your own risk!
///
/// It is recommended to call StrEnsureUnique before using the data.
///
/// Call StrInvalidateInternalData when you've modified the internal data.
CORE_API
array<char>&
StrInternalData(arc_string& String);

/// Use at your own risk!
///
/// It is recommended to call StrEnsureUnique before using the data.
///
/// \note There's no need to call StrInvalidateInternalData since you don't
/// \have write access to the data.
CORE_API
array<char> const&
StrInternalData(arc_string const& String);

/// Call this to trigger some internal string operations to ensure state
/// consistency within the string.
CORE_API
void
StrInvalidateInternalData(arc_string& String);

/// The amount of bytes available to this string.
CORE_API
size_t
StrCapacity(arc_string const& String);

CORE_API
size_t
StrNumBytes(arc_string const& String);

CORE_API
size_t
StrNumChars(arc_string const& String);

CORE_API
bool
StrIsEmpty(arc_string const& String);

CORE_API
char*
StrPtr(arc_string& String);

CORE_API
char const*
StrPtr(arc_string const& String);

CORE_API
slice<char>
Slice(arc_string& String);

CORE_API
slice<char const>
Slice(arc_string const& String);

CORE_API
void
StrClear(arc_string& String);

/// Tries to clear the memory of this string.
///
/// If other strings still reference the internal data, it won't be deleted of
/// course.
CORE_API
void
StrReset(arc_string& String);

/// \see arc_string::operator=()
CORE_API
void
StrSet(arc_string& String, slice<char const> Content);

/// Appends some data to the given string.
///
/// \see operator+=(arc_string&,slice<char const>)
CORE_API
void
StrAppend(arc_string& String, slice<char const> More);

inline void operator +=(arc_string& String, char const* StringPtr)     { StrAppend(String, SliceFromString(StringPtr)); }
inline void operator +=(arc_string& String, slice<char const> Content) { StrAppend(String, Content); }
inline void operator +=(arc_string& String, slice<char> Content)       { StrAppend(String, AsConst(Content)); }

/// Prepends some data to the given string.
/// \see operator+=(slice<char const>,arc_string&)
CORE_API
void
StrPrepend(arc_string& String, slice<char const> More);

/// \see operator+(arc_string&,slice<char const>)
CORE_API
arc_string
StrConcat(arc_string const& String, slice<char const> More);

inline arc_string operator +(arc_string const& String, char const* StringPtr)     { return StrConcat(String, SliceFromString(StringPtr)); }
inline arc_string operator +(arc_string const& String, slice<char const> Content) { return StrConcat(String, Content); }
inline arc_string operator +(arc_string const& String, slice<char> Content)       { return StrConcat(String, AsConst(Content)); }

/// \see operator+(slice<char const>,arc_string&)
CORE_API
arc_string
StrConcat(slice<char const> More, arc_string const& String);

inline arc_string operator +(char const* StringPtr, arc_string const& String)     { return StrConcat(SliceFromString(StringPtr), String); }
inline arc_string operator +(slice<char const> Content, arc_string const& String) { return StrConcat(Content, String); }
inline arc_string operator +(slice<char> Content, arc_string const& String)       { return StrConcat(AsConst(Content), String); }

CORE_API
bool
StrAreEqual(arc_string const& A, arc_string const& B);

CORE_API
bool
StrAreEqual(arc_string const& A, slice<char const> B);

CORE_API
bool
StrAreEqual(slice<char const> A, arc_string const& B);

inline bool operator ==(arc_string const& A, arc_string const& B) { return StrAreEqual(A, B); }
inline bool operator ==(arc_string const& A, slice<char const> B) { return StrAreEqual(A, B); }
inline bool operator ==(slice<char const> A, arc_string const& B) { return StrAreEqual(A, B); }
inline bool operator ==(arc_string const& A, slice<char      > B) { return StrAreEqual(A, AsConst(B)); }
inline bool operator ==(slice<char      > A, arc_string const& B) { return StrAreEqual(AsConst(A), B); }
inline bool operator ==(arc_string const& A, char const* B)       { return StrAreEqual(A, SliceFromString(B)); }
inline bool operator ==(char const* A,       arc_string const& B) { return StrAreEqual(SliceFromString(A), B); }

inline bool operator !=(arc_string const& A, arc_string const& B) { return !(A == B); }
inline bool operator !=(arc_string const& A, slice<char const> B) { return !(A == B); }
inline bool operator !=(slice<char const> A, arc_string const& B) { return !(A == B); }
inline bool operator !=(arc_string const& A, slice<char      > B) { return !(A == B); }
inline bool operator !=(slice<char      > A, arc_string const& B) { return !(A == B); }
inline bool operator !=(arc_string const& A, char const* B)       { return !(A == B); }
inline bool operator !=(char const* A,       arc_string const& B) { return !(A == B); }
