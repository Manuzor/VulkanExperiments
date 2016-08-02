#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable


//
// Uniforms
//
layout(binding = 0) buffer Globals
{
  mat4 ViewProjectionMatrix;
};


//
// Input
//
layout(location = 0) in vec3 Position;
layout(location = 1) in vec4 Color;


//
// Output
//
layout(location = 0) out vec4 OutColor;


void main()
{
  gl_Position = ViewProjectionMatrix * vec4(Position, 1.0f);
  OutColor = Color;
}
