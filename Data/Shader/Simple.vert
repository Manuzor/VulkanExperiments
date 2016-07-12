#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable

//
// Uniforms
//
layout(binding = 0) uniform Globals
{
  mat4 ModelViewProjection;
};

//
// Input
//
layout(location = 0) in vec4 Position;
layout(location = 1) in vec2 Attribute;

//
// Output
//
layout(location = 0) out vec2 TextureCoordinates;


void main()
{
  TextureCoordinates = Attribute;
  gl_Position = ModelViewProjection * Position;
  // gl_Position = Position;
}
