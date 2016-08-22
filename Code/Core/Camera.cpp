#include "Camera.hpp"


auto
::CameraProjectionMatrix(common_camera_data const& Cam)
  -> mat4x4
{
  return Mat4x4PerspectiveProjection(Cam.VerticalFieldOfView,
                                     Cam.Width / Cam.Height,
                                     Cam.NearPlane,
                                     Cam.FarPlane);
}

auto
::CameraViewMatrix(common_camera_data const& Cam, transform const& WorldTransform)
  -> mat4x4
{
  mat4x4 const WorldMatrix = Mat4x4(WorldTransform);
  mat4x4 const InvWorldMatrix = SafeInverted(WorldMatrix);
  mat4x4 const ViewMatrix = InvWorldMatrix * Mat4x4(UpVector3, ForwardVector3, -RightVector3);
  return ViewMatrix;
}

auto
::CameraViewProjectionMatrix(common_camera_data const& Cam, transform const& WorldTransform)
  -> mat4x4
{
  mat4x4 const ViewMatrix = CameraViewMatrix(Cam, WorldTransform);
  mat4x4 const ProjectionMatrix = CameraProjectionMatrix(Cam);
  // Note: ViewMatrix goes on the LEFT SIDE of the multiplication.
  return ViewMatrix * ProjectionMatrix;
}
