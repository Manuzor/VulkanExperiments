#include "TestHeader.hpp"
#include <Core/Math.hpp>

TEST_CASE("Math: Vector Basics", "[Math]")
{
  SECTION("Vec2 Construction")
  {
    REQUIRE( Vec2(0, 1).X == 0 );
    REQUIRE( Vec2(0, 1).Y == 1 );
    REQUIRE( Vec2(0, 1).Data[0] == 0 );
    REQUIRE( Vec2(0, 1).Data[1] == 1 );
  }
  SECTION("Vec3 Construction")
  {
    REQUIRE( Vec3(0, 1, 2).X == 0 );
    REQUIRE( Vec3(0, 1, 2).Y == 1 );
    REQUIRE( Vec3(0, 1, 2).Z == 2 );
    REQUIRE( Vec3(0, 1, 2).Data[0] == 0 );
    REQUIRE( Vec3(0, 1, 2).Data[1] == 1 );
    REQUIRE( Vec3(0, 1, 2).Data[2] == 2 );
  }
  SECTION("Vec4 Construction")
  {
    REQUIRE( Vec4(0, 1, 2, 3).X == 0 );
    REQUIRE( Vec4(0, 1, 2, 3).Y == 1 );
    REQUIRE( Vec4(0, 1, 2, 3).Z == 2 );
    REQUIRE( Vec4(0, 1, 2, 3).W == 3 );
    REQUIRE( Vec4(0, 1, 2, 3).Data[0] == 0 );
    REQUIRE( Vec4(0, 1, 2, 3).Data[1] == 1 );
    REQUIRE( Vec4(0, 1, 2, 3).Data[2] == 2 );
    REQUIRE( Vec4(0, 1, 2, 3).Data[3] == 3 );
  }
}

TEST_CASE("Math: Vector Length", "[Math]")
{
  vec2  const V2    = Vec2(1, 2);
  vec2  const V2Dir = Vec2(0.447213595f, 0.894427191f);
  float const V2Len = 2.236067977f;

  vec3  const V3    = Vec3(1, 2, 3);
  vec3  const V3Dir = Vec3(0.267261242f, 0.534522484f, 0.801783726f);
  float const V3Len = 3.741657387f;

  vec4  const V4    = Vec4(1, 2, 3, 4);
  vec4  const V4Dir = Vec4(0.182574186f, 0.365148372f, 0.547722558f, 0.730296743f);
  float const V4Len = 5.477225575f;

  SECTION("LengthSquared")
  {
    REQUIRE( AreNearlyEqual(LengthSquared(V2), V2Len * V2Len, 1e-5f) );
    REQUIRE( AreNearlyEqual(LengthSquared(V3), V3Len * V3Len, 1e-5f) );
    REQUIRE( AreNearlyEqual(LengthSquared(V4), V4Len * V4Len, 1e-5f) );
  }

  SECTION("Length")
  {
    REQUIRE( AreNearlyEqual(Length(V2), V2Len) );
    REQUIRE( AreNearlyEqual(Length(V3), V3Len) );
    REQUIRE( AreNearlyEqual(Length(V4), V4Len) );
  }

  SECTION("DirectionAndLength")
  {
    vec2 Dir2;
    float Len2;
    DirectionAndLength(V2, &Dir2, &Len2);
    REQUIRE( AreNearlyEqual(Dir2, V2Dir) );
    REQUIRE( AreNearlyEqual(Len2, V2Len) );

    vec3 Dir3;
    float Len3;
    DirectionAndLength(V3, &Dir3, &Len3);
    REQUIRE( AreNearlyEqual(Dir3, V3Dir) );
    REQUIRE( AreNearlyEqual(Len3, V3Len) );

    vec4 Dir4;
    float Len4;
    DirectionAndLength(V4, &Dir4, &Len4);
    REQUIRE( AreNearlyEqual(Dir4, V4Dir) );
    REQUIRE( AreNearlyEqual(Len4, V4Len) );
  }
}

TEST_CASE("Math: Matrix Basics", "[Math]")
{
  auto Mat = Mat4x4( 1,  2,  3,  4,
                     5,  6,  7,  8,
                     9, 10, 11, 12,
                    13, 14, 15, 16);

  SECTION("Direct Element Access")
  {
    REQUIRE( Mat.M00 ==  1 );
    REQUIRE( Mat.M01 ==  2 );
    REQUIRE( Mat.M02 ==  3 );
    REQUIRE( Mat.M03 ==  4 );
    REQUIRE( Mat.M10 ==  5 );
    REQUIRE( Mat.M11 ==  6 );
    REQUIRE( Mat.M12 ==  7 );
    REQUIRE( Mat.M13 ==  8 );
    REQUIRE( Mat.M20 ==  9 );
    REQUIRE( Mat.M21 == 10 );
    REQUIRE( Mat.M22 == 11 );
    REQUIRE( Mat.M23 == 12 );
    REQUIRE( Mat.M30 == 13 );
    REQUIRE( Mat.M31 == 14 );
    REQUIRE( Mat.M32 == 15 );
    REQUIRE( Mat.M33 == 16 );
  }

  SECTION("Subscript Operator")
  {
    REQUIRE( Mat[0][0] ==  1 );
    REQUIRE( Mat[0][1] ==  2 );
    REQUIRE( Mat[0][2] ==  3 );
    REQUIRE( Mat[0][3] ==  4 );
    REQUIRE( Mat[1][0] ==  5 );
    REQUIRE( Mat[1][1] ==  6 );
    REQUIRE( Mat[1][2] ==  7 );
    REQUIRE( Mat[1][3] ==  8 );
    REQUIRE( Mat[2][0] ==  9 );
    REQUIRE( Mat[2][1] == 10 );
    REQUIRE( Mat[2][2] == 11 );
    REQUIRE( Mat[2][3] == 12 );
    REQUIRE( Mat[3][0] == 13 );
    REQUIRE( Mat[3][1] == 14 );
    REQUIRE( Mat[3][2] == 15 );
    REQUIRE( Mat[3][3] == 16 );
  }

  SECTION("ColN Access")
  {
    REQUIRE( AreNearlyEqual(Mat.Col0, Vec4( 1,  2,  3,  4), 0.0f) );
    REQUIRE( AreNearlyEqual(Mat.Col1, Vec4( 5,  6,  7,  8), 0.0f) );
    REQUIRE( AreNearlyEqual(Mat.Col2, Vec4( 9, 10, 11, 12), 0.0f) );
    REQUIRE( AreNearlyEqual(Mat.Col3, Vec4(13, 14, 15, 16), 0.0f) );
  }
}

TEST_CASE("Math: Matrix Multiplication", "[Math]")
{
  SECTION("Identity * Identity")
  {
    REQUIRE( IdentityMatrix4x4 * IdentityMatrix4x4 == IdentityMatrix4x4 );
  }

  auto Mat = Mat4x4(
     1,  2,  3,  4,
     5,  6,  7,  8,
     9, 10, 11, 12,
    13, 14, 15, 16);

  SECTION("Identity * Arbitrary")
  {
    REQUIRE( Mat * IdentityMatrix4x4 == Mat );
  }

  SECTION("Arbitrary * Arbitrary")
  {
    auto Mat2 = Mat4x4(
      16, 15, 14, 13,
      12, 11, 10,  9,
       8,  7,  6,  5,
       4,  3,  2,  1
      );

    auto Expected = Mat4x4(
      80,   70,  60,  50,
      240, 214, 188, 162,
      400, 358, 316, 274,
      560, 502, 444, 386);

    REQUIRE( Mat * Mat2 == Expected );
  }
}

TEST_CASE("Math: Matrix Transposition and Inversion", "[Math]")
{

  SECTION("Determinant")
  {
    SECTION("Identity")
    {
      REQUIRE(Determinant(IdentityMatrix4x4) == 1);
      REQUIRE(IsInvertible(IdentityMatrix4x4));
    }

    SECTION("Arbitrary")
    {
      auto Mat = Mat4x4(
         1,  2,  3,  4,
         5,  6,  7,  8,
         9, 10, 11, 12,
        13, 14, 15, 16);

      REQUIRE(Determinant(Mat) == 0);
      REQUIRE(!IsInvertible(Mat));

      Mat = Mat4x4(ForwardVector3, RightVector3, UpVector3, Vec3(10, 20, 50));

      REQUIRE(Determinant(Mat) == 1);
      REQUIRE(IsInvertible(Mat));
    }
  }

  SECTION("Transposing the identity matrix")
  {
    REQUIRE( Transposed(IdentityMatrix4x4) == IdentityMatrix4x4 );
  }

  SECTION("Transposing an arbitrary matrix")
  {
    auto Mat = Mat4x4(
       1,  2,  3,  4,
       5,  6,  7,  8,
       9, 10, 11, 12,
      13, 14, 15, 16);

    auto Expected = Mat4x4(
       1, 5,  9, 13,
       2, 6, 10, 14,
       3, 7, 11, 15,
       4, 8, 12, 16);
    REQUIRE( Expected == Transposed(Mat) );
  }

  SECTION("Invert the identity matrix")
  {
    REQUIRE( Inverted(IdentityMatrix4x4) == IdentityMatrix4x4 );
  }

  SECTION("Invert an arbitrary matrix")
  {
    auto Mat = Mat4x4(ForwardVector3, RightVector3, UpVector3, Vec3(10, 20, 50));

    REQUIRE( IsInvertible(Mat) );
    REQUIRE( Mat * SafeInverted(Mat) == IdentityMatrix4x4 );
  }
}

TEST_CASE("Math: Matrix Axis Access", "[Math]")
{
  SECTION("Scaled")
  {
    auto Mat = Mat4x4(
       1,  2,  3,  4,
       5,  6,  7,  8,
       9, 10, 11, 12,
      13, 14, 15, 16);

    REQUIRE( ScaledXAxis(Mat) == Vec3(1,  2,  3) );
    REQUIRE( ScaledYAxis(Mat) == Vec3(5,  6,  7) );
    REQUIRE( ScaledZAxis(Mat) == Vec3(9, 10, 11) );
  }

  SECTION("Unit")
  {
    auto Mat = Mat4x4(
       5,  0,  0,  4,
       0,  6,  0,  8,
       0,  0, 11, 12,
      13, 14, 15, 16);

    REQUIRE( UnitXAxis(Mat) == Vec3(1, 0, 0) );
    REQUIRE( UnitYAxis(Mat) == Vec3(0, 1, 0) );
    REQUIRE( UnitZAxis(Mat) == Vec3(0, 0, 1) );
  }
}

TEST_CASE("Math: Matrix Construction from Axes", "[Math]")
{
  auto Mat = Mat4x4(Vec3(1, 0, 0), Vec3(0, 1, 0), Vec3(0, 0, 1), Vec3(10, 50, 20));
  auto Expected = Mat4x4( 1,  0,  0, 0,
                          0,  1,  0, 0,
                          0,  0,  1, 0,
                         10, 50, 20, 1);

  REQUIRE( Mat == Expected );
}

TEST_CASE("Math: Matrix Transforming A Vector", "[Math]")
{
  // Rotate 90 Degrees CCW around the Z Axis
  auto Mat = Mat4x4(-RightVector3, ForwardVector3, UpVector3, Vec3(0, 0, 0));
  auto Pos = Vec3(1, 0, 0);
  auto ExpectedPos = Vec3(0,-1, 0);

  auto Transformed = TransformPosition(Mat, Pos);
  REQUIRE( ExpectedPos == Transformed );
  Transformed = InverseTransformPosition(Mat, Transformed);
  REQUIRE( Pos == Transformed );

  Transformed = TransformDirection(Mat, ForwardVector3);
  REQUIRE( Transformed == -RightVector3 );
  Transformed = InverseTransformDirection(Mat, Transformed);
  REQUIRE( Transformed == ForwardVector3 );

  Mat = Mat4x4(ForwardVector3, RightVector3, UpVector3, Vec3(10, 20, 50));
  Pos = Vec3(1, 0, 0);
  ExpectedPos = Vec3(11, 20, 50);

  Transformed = TransformPosition(Mat, Pos);
  REQUIRE( ExpectedPos == Transformed );
  Transformed = InverseTransformPosition(Mat, Transformed);
  REQUIRE( Pos == Transformed );

  Transformed = TransformDirection(Mat, ForwardVector3);
  REQUIRE( Transformed == ForwardVector3 );
  Transformed = InverseTransformDirection(Mat, ForwardVector3);
  REQUIRE( Transformed == ForwardVector3 );

  REQUIRE( TransformDirection(Mat, Vec4(0, 0, 0, 2)) == Vec4(20, 40, 100, 2) );
  REQUIRE( InverseTransformDirection(Mat, TransformDirection(Mat, Vec4(0, 0, 0, 2))) == Vec4(0, 0, 0, 2) );
}

TEST_CASE("Math: Special Matrices", "[Math]")
{
  SECTION("LookAt")
  {
    auto Mat = Mat4x4LookAt(ZeroVector3, ForwardVector3, UpVector3);

    auto Result = TransformDirection(Mat, ForwardVector3);
    REQUIRE( Result == -ForwardVector3 );

    Result = TransformPosition(Mat, ForwardVector3);
    REQUIRE( Result == ZeroVector3 );
  }
}

TEST_CASE("Math: Matrix <=> Quaternion", "[Math]")
{
  auto RotationMatrix = Mat4x4(IdentityQuaternion);
  REQUIRE( AreNearlyEqual(RotationMatrix, IdentityMatrix4x4) );

  auto Quat = Quaternion(RotationMatrix);
  REQUIRE( AreNearlyEqual(Quat, IdentityQuaternion) );
}
