
// This file was generated using the tool Utilities/GenerateSingleHeader.py
// from the Backbone project.

#include "Backbone.hpp"


// ===================================
// === Source: Backbone/Memory.cpp ===
// ===================================

#include <cstring>


auto
::MemCopyBytes(void* Destination, void const* Source, size_t NumBytes)
  -> void
{
  // Using memmove so that Destination and Source may overlap.
  std::memmove(Destination, Source, NumBytes);
}

auto
MemSetBytes(void* Destination, int Value, size_t NumBytes)
  -> void
{
  std::memset(Destination, Value, NumBytes);
}

auto
MemEqualBytes(void const* A, void const* B, size_t NumBytes)
  -> bool
{
  return MemCompareBytes(A, B, NumBytes) == 0;
}

auto
MemCompareBytes(void const* A, void const* B, size_t NumBytes)
  -> int
{
  return std::memcmp(A, B, NumBytes);
}

