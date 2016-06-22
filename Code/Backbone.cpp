
// This file was generated using the tool Utilities/GenerateSingleHeader.py
// from the Backbone project.

#include "Backbone.hpp"


// ===================================
// === Source: Backbone/Memory.cpp ===
// ===================================

#include <cstring>


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

