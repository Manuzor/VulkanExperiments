#pragma once

#include "CoreAPI.hpp"
#include "DynamicArray.hpp"


/// \note If no allocator is supplied to this string, the default string
/// \allocator will be used.
/// \note ARC: Automatic Reference Counting
class CORE_API arc_string
{
public:

  struct internal
  {
    dynamic_array<char> Data;
    int RefCount;
  };

  allocator_interface* Allocator{};
  internal* Internal{};

  //
  // Ctor
  //
  arc_string() = default;

  arc_string(char const* StringPtr);
  arc_string(char const* StringPtr, allocator_interface* Allocator);

  arc_string(slice<char const> Content);
  arc_string(slice<char const> Content, allocator_interface* Allocator);

  arc_string(slice<char> Content);
  arc_string(slice<char> Content, allocator_interface* Allocator);

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

CORE_API
void
StrSet(arc_string& String, slice<char const> Content);

/// Appends some data to the given string.
CORE_API
void
StrAppend(arc_string& String, slice<char const> More);

inline void operator +=(arc_string& String, char const* StringPtr)     { StrAppend(String, SliceFromString(StringPtr)); }
inline void operator +=(arc_string& String, slice<char const> Content) { StrAppend(String, Content); }
inline void operator +=(arc_string& String, slice<char> Content)       { StrAppend(String, AsConst(Content)); }

CORE_API
arc_string
StrConcat(arc_string const& String, slice<char const> More);

inline arc_string operator +(arc_string const& String, char const* StringPtr)     { return StrConcat(String, SliceFromString(StringPtr)); }
inline arc_string operator +(arc_string const& String, slice<char const> Content) { return StrConcat(String, Content); }
inline arc_string operator +(arc_string const& String, slice<char> Content)       { return StrConcat(String, AsConst(Content)); }
