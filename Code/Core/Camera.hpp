#pragma once

#include <Core/Math.hpp>
#include <Backbone.hpp>


struct common_camera_data
{
  transform Transform;

  angle FieldOfView;
  float Width;
  float Height;
  float NearPlane;
  float FarPlane;
};

struct free_horizon_camera : public common_camera_data
{
  float MovementSpeed; // Units per second.
  float RotationSpeed; // Units per second.

  float InputYaw;
  float InputPitch;
};

CORE_API mat4x4
CameraProjectionMatrix(common_camera_data const& Cam);

CORE_API mat4x4
CameraViewMatrix(common_camera_data const& Cam, transform const& WorldTransform);

CORE_API mat4x4
CameraViewProjectionMatrix(common_camera_data const& Cam, transform const& WorldTransform);
