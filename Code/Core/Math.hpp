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
    float M00, M01, M02, M03;
    float M10, M11, M12, M13;
    float M20, M21, M22, M23;
    float M30, M31, M32, M33;
  };

  struct
  {
    vec4 Col0;
    vec4 Col1;
    vec4 Col2;
    vec4 Col3;
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

static_assert(sizeof(mat4x4) == 4 * 4 * sizeof(float), "Incorrect size.");

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

struct transform
{
  vec3 Translation;
  quaternion Rotation;
  vec3 Scale;
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
// Construction Functions: mat4x4
//

mat4x4 constexpr
Mat4x4(float M00, float M01, float M02, float M03,
       float M10, float M11, float M12, float M13,
       float M20, float M21, float M22, float M23,
       float M30, float M31, float M32, float M33)
{
  return { M00, M01, M02, M03,
           M10, M11, M12, M13,
           M20, M21, M22, M23,
           M30, M31, M32, M33 };
}

mat4x4 constexpr
Mat4x4(vec4 Col0, vec4 Col1, vec4 Col2, vec4 Col3)
{
  return { Col0.X, Col0.Y, Col0.Z, Col0.W,
           Col1.X, Col1.Y, Col1.Z, Col1.W,
           Col2.X, Col2.Y, Col2.Z, Col2.W,
           Col3.X, Col3.Y, Col3.Z, Col3.W };
}

mat4x4 CORE_API
Mat4x4(float const(&Col0)[4],
       float const(&Col1)[4],
       float const(&Col2)[4],
       float const(&Col3)[4]);

mat4x4 CORE_API
Mat4x4(mat4x4::float4x4 const& Data);

mat4x4 constexpr
Mat4x4(vec3 XAxis, vec3 YAxis, vec3 ZAxis, vec3 Position = ZeroVector3)
{
  return { XAxis.X,    XAxis.Y,    XAxis.Z,    0,
           YAxis.X,    YAxis.Y,    YAxis.Z,    0,
           ZAxis.X,    ZAxis.Y,    ZAxis.Z,    0,
           Position.X, Position.Y, Position.Z, 1 };
}

mat4x4 CORE_API
Mat4x4Perspective(angle HalfFOVY, float Width, float Height, float NearPlane, float FarPlane);

mat4x4 CORE_API
Mat4x4Orthogonal(float Width, float Height, float ZScale, float ZOffset);

mat4x4 CORE_API
Mat4x4LookAt(vec3 Target, vec3 Position, vec3 Up = UpVector3);

mat4x4 CORE_API
Mat4x4LookDir(vec3 Direction, vec3 Position, vec3 Up = UpVector3);

mat4x4 CORE_API
Mat4x4FromPositionRotation(vec3 Position, quaternion Rotation);

mat4x4 CORE_API
Mat4x4FromPositionRotationScale(vec3 Position, quaternion Rotation, vec3 Scale);

mat4x4 CORE_API
Mat4x4(quaternion Quat);

mat4x4 CORE_API
Mat4x4(transform Transform);


//
// Constants: mat4x4
//

constexpr mat4x4 ZeroMatrix4x4 = Mat4x4(0, 0, 0, 0,
                                        0, 0, 0, 0,
                                        0, 0, 0, 0,
                                        0, 0, 0, 0);

constexpr mat4x4 IdentityMatrix4x4 = Mat4x4(1, 0, 0, 0,
                                            0, 1, 0, 0,
                                            0, 0, 1, 0,
                                            0, 0, 0, 1);


//
// Construction Functions: quaternion
//


quaternion constexpr
Quaternion(float Data[4])
{
  return { Data[0], Data[1], Data[2], Data[3] };
}

quaternion constexpr
Quaternion(float X, float Y, float Z, float W)
{
  return { X, Y, Z, W };
}

quaternion CORE_API
Quaternion(vec3 Axis, angle Angle);

quaternion CORE_API
Quaternion(mat4x4 Mat);


//
// Constants: quaternion
//

constexpr quaternion IdentityQuaternion = Quaternion(0, 0, 0, 1);

//
// Construction Functions: transform
//

transform constexpr
Transform(vec3 Translation, quaternion Rotation, vec3 Scale)
{
  return { Translation, Rotation, Scale };
}

//
// Algorithms: Equality
//
bool constexpr operator ==(vec2 A, vec2 B) { return A.X == B.X && A.Y == B.Y; }
bool constexpr operator ==(vec3 A, vec3 B) { return A.X == B.X && A.Y == B.Y && A.Z == B.Z; }
bool constexpr operator ==(vec4 A, vec4 B) { return A.X == B.X && A.Y == B.Y && A.Z == B.Z && A.W == B.W; }
bool constexpr operator ==(quaternion A, quaternion B) { return A.Direction == B.Direction && A.Angle == B.Angle; }
bool constexpr operator ==(mat4x4 A, mat4x4 B) { return A.Col0 == B.Col0 && A.Col1 == B.Col1 && A.Col2 == B.Col2 && A.Col3 == B.Col3; }

bool constexpr operator !=(vec2 A, vec2 B) { return !(A == B); }
bool constexpr operator !=(vec3 A, vec3 B) { return !(A == B); }
bool constexpr operator !=(vec4 A, vec4 B) { return !(A == B); }
bool constexpr operator !=(quaternion A, quaternion B) { return !(A == B); }
bool constexpr operator !=(mat4x4 A, mat4x4 B) { return !(A == B); }

bool constexpr AreNearlyEqual(vec2 A, vec2 B, float Epsilon = 1e-4f) { return AreNearlyEqual(A.X, B.X, Epsilon) && AreNearlyEqual(A.Y, B.Y, Epsilon); }
bool constexpr AreNearlyEqual(vec3 A, vec3 B, float Epsilon = 1e-4f) { return AreNearlyEqual(A.X, B.X, Epsilon) && AreNearlyEqual(A.Y, B.Y, Epsilon) && AreNearlyEqual(A.Z, B.Z, Epsilon); }
bool constexpr AreNearlyEqual(vec4 A, vec4 B, float Epsilon = 1e-4f) { return AreNearlyEqual(A.X, B.X, Epsilon) && AreNearlyEqual(A.Y, B.Y, Epsilon) && AreNearlyEqual(A.Z, B.Z, Epsilon) && AreNearlyEqual(A.W, B.W, Epsilon); }
bool constexpr AreNearlyEqual(quaternion A, quaternion B, float Epsilon = 1e-4f) { return AreNearlyEqual(A.Direction, B.Direction, Epsilon) && AreNearlyEqual(A.Angle, B.Angle, Radians(Epsilon)); }
bool constexpr AreNearlyEqual(mat4x4 A, mat4x4 B, float Epsilon = 1e-4f) { return AreNearlyEqual(A.Col0, B.Col0, Epsilon) && AreNearlyEqual(A.Col1, B.Col1, Epsilon) && AreNearlyEqual(A.Col2, B.Col2, Epsilon) && AreNearlyEqual(A.Col3, B.Col3, Epsilon); }

bool constexpr IsNearlyZero(vec2 Vec, float Epsilon = 1e-4f) { return AreNearlyEqual(Vec, ZeroVector2, Epsilon); }
bool constexpr IsNearlyZero(vec3 Vec, float Epsilon = 1e-4f) { return AreNearlyEqual(Vec, ZeroVector3, Epsilon); }
bool constexpr IsNearlyZero(vec4 Vec, float Epsilon = 1e-4f) { return AreNearlyEqual(Vec, ZeroVector4, Epsilon); }
bool constexpr IsNearlyZero(mat4x4 Mat, float Epsilon = 1e-4f) { return AreNearlyEqual(Mat, ZeroMatrix4x4, Epsilon); }


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

void inline operator +=(vec2& A, vec2 B) { A = A + B; }
void inline operator +=(vec3& A, vec3 B) { A = A + B; }
void inline operator +=(vec4& A, vec4 B) { A = A + B; }


//
// Operator: Subtract
//
vec2 constexpr operator -(vec2 A, vec2 B) { return Vec2(A.X - B.X, A.Y - B.Y); }
vec3 constexpr operator -(vec3 A, vec3 B) { return Vec3(A.X - B.X, A.Y - B.Y, A.Z - B.Z); }
vec4 constexpr operator -(vec4 A, vec4 B) { return Vec4(A.X - B.X, A.Y - B.Y, A.Z - B.Z, A.W - B.W); }

void inline operator -=(vec2& A, vec2 B) { A = A - B; }
void inline operator -=(vec3& A, vec3 B) { A = A - B; }
void inline operator -=(vec4& A, vec4 B) { A = A - B; }


//
// Operator: Multiply
//
vec2 constexpr operator *(float S, vec2 V) { return Vec2(S * V.X, S * V.Y); }
vec3 constexpr operator *(float S, vec3 V) { return Vec3(S * V.X, S * V.Y, S * V.Z); }
vec4 constexpr operator *(float S, vec4 V) { return Vec4(S * V.X, S * V.Y, S * V.Z, S * V.W); }
vec2 constexpr operator *(vec2 V, float S) { return S * V; }
vec3 constexpr operator *(vec3 V, float S) { return S * V; }
vec4 constexpr operator *(vec4 V, float S) { return S * V; }
void inline operator *=(vec2& V, float S) { V = V * S; }
void inline operator *=(vec3& V, float S) { V = V * S; }
void inline operator *=(vec4& V, float S) { V = V * S; }

vec2 constexpr ComponentwiseMultiply(vec2 A, vec2 B) { return Vec2(A.X * B.X, A.Y * B.Y); }
vec3 constexpr ComponentwiseMultiply(vec3 A, vec3 B) { return Vec3(A.X * B.X, A.Y * B.Y, A.Z * B.Z); }
vec4 constexpr ComponentwiseMultiply(vec4 A, vec4 B) { return Vec4(A.X * B.X, A.Y * B.Y, A.Z * B.Z, A.W * B.W); }

vec2 constexpr operator *(vec2 A, vec2 B) { return ComponentwiseMultiply(A, B); }
vec3 constexpr operator *(vec3 A, vec3 B) { return ComponentwiseMultiply(A, B); }
vec4 constexpr operator *(vec4 A, vec4 B) { return ComponentwiseMultiply(A, B); }

void inline operator *=(vec2& A, vec2 B) { A = A * B; }
void inline operator *=(vec3& A, vec3 B) { A = A * B; }
void inline operator *=(vec4& A, vec4 B) { A = A * B; }

mat4x4 CORE_API MatrixMultiply(mat4x4 A, mat4x4 B);

mat4x4 CORE_API operator *(mat4x4 A, mat4x4 B);
void inline operator *=(mat4x4& A, mat4x4 B) { A = A * B; }

quaternion CORE_API operator *(quaternion A, quaternion B);
void inline operator *=(quaternion& A, quaternion B) { A = A * B; }

transform CORE_API operator *(transform A, transform B);
void inline operator *=(transform& A, transform B) { A = A * B; }


//
// Operator: Divide
//
vec2 constexpr operator /(vec2 V, float S) { return (1 / S) * V; }
vec3 constexpr operator /(vec3 V, float S) { return (1 / S) * V; }
vec4 constexpr operator /(vec4 V, float S) { return (1 / S) * V; }

void inline operator /=(vec2& V, float S) { V = (1 / S) * V; }
void inline operator /=(vec3& V, float S) { V = (1 / S) * V; }
void inline operator /=(vec4& V, float S) { V = (1 / S) * V; }


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
float constexpr LengthSquared(quaternion Quat) { return { Quat.X * Quat.X + Quat.Y * Quat.Y + Quat.Z * Quat.Z + Quat.W * Quat.W }; }

float CORE_API Length(vec2 A);
float CORE_API Length(vec3 A);
float CORE_API Length(vec4 A);
float CORE_API Length(quaternion Quat);


//
// Algorithms: Normalization
//
vec2 CORE_API Normalized(vec2 V);
vec3 CORE_API Normalized(vec3 V);
vec4 CORE_API Normalized(vec4 V);

void CORE_API Normalize(vec2* V);
void CORE_API Normalize(vec3* V);
void CORE_API Normalize(vec4* V);

vec2 CORE_API SafeNormalized(vec2 V, float Epsilon = 1e-4f);
vec3 CORE_API SafeNormalized(vec3 V, float Epsilon = 1e-4f);
vec4 CORE_API SafeNormalized(vec4 V, float Epsilon = 1e-4f);

void CORE_API SafeNormalize(vec2* V, float Epsilon = 1e-4f);
void CORE_API SafeNormalize(vec3* V, float Epsilon = 1e-4f);
void CORE_API SafeNormalize(vec4* V, float Epsilon = 1e-4f);

quaternion CORE_API Normalized(quaternion Quat);
quaternion CORE_API SafeNormalized(quaternion Quat, float Epsilon = 1e-4f);
void CORE_API SafeNormalize(quaternion* Quat, float Epsilon = 1e-4f);

//
// Algorithms: Cross Product
//
vec3 constexpr Cross(vec3 A, vec3 B) { return Vec3((A.Y * B.Z) - (A.Z * B.Y),
                                                   (A.Z * B.X) - (A.X * B.Z),
                                                   (A.X * B.Y) - (A.Y * B.X)); }

vec3 constexpr operator ^(vec3 A, vec3 B) { return Cross(A, B); }

void inline operator ^=(vec3& A, vec3 B) { A = A ^ B; }

//
// Algorithms: Reciprocal
//
vec2 constexpr
Reciprocal(vec2 Vec, float ZeroCase = 3.4e38f)
{
  return Vec2(Vec.X ? 1 / Vec.X : ZeroCase,
              Vec.Y ? 1 / Vec.Y : ZeroCase);
}

vec3 constexpr
Reciprocal(vec3 Vec, float ZeroCase = 3.4e38f)
{
  return Vec3(Vec.X ? 1 / Vec.X : ZeroCase,
              Vec.Y ? 1 / Vec.Y : ZeroCase,
              Vec.Z ? 1 / Vec.Z : ZeroCase);
}

vec4 constexpr
Reciprocal(vec4 Vec, float ZeroCase = 3.4e38f)
{
  return Vec4(Vec.X ? 1 / Vec.X : ZeroCase,
              Vec.Y ? 1 / Vec.Y : ZeroCase,
              Vec.Z ? 1 / Vec.Z : ZeroCase,
              Vec.W ? 1 / Vec.W : ZeroCase);
}

//
// Algorithms: Transforming Vectors
//
vec3 CORE_API
TransformDirection(quaternion Quat, vec3 Direction);

vec4 CORE_API
TransformDirection(quaternion Quat, vec4 Direction);

vec3 CORE_API
operator *(quaternion Quat, vec3 Direction);

vec4 CORE_API
TransformDirection(mat4x4 Mat, vec4 Vec);

vec3 CORE_API
TransformDirection(mat4x4 Mat, vec3 Vec);

vec3 CORE_API
TransformPosition(mat4x4 Mat, vec3 Vec);

vec4 CORE_API
InverseTransformDirection(mat4x4 Mat, vec4 Vec);

vec3 CORE_API
InverseTransformDirection(mat4x4 Mat, vec3 Vec);

vec3 CORE_API
InverseTransformPosition(mat4x4 Mat, vec3 Vec);

vec3 CORE_API
TransformDirection(quaternion Quat, vec3 Direction);

vec4 CORE_API
TransformDirection(quaternion Quat, vec4 Direction);

vec3 CORE_API
InverseTransformDirection(quaternion Quat, vec3 Direction);

vec4 CORE_API
InverseTransformDirection(quaternion Quat, vec4 Direction);

vec3 CORE_API
TransformDirection(transform Transform, vec3 Vec);

vec3 CORE_API
TransformPosition(transform Transform, vec3 Vec);

vec3 CORE_API
InverseTransformDirection(transform Transform, vec3 Vec);

vec3 CORE_API
InverseTransformPosition(transform Transform, vec3 Vec);

//
// Algorithms: Matrix Transposition and Inversion
//
mat4x4 constexpr
Transposed(mat4x4 const Mat)
{
  return { Mat[0][0], Mat[1][0], Mat[2][0], Mat[3][0],
           Mat[0][1], Mat[1][1], Mat[2][1], Mat[3][1],
           Mat[0][2], Mat[1][2], Mat[2][2], Mat[3][2],
           Mat[0][3], Mat[1][3], Mat[2][3], Mat[3][3] };
}

void CORE_API
Transpose(mat4x4* Mat);

bool CORE_API
IsInvertible(mat4x4 Mat);

mat4x4 CORE_API
Inverted(mat4x4 Mat);

void CORE_API
Invert(mat4x4* Mat);

mat4x4 CORE_API
SafeInverted(mat4x4 Mat);

void CORE_API
SafeInvert(mat4x4* Mat);


//
// Algorithms: Matrix Determinant
//
float CORE_API
Determinant(mat4x4 Mat);


//
// Algorithms: Matrix Accessors
//
vec3 CORE_API ScaledXAxis(mat4x4 Mat);
vec3 CORE_API ScaledYAxis(mat4x4 Mat);
vec3 CORE_API ScaledZAxis(mat4x4 Mat);

vec3 CORE_API UnitXAxis(mat4x4 Mat);
vec3 CORE_API UnitYAxis(mat4x4 Mat);
vec3 CORE_API UnitZAxis(mat4x4 Mat);

//
// Algorithms: Transform Accessors
//

vec3 CORE_API ForwardVector(transform Transform);
vec3 CORE_API RightVector(transform Transform);
vec3 CORE_API UpVector(transform Transform);


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
