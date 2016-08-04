#pragma once

#if defined(SHADER_COMPILER_EXPORT_DLL)
  #define SHADER_COMPILER_API __declspec(dllexport)
#else
  #define SHADER_COMPILER_API __declspec(dllimport)
#endif

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

class SHADER_COMPILER_API glsl_shader
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

SHADER_COMPILER_API
shader_compiler_context*
CreateShaderCompilerContext(allocator_interface* Allocator);

SHADER_COMPILER_API
void
DestroyShaderCompilerContext(allocator_interface* Allocator, shader_compiler_context* Context);

SHADER_COMPILER_API
bool
CompileCfgToGlsl(shader_compiler_context* Context, cfg_node const* ShaderRoot,
                 glsl_shader* GlslShader);

SHADER_COMPILER_API
bool
CompileGlslToSpv(shader_compiler_context* Context, glsl_shader const* GlslShader,
                 dynamic_array<uint32>* SpvByteCode);

SHADER_COMPILER_API
bool
CompileCfgToGlslAndSpv(shader_compiler_context* Context, cfg_node const* ShaderRoot,
                       glsl_shader* GlslShader, dynamic_array<uint32>* SpvByteCode);
