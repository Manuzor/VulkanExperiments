#pragma once

#include "CoreAPI.hpp"

#include <Backbone.hpp>


//
// Types
//

union vec2
{
  struct{ float X; float Y; };
  float Data[2];
};

union vec3
{
  struct{ float X; float Y; float Z; };
  float Data[3];
};

union vec4
{
  struct{ float X; float Y; float Z; float W; };
  float Data[4];
};

/// Column-Major
union mat4x4
{
  using float4 = float[4];
  using float4x4 = float4[4];

  struct
  {
    float4 Col0;
    float4 Col1;
    float4 Col2;
    float4 Col3;
  };

  struct
  {
    float M00, M01, M02, M03;
    float M10, M11, M12, M13;
    float M20, M21, M22, M23;
    float M30, M31, M32, M33;
  };

  float4x4 Data;

  inline
  float4&
  operator [](size_t Index)
  {
    return Data[Index];
  }

  constexpr
  float4 const&
  operator [](size_t Index) const
  {
    return Data[Index];
  }
};

union quaternion
{
  struct
  {
    float X;
    float Y;
    float Z;
    float W;
  };

  struct
  {
    vec3 Direction;
    angle Angle;
  };

  float Data[4];
};


//
// Construction Functions: Vec2, Vec3, Vec4
//

vec2 constexpr Vec2(float XY) {return { XY, XY }; }
vec2 constexpr Vec2(float X, float Y) {return { X, Y }; }


vec3 constexpr Vec3(float XYZ) {return { XYZ, XYZ, XYZ }; }
vec3 constexpr Vec3(float X, float Y, float Z) {return { X, Y, Z }; }
vec3 constexpr Vec3(float X, vec2 YZ) { return Vec3(X, YZ.X, YZ.Y); }
vec3 constexpr Vec3(vec2 XY, float Z) { return Vec3(XY.X, XY.Y, Z); }

vec3 constexpr Vec3FromXYZ(vec4 Vec) { return Vec3(Vec.X, Vec.Y, Vec.Z); }


vec4 constexpr Vec4(float XYZW) {return { XYZW, XYZW, XYZW, XYZW }; }
vec4 constexpr Vec4(float X, float Y, float Z, float W) {return { X, Y, Z, W }; }
vec4 constexpr Vec4(float X, float Y, vec2 ZW) { return Vec4(   X,    Y, ZW.X, ZW.Y); }
vec4 constexpr Vec4(float X, vec2 YZ, float W) { return Vec4(   X, YZ.X, YZ.Y,    W); }
vec4 constexpr Vec4(vec2 XY, float Z, float W) { return Vec4(XY.X, XY.Y,    Z,    W); }
vec4 constexpr Vec4(vec2 XY, vec2 ZW)          { return Vec4(XY.X, XY.Y, ZW.X, ZW.Y); }
vec4 constexpr Vec4(float X, vec3 YZW) { return Vec4(X, YZW.X, YZW.Y, YZW.Z); }
vec4 constexpr Vec4(vec3 XYZ, float W) { return Vec4(XYZ.X, XYZ.Y, XYZ.Z, W); }


//
// Constants: vec2
//

constexpr vec2 ZeroVector2      = Vec2(0, 0);
constexpr vec2 UnitScaleVector2 = Vec2(1, 1);
constexpr vec2 UnitXVector2     = Vec2(1, 0);
constexpr vec2 UnitYVector2     = Vec2(0, 1);


//
// Constants: vec3
//

constexpr vec3 ZeroVector3      = Vec3(0, 0, 0);
constexpr vec3 UnitScaleVector3 = Vec3(1, 1, 1);
constexpr vec3 ForwardVector3   = Vec3(1, 0, 0);
constexpr vec3 RightVector3     = Vec3(0, 1, 0);
constexpr vec3 UpVector3        = Vec3(0, 0, 1);


//
// Constants: vec4
//

constexpr vec4 ZeroVector4      = Vec4(0, 0, 0, 0);
constexpr vec4 UnitScaleVector4 = Vec4(1, 1, 1, 1);
constexpr vec4 ForwardVector4   = Vec4(1, 0, 0, 0);
constexpr vec4 RightVector4     = Vec4(0, 1, 0, 0);
constexpr vec4 UpVector4        = Vec4(0, 0, 1, 0);
constexpr vec4 WeightVector4    = Vec4(0, 0, 0, 1);


//
// Construction Functions: mat4x4, quaternion
//

mat4x4 CORE_API
Mat4x4(mat4x4::float4x4 const& Data);

mat4x4 CORE_API
Mat4x4(vec3 XAxis, vec3 YAxis, vec3 ZAxis, vec3 Position = ZeroVector3);


quaternion CORE_API
Quaternion(float Data[4]);

quaternion CORE_API
Quaternion(float X, float Y, float Z, float W);

quaternion CORE_API
Quaternion(vec3 Axis, angle Angle);


//
// Operator: Negate
//

vec2 constexpr operator -(vec2 V) { return Vec2(-V.X, -V.Y); }
vec3 constexpr operator -(vec3 V) { return Vec3(-V.X, -V.Y, -V.Z); }
vec4 constexpr operator -(vec4 V) { return Vec4(-V.X, -V.Y, -V.Z, -V.W); }


//
// Operator: Add
//
vec2 constexpr operator +(vec2 A, vec2 B) { return Vec2(A.X + B.X, A.Y + B.Y); }
vec3 constexpr operator +(vec3 A, vec3 B) { return Vec3(A.X + B.X, A.Y + B.Y, A.Z + B.Z); }
vec4 constexpr operator +(vec4 A, vec4 B) { return Vec4(A.X + B.X, A.Y + B.Y, A.Z + B.Z, A.W + B.W); }


//
// Operator: Subtract
//
vec2 constexpr operator -(vec2 A, vec2 B) { return Vec2(A.X - B.X, A.Y - B.Y); }
vec3 constexpr operator -(vec3 A, vec3 B) { return Vec3(A.X - B.X, A.Y - B.Y, A.Z - B.Z); }
vec4 constexpr operator -(vec4 A, vec4 B) { return Vec4(A.X - B.X, A.Y - B.Y, A.Z - B.Z, A.W - B.W); }


//
// Operator: Multiply with scalar
//
vec2 constexpr operator *(float S, vec2 V) { return Vec2(S * V.X, S * V.Y); }
vec3 constexpr operator *(float S, vec3 V) { return Vec3(S * V.X, S * V.Y, S * V.Z); }
vec4 constexpr operator *(float S, vec4 V) { return Vec4(S * V.X, S * V.Y, S * V.Z, S * V.W); }
vec2 constexpr operator *(vec2 V, float S) { return S * V; }
vec3 constexpr operator *(vec3 V, float S) { return S * V; }
vec4 constexpr operator *(vec4 V, float S) { return S * V; }


//
// Operator: Divide by scalar
//
vec2 constexpr operator /(vec2 V, float S) { return (1 / S) * V; }
vec3 constexpr operator /(vec3 V, float S) { return (1 / S) * V; }
vec4 constexpr operator /(vec4 V, float S) { return (1 / S) * V; }


//
// Algorithms: Dot Product
//

float constexpr Dot(vec2 A, vec2 B) { return A.X * B.X + A.Y * B.Y; }
float constexpr Dot(vec3 A, vec3 B) { return A.X * B.X + A.Y * B.Y + A.Z * B.Z; }
float constexpr Dot(vec4 A, vec4 B) { return A.X * B.X + A.Y * B.Y + A.Z * B.Z + A.W * B.W; }

float constexpr operator |(vec2 A, vec2 B) { return Dot(A, B); }
float constexpr operator |(vec3 A, vec3 B) { return Dot(A, B); }
float constexpr operator |(vec4 A, vec4 B) { return Dot(A, B); }


//
// Algorithms: Length and Squared Length
//

float constexpr LengthSquared(vec2 A) { return A | A; }
float constexpr LengthSquared(vec3 A) { return A | A; }
float constexpr LengthSquared(vec4 A) { return A | A; }

float CORE_API Length(vec2 A);
float CORE_API Length(vec3 A);
float CORE_API Length(vec4 A);


//
// Algorithms: Normalization
//

vec2 CORE_API Normalized(vec2 V);
vec3 CORE_API Normalized(vec3 V);
vec4 CORE_API Normalized(vec4 V);

void CORE_API Normalize(vec2* V);
void CORE_API Normalize(vec3* V);
void CORE_API Normalize(vec4* V);

void CORE_API SafeNormalize(vec2* V);
void CORE_API SafeNormalize(vec3* V);
void CORE_API SafeNormalize(vec4* V);


//
// Algorithms: Cross Product
//
vec3 constexpr Cross(vec3 A, vec3 B) { return Vec3((A.Y * B.Z) - (A.Z * B.Y), (A.Z * B.X) - (A.X * B.Z), (A.X * B.Y) - (A.Y * B.X)); }

vec3 constexpr operator ^(vec3 A, vec3 B) { return Cross(A, B); }

//
// Algorithms: Equality
//
bool constexpr AreNearlyEqual(vec2 A, vec2 B, float Epsilon = 1e-4f) { return AreNearlyEqual(A.X, B.X, Epsilon) && AreNearlyEqual(A.Y, B.Y, Epsilon); }
bool constexpr AreNearlyEqual(vec3 A, vec3 B, float Epsilon = 1e-4f) { return AreNearlyEqual(A.X, B.X, Epsilon) && AreNearlyEqual(A.Y, B.Y, Epsilon) && AreNearlyEqual(A.Z, B.Z, Epsilon); }
bool constexpr AreNearlyEqual(vec4 A, vec4 B, float Epsilon = 1e-4f) { return AreNearlyEqual(A.X, B.X, Epsilon) && AreNearlyEqual(A.Y, B.Y, Epsilon) && AreNearlyEqual(A.Z, B.Z, Epsilon) && AreNearlyEqual(A.W, B.W, Epsilon); }

bool constexpr IsNearlyZero(vec2 Vec, float Epsilon = 1e-4f) { return AreNearlyEqual(Vec, ZeroVector2, Epsilon); }
bool constexpr IsNearlyZero(vec3 Vec, float Epsilon = 1e-4f) { return AreNearlyEqual(Vec, ZeroVector3, Epsilon); }
bool constexpr IsNearlyZero(vec4 Vec, float Epsilon = 1e-4f) { return AreNearlyEqual(Vec, ZeroVector4, Epsilon); }

//
// Algorithms: Quaternions transforming Vectors
//

vec3 CORE_API
TransformDirection(quaternion Quat, vec3 Direction);

vec4 CORE_API
TransformDirection(quaternion Quat, vec4 Direction);

vec3 CORE_API
operator *(quaternion Quat, vec3 Direction);

mat4x4 CORE_API
ToRotationMatrix(quaternion Quat);


//
// Slicing
//
slice<float> constexpr Slice(vec2& V) { return Slice(V.Data); }
slice<float> constexpr Slice(vec3& V) { return Slice(V.Data); }
slice<float> constexpr Slice(vec4& V) { return Slice(V.Data); }

slice<float const> constexpr Slice(vec2 const& V) { return Slice(V.Data); }
slice<float const> constexpr Slice(vec3 const& V) { return Slice(V.Data); }
slice<float const> constexpr Slice(vec4 const& V) { return Slice(V.Data); }

slice<float>       inline    Slice(mat4x4      & Mat) { return Slice(4 * 4, &Mat[0][0]); }
slice<float const> constexpr Slice(mat4x4 const& Mat) { return Slice(4 * 4, &Mat[0][0]); }
