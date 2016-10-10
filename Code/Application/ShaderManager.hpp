#pragma once

#include <Backbone.hpp>

#include <Core/Allocator.hpp>
#include <Core/Array.hpp>
#include <Core/Dictionary.hpp>


#include <vulkan/vulkan.h>


// Include <ShaderCompiler/ShaderCompiler.hpp> for these types.
struct glsl_shader;
struct spirv_shader;


// These are opaque types for the public.
struct compiled_shader;
struct shader_manager;


enum class shader_stage
{
  Vertex,
  TessellationControl,
  TessellationEvaluation,
  Geometry,
  Fragment,
  Compute,
};


shader_manager*
CreateShaderManager(allocator_interface& Allocator);

void
DestroyShaderManager(allocator_interface& Allocator, shader_manager* Manager);

compiled_shader*
GetCompiledShader(shader_manager& Manager, slice<char const> FileName);

bool
HasShaderStage(compiled_shader& CompiledShader, shader_stage Stage);

glsl_shader*
GetGlslShader(compiled_shader& CompiledShader, shader_stage Stage);

spirv_shader*
GetSpirvShader(compiled_shader& CompiledShader, shader_stage Stage);

void
GenerateVertexInputDescriptions(compiled_shader& CompiledShader,
                                VkVertexInputBindingDescription const& Binding,
                                array<VkVertexInputAttributeDescription>& InputAttributes);

shader_stage
ShaderStageFromName(slice<char const> ShaderStageName);

VkShaderStageFlagBits
ShaderStageToVulkan(shader_stage Stage);

void
GetDescriptorTypeCounts(compiled_shader& CompiledShader,
                        dictionary<VkDescriptorType, uint32>& DescriptorCounts);

void
GetDescriptorSetLayoutBindings(compiled_shader& CompiledShader,
                               array<VkDescriptorSetLayoutBinding>& LayoutBindings);
