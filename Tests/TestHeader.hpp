#pragma once

#include <Core/Allocator.hpp>

#include "catch.hpp"

using test_allocator = mallocator;

#include <Core/Math.hpp>

inline std::ostream&
operator <<(std::ostream& OutStream, vec2 const& Vec)
{
  OutStream << "{ X=" << Vec.X << ", Y=" << Vec.Y << '}';
  return OutStream;
}

inline std::ostream&
operator <<(std::ostream& OutStream, vec3 const& Vec)
{
  OutStream << "{ X=" << Vec.X << ", Y=" << Vec.Y << ", Z=" << Vec.Z << '}';
  return OutStream;
}

inline std::ostream&
operator <<(std::ostream& OutStream, vec4 const& Vec)
{
  OutStream << "{ X=" << Vec.X << ", Y=" << Vec.Y << ", Z=" << Vec.Z << ", W=" << Vec.W << '}';
  return OutStream;
}

inline std::ostream&
operator <<(std::ostream& OutStream, quaternion const& Quat)
{
  OutStream << "{ X=" << Quat.X << ", Y=" << Quat.Y << ", Z=" << Quat.Z << ", W=" << Quat.W << '}';
  return OutStream;
}

inline std::ostream&
operator <<(std::ostream& OutStream, mat4x4 const& Mat)
{
  OutStream << '{'
  << "{ " << Mat[0][0] << ", " << Mat[0][1] << ", " << Mat[0][2] << ", " << Mat[0][3] << " }"
  << "{ " << Mat[1][0] << ", " << Mat[1][1] << ", " << Mat[1][2] << ", " << Mat[1][3] << " }"
  << "{ " << Mat[2][0] << ", " << Mat[2][1] << ", " << Mat[2][2] << ", " << Mat[2][3] << " }"
  << "{ " << Mat[3][0] << ", " << Mat[3][1] << ", " << Mat[3][2] << ", " << Mat[3][3] << " }"
  << '}';
  return OutStream;
}
