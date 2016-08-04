#pragma once

#include <Core/Allocator.hpp>

#include <Cfg/Cfg.hpp>


enum class glsl_shader_stage
{
  Vertex,
  TesselationControl,
  TesselationEvaluation,
  Geometry,
  Fragment,
  Compute,
};

class glsl_shader
{
public:
  dynamic_array<char> EntryPoint{};
  glsl_shader_stage Stage{};
  dynamic_array<char> Code{};

  glsl_shader(allocator_interface* Allocator);
  ~glsl_shader();
};

struct shader_compiler_context
{
};

shader_compiler_context*
CreateShaderCompilerContext(allocator_interface* Allocator);

void
DestroyShaderCompilerContext(allocator_interface* Allocator, shader_compiler_context* Context);

bool
CompileCfgToGlsl(shader_compiler_context* Context, cfg_node const* ShaderRoot,
                 glsl_shader* GlslShader);

bool
CompileGlslToSpv(shader_compiler_context* Context, glsl_shader const* GlslShader,
                 dynamic_array<uint32>* SpvByteCode);

bool
CompileCfgToGlslAndSpv(shader_compiler_context* Context, cfg_node const* ShaderRoot,
                       glsl_shader* GlslShader, dynamic_array<uint32>* SpvByteCode);
