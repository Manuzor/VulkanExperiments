#pragma once

#include <Backbone.hpp>

#include <Core/Allocator.hpp>
#include <Core/DynamicArray.hpp>
#include <Core/String.hpp>
#include <Core/Log.hpp>


// Include <ShaderCompiler/ShaderCompiler.hpp> for these types.
struct glsl_shader;
struct spirv_shader;


// These are opaque types for the public.
struct compiled_shader;
struct shader_manager;


shader_manager*
CreateShaderManager(allocator_interface* Allocator);

void
DestroyShaderManager(allocator_interface* Allocator, shader_manager* Manager);

compiled_shader*
GetCompiledShader(shader_manager* Manager, slice<char const> FileName,
                  log_data* Log = nullptr);

glsl_shader*
GetGlslVertexShader(compiled_shader* CompiledShader);

glsl_shader*
GetGlslFragmentShader(compiled_shader* CompiledShader);

spirv_shader*
GetSpirvVertexShader(compiled_shader* CompiledShader);

spirv_shader*
GetSpirvFragmentShader(compiled_shader* CompiledShader);
