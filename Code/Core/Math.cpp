#include "Math.hpp"

auto
::MatrixMultiply(mat4x4 const& A, mat4x4 const& B)
  -> mat4x4
{
  mat4x4 Result;

  Result[0][0] = A[0][0] * B[0][0] + A[0][1] * B[1][0] + A[0][2] * B[2][0] + A[0][3] * B[3][0];
  Result[0][1] = A[0][0] * B[0][1] + A[0][1] * B[1][1] + A[0][2] * B[2][1] + A[0][3] * B[3][1];
  Result[0][2] = A[0][0] * B[0][2] + A[0][1] * B[1][2] + A[0][2] * B[2][2] + A[0][3] * B[3][2];
  Result[0][3] = A[0][0] * B[0][3] + A[0][1] * B[1][3] + A[0][2] * B[2][3] + A[0][3] * B[3][3];

  Result[1][0] = A[1][0] * B[0][0] + A[1][1] * B[1][0] + A[1][2] * B[2][0] + A[1][3] * B[3][0];
  Result[1][1] = A[1][0] * B[0][1] + A[1][1] * B[1][1] + A[1][2] * B[2][1] + A[1][3] * B[3][1];
  Result[1][2] = A[1][0] * B[0][2] + A[1][1] * B[1][2] + A[1][2] * B[2][2] + A[1][3] * B[3][2];
  Result[1][3] = A[1][0] * B[0][3] + A[1][1] * B[1][3] + A[1][2] * B[2][3] + A[1][3] * B[3][3];

  Result[2][0] = A[2][0] * B[0][0] + A[2][1] * B[1][0] + A[2][2] * B[2][0] + A[2][3] * B[3][0];
  Result[2][1] = A[2][0] * B[0][1] + A[2][1] * B[1][1] + A[2][2] * B[2][1] + A[2][3] * B[3][1];
  Result[2][2] = A[2][0] * B[0][2] + A[2][1] * B[1][2] + A[2][2] * B[2][2] + A[2][3] * B[3][2];
  Result[2][3] = A[2][0] * B[0][3] + A[2][1] * B[1][3] + A[2][2] * B[2][3] + A[2][3] * B[3][3];

  Result[3][0] = A[3][0] * B[0][0] + A[3][1] * B[1][0] + A[3][2] * B[2][0] + A[3][3] * B[3][0];
  Result[3][1] = A[3][0] * B[0][1] + A[3][1] * B[1][1] + A[3][2] * B[2][1] + A[3][3] * B[3][1];
  Result[3][2] = A[3][0] * B[0][2] + A[3][1] * B[1][2] + A[3][2] * B[2][2] + A[3][3] * B[3][2];
  Result[3][3] = A[3][0] * B[0][3] + A[3][1] * B[1][3] + A[3][2] * B[2][3] + A[3][3] * B[3][3];

  return Result;
}

auto
::operator *(mat4x4 const& A, mat4x4 const& B)
  -> mat4x4
{
  return MatrixMultiply(A, B);
}

auto
::operator *(quaternion const& A, quaternion const& B)
  -> quaternion
{
  return Quaternion(
    (A.W * B.X) + (A.X * B.W) + (A.Y * B.Z) - (A.Z * B.Y),
    (A.W * B.Y) + (A.Y * B.W) + (A.Z * B.X) - (A.X * B.Z),
    (A.W * B.Z) + (A.Z * B.W) + (A.X * B.Y) - (A.Y * B.X),
    (A.W * B.W) - (A.X * B.X) - (A.Y * B.Y) - (A.Z * B.Z)
  );
}

auto
::operator *(transform const& A, transform const& B)
  -> transform
{
  transform Result;
  Result.Rotation = B.Rotation * A.Rotation;
  Result.Scale = ComponentwiseMultiply(A.Scale, B.Scale);
  Result.Translation = B.Rotation * (ComponentwiseMultiply(B.Scale, A.Translation)) + B.Translation;
  return Result;
}

auto
::Length(vec2 const& Vec)
  -> float
{
  auto Result = Sqrt(LengthSquared(Vec));
  return Result;
}

auto
::Length(vec3 const& Vec)
  -> float
{
  auto Result = Sqrt(LengthSquared(Vec));
  return Result;
}

auto
::Length(vec4 const& Vec)
  -> float
{
  auto Result = Sqrt(LengthSquared(Vec));
  return Result;
}

auto
Length(quaternion const& Quat)
  -> float
{
  auto Result = Sqrt(LengthSquared(Quat));
  return Result;
}


auto
::Normalized(vec2 const& Vec)
  -> vec2
{
  auto Result = Vec / Length(Vec);
  return Result;
}

auto
::Normalized(vec3 const& Vec)
  -> vec3
{
  auto Result = Vec / Length(Vec);
  return Result;
}

auto
::Normalized(vec4 const& Vec)
  -> vec4
{
  auto Result = Vec / Length(Vec);
  return Result;
}

auto
::Normalize(vec2* Vec)
  -> void
{
  *Vec = Normalized(*Vec);
}

auto
::Normalize(vec3* Vec)
  -> void
{
  *Vec = Normalized(*Vec);
}

auto
::Normalize(vec4* Vec)
  -> void
{
  *Vec = Normalized(*Vec);
}

auto
::SafeNormalized(vec2 const& Vec, float Epsilon)
  -> vec2
{
  return IsNearlyZero(Vec, Epsilon) ? ZeroVector2 : Normalized(Vec);
}

auto
::SafeNormalized(vec3 const& Vec, float Epsilon)
  -> vec3
{
  return IsNearlyZero(Vec, Epsilon) ? ZeroVector3 : Normalized(Vec);
}

auto
::SafeNormalized(vec4 const& Vec, float Epsilon)
  -> vec4
{
  return IsNearlyZero(Vec, Epsilon) ? ZeroVector4 : Normalized(Vec);
}

auto
::SafeNormalize(vec2* Vec, float Epsilon)
  -> void
{
  *Vec = SafeNormalized(*Vec, Epsilon);
}

auto
::SafeNormalize(vec3* Vec, float Epsilon)
  -> void
{
  *Vec = SafeNormalized(*Vec, Epsilon);
}

auto
::SafeNormalize(vec4* Vec, float Epsilon)
  -> void
{
  *Vec = SafeNormalized(*Vec, Epsilon);
}

auto
::Normalized(quaternion const& Quat)
  -> quaternion
{
  auto Len = Length(Quat);
  return { Quat.X / Len, Quat.Y / Len, Quat.Z / Len, Quat.W / Len };
}

auto
::SafeNormalized(quaternion const& Quat, float Epsilon)
  -> quaternion
{
  auto Len = Length(Quat);
  if(Len > Epsilon)
    return { Quat.X / Len, Quat.Y / Len, Quat.Z / Len, Quat.W / Len };

  return IdentityQuaternion;
}

auto
::SafeNormalize(quaternion* Quat, float Epsilon)
  -> void
{
  *Quat = SafeNormalized(*Quat, Epsilon);
}


auto
::Quaternion(vec3 const& Axis, angle Angle)
  -> quaternion
{
  auto NormalizedAxis = SafeNormalized(Axis);

  quaternion Quat;
  Quat.W = Cos(Angle * 0.5f);
  float const Sine = Sin(Angle * 0.5f);
  Quat.X = Sine * NormalizedAxis.X;
  Quat.Y = Sine * NormalizedAxis.Y;
  Quat.Z = Sine * NormalizedAxis.Z;

  return Quat;
}

auto
::Quaternion(mat4x4 const& Mat)
  -> quaternion
{
  if(IsNearlyZero(ScaledXAxis(Mat)) || IsNearlyZero(ScaledYAxis(Mat)) || IsNearlyZero(ScaledZAxis(Mat)))
  {
    return IdentityQuaternion;
  }

  quaternion Result;

  float HalfInvSqrt;

  // Check diagonal (trace)
  const float Trace = Mat[0][0] + Mat[1][1] + Mat[2][2];

  if(Trace > 0.0f)
  {
    float InvS = InvSqrt(Trace + 1.0f);
    Result.W = 0.5f * (1.0f / InvS);
    HalfInvSqrt = 0.5f * InvS;

    Result.X = (Mat[1][2] - Mat[2][1]) * HalfInvSqrt;
    Result.Y = (Mat[2][0] - Mat[0][2]) * HalfInvSqrt;
    Result.Z = (Mat[0][1] - Mat[1][0]) * HalfInvSqrt;
  }
  else
  {
    // diagonal is negative
    int Index = 0;

    if(Mat[1][1] > Mat[0][0])
      Index = 1;

    if(Mat[2][2] > Mat[Index][Index])
      Index = 2;

    int const Next[3] = { 1, 2, 0 };
    int const j = Next[Index];
    int const k = Next[j];

    HalfInvSqrt = Mat[Index][Index] - Mat[j][j] - Mat[k][k] + 1.0f;

    float InvS = InvSqrt(HalfInvSqrt);

    Result.Data[Index] = 0.5f * (1.0f / InvS);

    HalfInvSqrt = 0.5f * InvS;

    Result.Data[3] = (Mat[j][k] - Mat[k][j]) * HalfInvSqrt;
    Result.Data[j] = (Mat[Index][j] + Mat[j][Index]) * HalfInvSqrt;
    Result.Data[k] = (Mat[Index][k] + Mat[k][Index]) * HalfInvSqrt;
  }

  SafeNormalize(&Result);
  return Result;
}

auto
::TransformDirection(quaternion const& Quat, vec3 const& Direction)
  -> vec3
{
  auto const Q = Vec3(Quat.X, Quat.Y, Quat.Z);
  auto const T = 2.0f * (Q ^ Direction);
  return Direction + (Quat.W * T) + (Q ^ T);
}

auto
::TransformDirection(quaternion const& Quat, vec4 const& Direction)
  -> vec4
{
  auto const Q = Vec3(Quat.X, Quat.Y, Quat.Z);
  auto const XYZ = Vec3FromXYZ(Direction);
  auto const T = 2.0f * (Q ^ XYZ);
  return Vec4(XYZ + (Quat.W * T) + (Q ^ T), Direction.W);
}

auto
::operator *(quaternion const& Quat, vec3 const& Direction)
  -> vec3
{
  return TransformDirection(Quat, Direction);
}

auto
::TransformDirection(mat4x4 const& Mat, vec4 const& Vec)
  -> vec4
{
  vec4 Result;

  Result.Data[0] = Mat[0][0] * Vec.Data[0] + Mat[1][0] * Vec.Data[1] + Mat[2][0] * Vec.Data[2] + Mat[3][0] * Vec.Data[3];
  Result.Data[1] = Mat[0][1] * Vec.Data[0] + Mat[1][1] * Vec.Data[1] + Mat[2][1] * Vec.Data[2] + Mat[3][1] * Vec.Data[3];
  Result.Data[2] = Mat[0][2] * Vec.Data[0] + Mat[1][2] * Vec.Data[1] + Mat[2][2] * Vec.Data[2] + Mat[3][2] * Vec.Data[3];
  Result.Data[3] = Mat[0][3] * Vec.Data[0] + Mat[1][3] * Vec.Data[1] + Mat[2][3] * Vec.Data[2] + Mat[3][3] * Vec.Data[3];

  return Result;
}

auto
::TransformDirection(mat4x4 const& Mat, vec3 const& Vec)
  -> vec3
{
  return Vec3FromXYZ(TransformDirection(Mat, Vec4(Vec, 0)));
}

auto
::TransformPosition(mat4x4 const& Mat, vec3 const& Vec)
  -> vec3
{
  return Vec3FromXYZ(TransformDirection(Mat, Vec4(Vec, 1)));
}

auto
::InverseTransformDirection(mat4x4 const& Mat, vec4 const& Vec)
  -> vec4
{
  vec4 Result;

  auto Inverted = SafeInverted(Mat);

  Result.Data[0] = Inverted[0][0] * Vec.Data[0] + Inverted[1][0] * Vec.Data[1] + Inverted[2][0] * Vec.Data[2] + Inverted[3][0] * Vec.Data[3];
  Result.Data[1] = Inverted[0][1] * Vec.Data[0] + Inverted[1][1] * Vec.Data[1] + Inverted[2][1] * Vec.Data[2] + Inverted[3][1] * Vec.Data[3];
  Result.Data[2] = Inverted[0][2] * Vec.Data[0] + Inverted[1][2] * Vec.Data[1] + Inverted[2][2] * Vec.Data[2] + Inverted[3][2] * Vec.Data[3];
  Result.Data[3] = Inverted[0][3] * Vec.Data[0] + Inverted[1][3] * Vec.Data[1] + Inverted[2][3] * Vec.Data[2] + Inverted[3][3] * Vec.Data[3];

  return Result;
}

auto
::InverseTransformDirection(mat4x4 const& Mat, vec3 const& Vec)
  -> vec3
{
  return Vec3FromXYZ(InverseTransformDirection(Mat, Vec4(Vec, 0)));
}

auto
::InverseTransformPosition(mat4x4 const& Mat, vec3 const& Vec)
  -> vec3
{
  return Vec3FromXYZ(InverseTransformDirection(Mat, Vec4(Vec, 1)));
}

auto
::TransformDirection(quaternion const& Quat, vec3 const& Direction)
  -> vec3
{
  auto const Q = Vec3(Quat.X, Quat.Y, Quat.Z);
  auto const T = 2.0f * (Q ^ Direction);
  return Direction + (Quat.W * T) + (Q ^ T);
}

auto
::TransformDirection(quaternion const& Quat, vec4 const& Direction)
  -> vec4
{
  auto const XYZ = Vec3FromXYZ(Direction);
  auto const Q = Vec3(Quat.X, Quat.Y, Quat.Z);
  auto const T = 2.0f * (Q ^ XYZ);
  return Vec4(XYZ + (Quat.W * T) + (Q ^ T), Direction.W);
}

auto
::InverseTransformDirection(quaternion const& Quat, vec3 const& Direction)
  -> vec3
{
  auto const Q = Vec3(-Quat.X, -Quat.Y, -Quat.Z);
  auto const T = 2.0f * (Q ^ Direction);
  return Direction + (Quat.W * T) + (Q ^ T);
}

auto
::InverseTransformDirection(quaternion const& Quat, vec4 const& Direction)
  -> vec4
{
  auto const XYZ = Vec3FromXYZ(Direction);
  auto const Q = Vec3(-Quat.X, -Quat.Y, -Quat.Z);
  auto const T = 2.0f * (Q ^ XYZ);
  return Vec4(XYZ + (Quat.W * T) + (Q ^ T), Direction.W);
}

auto
::TransformDirection(transform const& Transform, vec3 const& Vec)
  -> vec3
{
  return Transform.Rotation * ComponentwiseMultiply(Transform.Scale, Vec);
}

auto
::TransformPosition(transform const& Transform, vec3 const& Vec)
  -> vec3
{
  return Transform.Rotation * ComponentwiseMultiply(Transform.Scale, Vec) + Transform.Translation;
}

auto
::InverseTransformDirection(transform const& Transform, vec3 const& Vec)
  -> vec3
{
  return InverseTransformDirection(Transform.Rotation, Vec) * Reciprocal(Transform.Scale, 0);
}

auto
::InverseTransformPosition(transform const& Transform, vec3 const& Vec)
  -> vec3
{
  return InverseTransformDirection(Transform.Rotation, Vec - Transform.Translation) * Reciprocal(Transform.Scale, 0);
}

auto
::Mat4x4(float const(&Col0)[4],
         float const(&Col1)[4],
         float const(&Col2)[4],
         float const(&Col3)[4])
  -> mat4x4
{
  mat4x4 Result;
  MemCopy(4, &Result.M00, &Col0[0]);
  MemCopy(4, &Result.M10, &Col1[0]);
  MemCopy(4, &Result.M20, &Col2[0]);
  MemCopy(4, &Result.M30, &Col3[0]);
  return Result;
}

auto
::Mat4x4(mat4x4::float4x4 const& Data)
  -> mat4x4
{
  mat4x4 Result;
  MemCopy(4 * 4, &Result.M00, &Data[0][0]);
  return Result;
}

auto
::Mat4x4Perspective(angle HalfFOVY, float Width, float Height, float NearPlane, float FarPlane)
  -> mat4x4
{
  auto A = 1.0f / Tan(HalfFOVY);
  auto B = Width / Tan(HalfFOVY) / Height;
  auto C = ((NearPlane == FarPlane) ? 1.0f : FarPlane / (FarPlane - NearPlane));
  auto D = -NearPlane * ((NearPlane == FarPlane) ? 1.0f : FarPlane / (FarPlane - NearPlane));

  return {
    A, 0, 0, 0,
    0, B, 0, 0,
    0, 0, C, 1,
    0, 0, D, 0,
  };
}

auto
::Mat4x4Orthogonal(float Width, float Height, float ZScale, float ZOffset)
  -> mat4x4
{
  auto A = Width  ? (1.0f / Width) : 1.0f;
  auto B = Height ? (1.0f / Height) : 1.0f;
  auto C = ZScale;
  auto D = ZOffset * ZScale;

  return {
    A, 0, 0, 0,
    0, B, 0, 0,
    0, 0, C, 0,
    0, 0, D, 1,
  };
}

auto
::Mat4x4LookAt(vec3 const& Target, vec3 const& Position, vec3 const& Up)
  -> mat4x4
{
  auto Direction = Target - Position;
  return Mat4x4LookDir(Direction, Position, Up);
}

auto
::Mat4x4LookDir(vec3 const& Direction, vec3 const& Position, vec3 const& Up)
  -> mat4x4
{
  auto Dir = SafeNormalized(Direction);
  auto Right = Dir ^ SafeNormalized(Up);
  auto NewUp = Right ^ Dir;
  return Mat4x4(Dir, Right, NewUp, Position);
}

auto
::Mat4x4FromPositionRotation(vec3 const& Position, quaternion const& Rotation)
  -> mat4x4
{
  return Mat4x4FromPositionRotationScale(Position, Rotation, UnitScaleVector3);
}

auto
::Mat4x4FromPositionRotationScale(vec3 const& Position, quaternion const& Rotation, vec3 const& Scale)
  -> mat4x4
{
  mat4x4 Result;

  float const X2 = Rotation.X + Rotation.X;
  float const Y2 = Rotation.Y + Rotation.Y;
  float const Z2 = Rotation.Z + Rotation.Z;

  float const XX2 = Rotation.X * X2;
  float const YY2 = Rotation.Y * Y2;
  float const ZZ2 = Rotation.Z * Z2;
  float const XY2 = Rotation.X * Y2;
  float const WZ2 = Rotation.W * Z2;
  float const YZ2 = Rotation.Y * Z2;
  float const WX2 = Rotation.W * X2;
  float const XZ2 = Rotation.X * Z2;
  float const WY2 = Rotation.W * Y2;

  Result[0][0] = (1.0f - (YY2 + ZZ2)) * Scale.X;
  Result[0][1] = (XY2 + WZ2) * Scale.X;
  Result[0][2] = (XZ2 - WY2) * Scale.X;
  Result[1][0] = (XY2 - WZ2) * Scale.Y;
  Result[1][1] = (1.0f - (XX2 + ZZ2)) * Scale.Y;
  Result[1][2] = (YZ2 + WX2) * Scale.Y;
  Result[2][0] = (XZ2 + WY2) * Scale.Z;
  Result[2][1] = (YZ2 - WX2) * Scale.Z;
  Result[2][2] = (1.0f - (XX2 + YY2)) * Scale.Z;

  Result[0][3] = 0.0f;
  Result[1][3] = 0.0f;
  Result[2][3] = 0.0f;
  Result[3][3] = 1.0f;

  Result[3][0] = Position.X;
  Result[3][1] = Position.Y;
  Result[3][2] = Position.Z;

  return Result;
}

auto
::Mat4x4(quaternion const& Quat)
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
::Mat4x4(transform const& Transform)
  -> mat4x4
{
  return Mat4x4FromPositionRotationScale(Transform.Translation, Transform.Rotation, Transform.Scale);
}

auto
::Transpose(mat4x4* Mat)
  -> void
{
  *Mat = Transposed(*Mat);
}

auto
::IsInvertible(mat4x4 const& Mat)
  -> bool
{
  return !IsNearlyZero(Determinant(Mat));
}

auto
::Inverted(mat4x4 const& Mat)
  -> mat4x4
{
  Assert(IsInvertible(Mat));
  Assert(!IsNearlyZero(ScaledXAxis(Mat)) &&
         !IsNearlyZero(ScaledYAxis(Mat)) &&
         !IsNearlyZero(ScaledZAxis(Mat)));

  mat4x4 Result;
  float const InvDet = 1 / Determinant(Mat);

  Result[0][0] = InvDet * (
    Mat[1][2] * Mat[2][3] * Mat[3][1] - Mat[1][3] * Mat[2][2] * Mat[3][1] + Mat[1][3] * Mat[2][1] * Mat[3][2] -
    Mat[1][1] * Mat[2][3] * Mat[3][2] - Mat[1][2] * Mat[2][1] * Mat[3][3] + Mat[1][1] * Mat[2][2] * Mat[3][3]);
  Result[0][1] = InvDet * (
    Mat[0][3] * Mat[2][2] * Mat[3][1] - Mat[0][2] * Mat[2][3] * Mat[3][1] - Mat[0][3] * Mat[2][1] * Mat[3][2] +
    Mat[0][1] * Mat[2][3] * Mat[3][2] + Mat[0][2] * Mat[2][1] * Mat[3][3] - Mat[0][1] * Mat[2][2] * Mat[3][3]);
  Result[0][2] = InvDet * (
    Mat[0][2] * Mat[1][3] * Mat[3][1] - Mat[0][3] * Mat[1][2] * Mat[3][1] + Mat[0][3] * Mat[1][1] * Mat[3][2] -
    Mat[0][1] * Mat[1][3] * Mat[3][2] - Mat[0][2] * Mat[1][1] * Mat[3][3] + Mat[0][1] * Mat[1][2] * Mat[3][3]);
  Result[0][3] = InvDet * (
    Mat[0][3] * Mat[1][2] * Mat[2][1] - Mat[0][2] * Mat[1][3] * Mat[2][1] - Mat[0][3] * Mat[1][1] * Mat[2][2] +
    Mat[0][1] * Mat[1][3] * Mat[2][2] + Mat[0][2] * Mat[1][1] * Mat[2][3] - Mat[0][1] * Mat[1][2] * Mat[2][3]);
  Result[1][0] = InvDet * (
    Mat[1][3] * Mat[2][2] * Mat[3][0] - Mat[1][2] * Mat[2][3] * Mat[3][0] - Mat[1][3] * Mat[2][0] * Mat[3][2] +
    Mat[1][0] * Mat[2][3] * Mat[3][2] + Mat[1][2] * Mat[2][0] * Mat[3][3] - Mat[1][0] * Mat[2][2] * Mat[3][3]);
  Result[1][1] = InvDet * (
    Mat[0][2] * Mat[2][3] * Mat[3][0] - Mat[0][3] * Mat[2][2] * Mat[3][0] + Mat[0][3] * Mat[2][0] * Mat[3][2] -
    Mat[0][0] * Mat[2][3] * Mat[3][2] - Mat[0][2] * Mat[2][0] * Mat[3][3] + Mat[0][0] * Mat[2][2] * Mat[3][3]);
  Result[1][2] = InvDet * (
    Mat[0][3] * Mat[1][2] * Mat[3][0] - Mat[0][2] * Mat[1][3] * Mat[3][0] - Mat[0][3] * Mat[1][0] * Mat[3][2] +
    Mat[0][0] * Mat[1][3] * Mat[3][2] + Mat[0][2] * Mat[1][0] * Mat[3][3] - Mat[0][0] * Mat[1][2] * Mat[3][3]);
  Result[1][3] = InvDet * (
    Mat[0][2] * Mat[1][3] * Mat[2][0] - Mat[0][3] * Mat[1][2] * Mat[2][0] + Mat[0][3] * Mat[1][0] * Mat[2][2] -
    Mat[0][0] * Mat[1][3] * Mat[2][2] - Mat[0][2] * Mat[1][0] * Mat[2][3] + Mat[0][0] * Mat[1][2] * Mat[2][3]);
  Result[2][0] = InvDet * (
    Mat[1][1] * Mat[2][3] * Mat[3][0] - Mat[1][3] * Mat[2][1] * Mat[3][0] + Mat[1][3] * Mat[2][0] * Mat[3][1] -
    Mat[1][0] * Mat[2][3] * Mat[3][1] - Mat[1][1] * Mat[2][0] * Mat[3][3] + Mat[1][0] * Mat[2][1] * Mat[3][3]);
  Result[2][1] = InvDet * (
    Mat[0][3] * Mat[2][1] * Mat[3][0] - Mat[0][1] * Mat[2][3] * Mat[3][0] - Mat[0][3] * Mat[2][0] * Mat[3][1] +
    Mat[0][0] * Mat[2][3] * Mat[3][1] + Mat[0][1] * Mat[2][0] * Mat[3][3] - Mat[0][0] * Mat[2][1] * Mat[3][3]);
  Result[2][2] = InvDet * (
    Mat[0][1] * Mat[1][3] * Mat[3][0] - Mat[0][3] * Mat[1][1] * Mat[3][0] + Mat[0][3] * Mat[1][0] * Mat[3][1] -
    Mat[0][0] * Mat[1][3] * Mat[3][1] - Mat[0][1] * Mat[1][0] * Mat[3][3] + Mat[0][0] * Mat[1][1] * Mat[3][3]);
  Result[2][3] = InvDet * (
    Mat[0][3] * Mat[1][1] * Mat[2][0] - Mat[0][1] * Mat[1][3] * Mat[2][0] - Mat[0][3] * Mat[1][0] * Mat[2][1] +
    Mat[0][0] * Mat[1][3] * Mat[2][1] + Mat[0][1] * Mat[1][0] * Mat[2][3] - Mat[0][0] * Mat[1][1] * Mat[2][3]);
  Result[3][0] = InvDet * (
    Mat[1][2] * Mat[2][1] * Mat[3][0] - Mat[1][1] * Mat[2][2] * Mat[3][0] - Mat[1][2] * Mat[2][0] * Mat[3][1] +
    Mat[1][0] * Mat[2][2] * Mat[3][1] + Mat[1][1] * Mat[2][0] * Mat[3][2] - Mat[1][0] * Mat[2][1] * Mat[3][2]);
  Result[3][1] = InvDet * (
    Mat[0][1] * Mat[2][2] * Mat[3][0] - Mat[0][2] * Mat[2][1] * Mat[3][0] + Mat[0][2] * Mat[2][0] * Mat[3][1] -
    Mat[0][0] * Mat[2][2] * Mat[3][1] - Mat[0][1] * Mat[2][0] * Mat[3][2] + Mat[0][0] * Mat[2][1] * Mat[3][2]);
  Result[3][2] = InvDet * (
    Mat[0][2] * Mat[1][1] * Mat[3][0] - Mat[0][1] * Mat[1][2] * Mat[3][0] - Mat[0][2] * Mat[1][0] * Mat[3][1] +
    Mat[0][0] * Mat[1][2] * Mat[3][1] + Mat[0][1] * Mat[1][0] * Mat[3][2] - Mat[0][0] * Mat[1][1] * Mat[3][2]);
  Result[3][3] = InvDet * (
    Mat[0][1] * Mat[1][2] * Mat[2][0] - Mat[0][2] * Mat[1][1] * Mat[2][0] + Mat[0][2] * Mat[1][0] * Mat[2][1] -
    Mat[0][0] * Mat[1][2] * Mat[2][1] - Mat[0][1] * Mat[1][0] * Mat[2][2] + Mat[0][0] * Mat[1][1] * Mat[2][2]);

  return Result;
}

auto
::Invert(mat4x4* Mat)
  -> void
{
  *Mat = Inverted(*Mat);
}

auto
::SafeInverted(mat4x4 const& Mat)
  -> mat4x4
{
  if(!IsInvertible(Mat) ||
     (IsNearlyZero(ScaledXAxis(Mat)) &&
      IsNearlyZero(ScaledYAxis(Mat)) &&
      IsNearlyZero(ScaledZAxis(Mat)))
    )
  {
    return IdentityMatrix4x4;
  }

  return Inverted(Mat);
}

auto
::SafeInvert(mat4x4* Mat)
  -> void
{
  *Mat = SafeInverted(*Mat);
}


auto
::Determinant(mat4x4 const& Mat)
  -> float
{
  /*
  _         _
  |a, b, c, d|
  |e, f, g, h|
  |i, j, k, l|
  |m, n, o ,p|
  _         _
  */

  /*
          |f, g, h|
  DetA = a|j, k, l|
          |n, o ,p|
  */
  float DetA = Mat[0][0] * (
    (Mat[1][1] * (Mat[2][2] * Mat[3][3] - Mat[2][3] * Mat[3][2])) -
    (Mat[1][2] * (Mat[2][1] * Mat[3][3] - Mat[2][3] * Mat[3][1])) +
    (Mat[1][3] * (Mat[2][1] * Mat[3][2] - Mat[2][2] * Mat[3][1]))
  );

  /*
          |e, g, h|
  DetB = b|i, k, l|
          |m, o ,p|
  */
  float DetB = Mat[0][1] * (
    (Mat[1][0] * (Mat[2][2] * Mat[3][3] - Mat[2][3] * Mat[3][2])) -
    (Mat[1][2] * (Mat[2][0] * Mat[3][3] - Mat[2][3] * Mat[3][0])) +
    (Mat[1][3] * (Mat[2][0] * Mat[3][2] - Mat[2][2] * Mat[3][0]))
  );

  /*
          |e, f, h|
  DetC = c|i, j, l|
          |m, n ,p|
  */
  float DetC = Mat[0][2] * (
    (Mat[1][0] * (Mat[2][1] * Mat[3][3] - Mat[2][3] * Mat[3][1])) -
    (Mat[1][1] * (Mat[2][0] * Mat[3][3] - Mat[2][3] * Mat[3][0])) +
    (Mat[1][3] * (Mat[2][0] * Mat[3][1] - Mat[2][1] * Mat[3][0]))
  );

  /*
          |e, f, g|
  DetD = d|i, j, k|
          |m, n ,o|
  */
  float DetD = Mat[0][3] * (
    (Mat[1][0] * (Mat[2][1] * Mat[3][2] - Mat[2][2] * Mat[3][1])) -
    (Mat[1][1] * (Mat[2][0] * Mat[3][2] - Mat[2][2] * Mat[3][0])) +
    (Mat[1][2] * (Mat[2][0] * Mat[3][1] - Mat[2][1] * Mat[3][0]))
  );

  return DetA - DetB + DetC - DetD;
}

auto
::ScaledXAxis(mat4x4 const& Mat)
  -> vec3
{
  return Vec3FromXYZ(Mat.Col0);
}

auto
::ScaledYAxis(mat4x4 const& Mat)
  -> vec3
{
  return Vec3FromXYZ(Mat.Col1);
}

auto
::ScaledZAxis(mat4x4 const& Mat)
  -> vec3
{
  return Vec3FromXYZ(Mat.Col2);
}

auto
::UnitXAxis(mat4x4 const& Mat)
  -> vec3
{
  return Normalized(ScaledXAxis(Mat));
}

auto
::UnitYAxis(mat4x4 const& Mat)
  -> vec3
{
  return Normalized(ScaledYAxis(Mat));
}

auto
::UnitZAxis(mat4x4 const& Mat)
  -> vec3
{
  return Normalized(ScaledZAxis(Mat));
}

auto
::ForwardVector(transform const& Transform)
  -> vec3
{
  return TransformDirection(Transform, ForwardVector3);
}

auto
::RightVector(transform const& Transform)
  -> vec3
{
  return TransformDirection(Transform, RightVector3);
}

auto
::UpVector(transform const& Transform)
  -> vec3
{
  return TransformDirection(Transform, UpVector3);
}

auto
DirectionAndLength(vec2 const& Vec, vec2* OutDirection, float* OutLength, float Epsilon)
  -> void
{
  *OutLength = Length(Vec);

  if(*OutLength > Epsilon)
  {
    *OutDirection = Vec / *OutLength;
  }
  else
  {
    *OutDirection = ZeroVector2;
  }
}

auto
DirectionAndLength(vec3 const& Vec, vec3* OutDirection, float* OutLength, float Epsilon)
  -> void
{
  *OutLength = Length(Vec);

  if(*OutLength > Epsilon)
  {
    *OutDirection = Vec / *OutLength;
  }
  else
  {
    *OutDirection = ZeroVector3;
  }
}

auto
DirectionAndLength(vec4 const& Vec, vec4* OutDirection, float* OutLength, float Epsilon)
  -> void
{
  *OutLength = Length(Vec);

  if(*OutLength > Epsilon)
  {
    *OutDirection = Vec / *OutLength;
  }
  else
  {
    *OutDirection = ZeroVector4;
  }
}
