#version 450

#extension GL_ARB_separate_shader_objects : enable
#extension GL_ARB_shading_language_420pack : enable


//
// Uniforms
//
layout(binding = 1) uniform sampler2D Sampler; // Shhh, it'll be over soon.


//
// Input
//
layout(location = 0) in vec4 Color;


//
// Output
//
layout(location = 0) out vec4 FragmentColor;


void main()
{
  FragmentColor = Color;
}
