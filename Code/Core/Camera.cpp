#include "Camera.hpp"


auto
::CameraProjectionMatrix(common_camera_data const& Cam)
  -> mat4x4
{
  return Mat4x4Perspective(0.5f * Cam.FieldOfView, Cam.Width, Cam.Height, Cam.NearPlane, Cam.FarPlane);
}

auto
::CameraViewMatrix(common_camera_data const& Cam, transform const& WorldTransform)
  -> mat4x4
{
  auto const WorldMatrix = Mat4x4(WorldTransform);
  auto const InvWorldMatrix = SafeInverted(WorldMatrix);
  auto const ViewMatrix = InvWorldMatrix * Mat4x4(UpVector3, ForwardVector3, RightVector3);
  return ViewMatrix;
}

auto
::CameraViewProjectionMatrix(common_camera_data const& Cam, transform const& WorldTransform)
  -> mat4x4
{
  auto const ViewMatrix = CameraViewMatrix(Cam, WorldTransform);
  auto const ProjectionMatrix = CameraProjectionMatrix(Cam);
  return ViewMatrix * ProjectionMatrix;
}
