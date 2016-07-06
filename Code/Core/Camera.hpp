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

mat4x4 CORE_API
CameraProjectionMatrix(common_camera_data const& Cam);

mat4x4 CORE_API
CameraViewMatrix(common_camera_data const& Cam, transform const& WorldTransform);

mat4x4 CORE_API
CameraViewProjectionMatrix(common_camera_data const& Cam, transform const& WorldTransform);
