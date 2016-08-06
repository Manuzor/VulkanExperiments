#pragma once

#if defined(SHADER_COMPILER_EXPORT_DLL)
  #define SHADER_COMPILER_API __declspec(dllexport)
#else
  #define SHADER_COMPILER_API __declspec(dllimport)
#endif

#include <Core/Allocator.hpp>
#include <Core/DynamicArray.hpp>
#include <Core/string.hpp>

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

struct glsl_shader
{
  arc_string EntryPoint{};
  glsl_shader_stage Stage{};
  arc_string Code{};
};

SHADER_COMPILER_API
void
Init(glsl_shader& GlslShader, allocator_interface* Allocator, glsl_shader_stage Stage);

SHADER_COMPILER_API
void
Finalize(glsl_shader& GlslShader);

struct spirv_shader
{
  dynamic_array<uint32> Code{};
};

SHADER_COMPILER_API
void
Init(spirv_shader& SpirvShader, allocator_interface* Allocator);

SHADER_COMPILER_API
void
Finalize(spirv_shader& SpirvShader);


struct shader_compiler_context;

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
                 spirv_shader* SpirvShader);

SHADER_COMPILER_API
bool
CompileCfgToGlslAndSpv(shader_compiler_context* Context, cfg_node const* ShaderRoot,
                       glsl_shader* GlslShader, spirv_shader* SpirvShader);
