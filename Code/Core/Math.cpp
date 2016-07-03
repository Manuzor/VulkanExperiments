#include "Math.hpp"

auto
::Length(vec2 A)
  -> float
{
  auto Result = Sqrt(LengthSquared(A));
  return Result;
}

auto
::Length(vec3 A)
  -> float
{
  auto Result = Sqrt(LengthSquared(A));
  return Result;
}

auto
::Length(vec4 A)
  -> float
{
  auto Result = Sqrt(LengthSquared(A));
  return Result;
}


auto
::Normalized(vec2 V)
  -> vec2
{
  auto Result = V / Length(V);
  return Result;
}

auto
::Normalized(vec3 V)
  -> vec3
{
  auto Result = V / Length(V);
  return Result;
}

auto
::Normalized(vec4 V)
  -> vec4
{
  auto Result = V / Length(V);
  return Result;
}

auto
::Normalize(vec2* V)
  -> void
{
  *V = Normalized(*V);
}

auto
::Normalize(vec3* V)
  -> void
{
  *V = Normalized(*V);
}

auto
::Normalize(vec4* V)
  -> void
{
  *V = Normalized(*V);
}

auto
::SafeNormalize(vec2* V)
  -> void
{
  if(IsNearlyZero(*V))
  {
    *V = ZeroVector2;
  }
  else
  {
    Normalize(V);
  }
}

auto
::SafeNormalize(vec3* V)
  -> void
{
  if(IsNearlyZero(*V))
  {
    *V = ZeroVector3;
  }
  else
  {
    Normalize(V);
  }
}

auto
::SafeNormalize(vec4* V)
  -> void
{
  if(IsNearlyZero(*V))
  {
    *V = ZeroVector4;
  }
  else
  {
    Normalize(V);
  }
}

auto
::Quaternion(float Data[4])
  -> quaternion
{
  quaternion Result;
  MemCopy(4, &Result.Data[0], &Data[0]);
  return Result;
}

auto
Quaternion(float X, float Y, float Z, float W)
  -> quaternion
{
  quaternion Result;
  Result.X = X;
  Result.Y = Y;
  Result.Z = Z;
  Result.W = W;
  return Result;
}

auto
::Quaternion(vec3 Axis, angle Angle)
  -> quaternion
{
  SafeNormalize(&Axis);

  quaternion Quat;
  Quat.W = Cos(Angle * 0.5f);
  float const Sine = Sin(Angle * 0.5f);
  Quat.X = Sine * Axis.X;
  Quat.Y = Sine * Axis.Y;
  Quat.Z = Sine * Axis.Z;

  return Quat;
}

auto
::TransformDirection(quaternion Quat, vec3 Direction)
  -> vec3
{
  auto const Q = Vec3(Quat.X, Quat.Y, Quat.Z);
  auto const T = 2.0f * (Q ^ Direction);
  return Direction + (Quat.W * T) + (Q ^ T);
}

auto
::TransformDirection(quaternion Quat, vec4 Direction)
  -> vec4
{
  auto const Q = Vec3(Quat.X, Quat.Y, Quat.Z);
  auto const XYZ = Vec3FromXYZ(Direction);
  auto const T = 2.0f * (Q ^ XYZ);
  return Vec4(XYZ + (Quat.W * T) + (Q ^ T), Direction.W);
}

auto
::operator *(quaternion Quat, vec3 Direction)
  -> vec3
{
  return TransformDirection(Quat, Direction);
}

auto
::ToRotationMatrix(quaternion Quat)
  -> mat4x4
{
  float const X2 = Quat.X + Quat.X;
  float const Y2 = Quat.Y + Quat.Y;
  float const Z2 = Quat.Z + Quat.Z;

  float const XX = Quat.X * X2;
  float const XY = Quat.X * Y2;
  float const XZ = Quat.X * Z2;

  float const YY = Quat.Y * Y2;
  float const YZ = Quat.Y * Z2;
  float const ZZ = Quat.Z * Z2;

  float const WX = Quat.W * X2;
  float const WY = Quat.W * Y2;
  float const WZ = Quat.W * Z2;

  return Mat4x4({
    {1.0f - (YY + ZZ), XY + WZ,          XZ - WY,          0.0f},
    {XY - WZ,          1.0f - (XX + ZZ), YZ + WX,          0.0f},
    {XZ + WY,          YZ - WX,          1.0f - (XX + YY), 0.0f},
    {0.0f,             0.0f,             0.0f,             1.0f},
  });
}

auto
Mat4x4(mat4x4::float4x4 const& Data)
  -> mat4x4
{
  mat4x4 Result;
  MemCopy(4 * 4, &Result.M00, &Data[0][0]);
  return Result;
}

auto
::Mat4x4(vec3 XAxis, vec3 YAxis, vec3 ZAxis, vec3 Position)
  -> mat4x4
{
  mat4x4 Result;
  Result.M00 = XAxis.X;
  Result.M01 = XAxis.Y;
  Result.M02 = XAxis.Z;
  Result.M03 = 0;
  Result.M10 = YAxis.X;
  Result.M11 = YAxis.Y;
  Result.M12 = YAxis.Z;
  Result.M13 = 0;
  Result.M20 = ZAxis.X;
  Result.M21 = ZAxis.Y;
  Result.M22 = ZAxis.Z;
  Result.M23 = 0;
  return Result;
}
