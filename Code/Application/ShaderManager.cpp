#include "ShaderManager.hpp"

#include <Core/Log.hpp>

#include <ShaderCompiler/ShaderCompiler.hpp>
#include <Cfg/Cfg.hpp>
#include <Cfg/CfgParser.hpp>

struct compiled_shader
{
  arc_string Id;
  arc_string CfgSource;
  cfg_document Cfg{};
  glsl_shader GlslVertexShader;
  spirv_shader SpirvVertexShader;
  glsl_shader GlslFragmentShader;
  spirv_shader SpirvFragmentShader;
};

static compiled_shader*
CreateCompiledShader(allocator_interface* Allocator)
{
  auto CompiledShader = Allocate<compiled_shader>(Allocator);
  MemDefaultConstruct(1, CompiledShader);
  Init(&CompiledShader->Cfg, Allocator);
  Init(CompiledShader->GlslVertexShader, Allocator, glsl_shader_stage::Vertex);
  Init(CompiledShader->SpirvVertexShader, Allocator);
  Init(CompiledShader->GlslFragmentShader, Allocator, glsl_shader_stage::Fragment);
  Init(CompiledShader->SpirvFragmentShader, Allocator);
  return CompiledShader;
}

static void
DestroyCompiledShader(allocator_interface* Allocator, compiled_shader* CompiledShader)
{
  Finalize(CompiledShader->SpirvFragmentShader);
  Finalize(CompiledShader->GlslFragmentShader);
  Finalize(CompiledShader->SpirvVertexShader);
  Finalize(CompiledShader->GlslVertexShader);
  Finalize(&CompiledShader->Cfg);
  Deallocate(Allocator, CompiledShader);
}

struct shader_manager
{
  allocator_interface* Allocator;
  shader_compiler_context* CompilerContext;
  dynamic_array<compiled_shader*> CompiledShaders;
};

auto
::CreateShaderManager(allocator_interface* Allocator)
  -> shader_manager*
{
  auto Manager = Allocate<shader_manager>(Allocator);
  *Manager = {};
  Manager->Allocator = Allocator;
  Manager->CompilerContext = CreateShaderCompilerContext(Allocator);
  Init(&Manager->CompiledShaders, Allocator);
  return Manager;
}

auto
::DestroyShaderManager(allocator_interface* Allocator, shader_manager* Manager)
  -> void
{
  for(auto CompiledShader : Slice(&Manager->CompiledShaders))
  {
    DestroyCompiledShader(Manager->Allocator, CompiledShader);
  }

  Finalize(&Manager->CompiledShaders);
  DestroyShaderCompilerContext(Allocator, Manager->CompilerContext);
  Deallocate(Allocator, Manager);
}

// TODO: This function is duplicated in a lot of places.
//       Maybe put it in the Core?
#include <cstdio>

static bool
ReadFileContentIntoArray(char const* FileName, dynamic_array<uint8>* Array, log_data* Log)
{
  Clear(Array);

  auto File = std::fopen(FileName, "rb");
  if(File == nullptr)
    return false;

  Defer [File](){ std::fclose(File); };

  size_t const ChunkSize = KiB(4);

  while(true)
  {
    auto NewSlice = ExpandBy(Array, ChunkSize);
    auto const NumBytesRead = std::fread(NewSlice.Ptr, 1, ChunkSize, File);
    auto const Delta = ChunkSize - NumBytesRead;

    if(std::feof(File))
    {
      // Correct the internal array value in case we didn't exactly read a
      // ChunkSize worth of bytes last time.
      Array->Num -= Delta;

      return true;
    }

    if(Delta > 0)
    {
      LogError(Log, "Didn't reach the end of file but failed to read any more bytes: %s", FileName);
      return false;
    }
  }
}

static compiled_shader*
LoadAndCompileShader(shader_manager* ShaderManager, slice<char const> FileName,
                     log_data* Log)
{
  // Make a copy of FileName to ensure it's zero terminated.
  arc_string FileNameString{ FileName };
  auto InputFilePath = AsConst(Slice(FileNameString));

  temp_allocator TempAllocator{};
  allocator_interface* Allocator = *TempAllocator;

  scoped_array<uint8> Content{ Allocator };
  if(!ReadFileContentIntoArray(InputFilePath.Ptr, &Content, Log))
  {
    LogError(Log, "Failed to read file: %s", InputFilePath.Ptr);
    return nullptr;
  }

  auto CompiledShader = CreateCompiledShader(Allocator);
  CompiledShader->Id = InputFilePath;

  // Copy over the source to ensure it lives as long as the document itself.
  CompiledShader->CfgSource = SliceReinterpret<char const>(Slice(&Content));

  {
    // TODO: Supply the GlobalLog here as soon as the cfg parser code is more robust.
    cfg_parsing_context ParsingContext{ InputFilePath, Log };

    if(!CfgDocumentParseFromString(&CompiledShader->Cfg, Slice(CompiledShader->CfgSource), &ParsingContext))
    {
      LogError(Log, "Failed to parse cfg.");
      return nullptr;
    }
  }

  cfg_node* VertexShaderNode = nullptr;
  cfg_node* FragmentShaderNode = nullptr;

  for(auto Node = CompiledShader->Cfg.Root->FirstChild; Node != nullptr; Node = Node->Next)
  {
    if     (Node->Name == "VertexShader"_S)   VertexShaderNode = Node;
    else if(Node->Name == "FragmentShader"_S) FragmentShaderNode = Node;
  }

  //
  // Vertex Shader
  //
  if(VertexShaderNode)
  {
    //
    // Compile to GLSL
    //
    LogBeginScope("Compiling vertex shader from Cfg to GLSL");
    bool const CompiledGLSL = CompileCfgToGlsl(ShaderManager->CompilerContext, VertexShaderNode,
                                               &CompiledShader->GlslVertexShader);
    LogEndScope("Finished compiling vertex shader from Cfg to GLSL");

    //
    // Compile to SPIR-V
    //
    if(CompiledGLSL)
    {
      LogBeginScope("Compiling vertex shader from GLSL to SPIR-V");
      bool const CompiledSpv = CompileGlslToSpv(ShaderManager->CompilerContext, &CompiledShader->GlslVertexShader,
                                                &CompiledShader->SpirvVertexShader);
      LogEndScope("Finished compiling vertex shader from GLSL to SPIR-V");
    }
  }


  //
  // Fragment Shader
  //
  if(FragmentShaderNode)
  {
    //
    // Compile to GLSL
    //
    LogBeginScope("Compiling fragment shader from Cfg to GLSL");
    bool const CompiledGLSL = CompileCfgToGlsl(ShaderManager->CompilerContext, FragmentShaderNode,
                                               &CompiledShader->GlslFragmentShader);
    LogEndScope("Finished compiling fragment shader from Cfg to GLSL");

    //
    // Compile to SPIR-V
    //
    if(CompiledGLSL)
    {
      LogBeginScope("Compiling fragment shader from GLSL to SPIR-V");
      bool const CompiledSpv = CompileGlslToSpv(ShaderManager->CompilerContext, &CompiledShader->GlslFragmentShader,
                                                &CompiledShader->SpirvFragmentShader);
      LogEndScope("Finished compiling fragment shader from GLSL to SPIR-V");
    }
  }

  Expand(&ShaderManager->CompiledShaders) = CompiledShader;

  return CompiledShader;
}

auto
::GetCompiledShader(shader_manager* Manager, slice<char const> FileName,
                    log_data* Log)
  -> compiled_shader*
{
  if(Manager == nullptr)
    return nullptr;

  for(auto CompiledShader : Slice(&Manager->CompiledShaders))
  {
    if(Slice(CompiledShader->Id) == FileName)
    {
      return CompiledShader;
    }
  }

  return LoadAndCompileShader(Manager, FileName, Log);
}

auto
::HasShaderStage(compiled_shader* CompiledShader, shader_stage Stage)
  -> bool
{
  switch(Stage)
  {
    case shader_stage::Vertex:   return CompiledShader->SpirvVertexShader.Code.Num > 0;
    case shader_stage::Fragment: return CompiledShader->SpirvFragmentShader.Code.Num > 0;
    default: return false;
  }
}

auto
::GetGlslShader(compiled_shader* CompiledShader, shader_stage Stage)
  -> glsl_shader*
{
  switch(Stage)
  {
    case shader_stage::Vertex:   return &CompiledShader->GlslVertexShader;
    case shader_stage::Fragment: return &CompiledShader->GlslFragmentShader;
    default:
      break;
  }

  LogError("Unknown shader stage: %u", Stage);
  Assert(0);
  return nullptr;
}

auto
::GetSpirvShader(compiled_shader* CompiledShader, shader_stage Stage)
  -> spirv_shader*
{
  switch(Stage)
  {
    case shader_stage::Vertex:   return &CompiledShader->SpirvVertexShader;
    case shader_stage::Fragment: return &CompiledShader->SpirvFragmentShader;
    default:
      break;
  }

  LogError("Unknown shader stage: %u", Stage);
  Assert(0);
  return nullptr;
}

static void
MapShaderTypeNameToFormatAndSize(slice<char const> TypeName, VkFormat* Format, uint32* Size)
{
  if(TypeName == "float"_S)
  {
    *Format = VK_FORMAT_R32_SFLOAT;
    *Size = Convert<uint32>(sizeof(float));
  }
  else if(TypeName == "vec2"_S)
  {
    *Format = VK_FORMAT_R32G32_SFLOAT;
    *Size = Convert<uint32>(2 * sizeof(float));
  }
  else if(TypeName == "vec3"_S)
  {
    *Format = VK_FORMAT_R32G32B32_SFLOAT;
    *Size = Convert<uint32>(3 * sizeof(float));
  }
  else if(TypeName == "vec4"_S)
  {
    *Format = VK_FORMAT_R32G32B32A32_SFLOAT;
    *Size = Convert<uint32>(4 * sizeof(float));
  }
  else
  {
    *Format = VK_FORMAT_UNDEFINED;
    *Size = IntMinValue<uint32>();
  }
}

auto
::GenerateVertexInputDescriptions(compiled_shader* CompiledShader,
                                  VkVertexInputBindingDescription const& InputBinding,
                                  dynamic_array<VkVertexInputAttributeDescription>* InputAttributes)
  -> void
{
  // Note: this procedure assumes the shader code was compiled successfully
  //       before and is valid.

  if(CompiledShader == nullptr)
  {
    LogError("Invalid CompiledShader ptr.");
    return;
  }

  cfg_node* VertexShaderNode{};
  for(auto Node = CompiledShader->Cfg.Root->FirstChild;
      Node != nullptr;
      Node = Node->Next)
  {
    if(Node->Name == "VertexShader"_S)
    {
      VertexShaderNode = Node;
      break;
    }
  }

  if(VertexShaderNode == nullptr)
  {
    LogError("No VertexShader node in Cfg document.");
    return;
  }

  cfg_node* InputNode{};
  for(auto Node = VertexShaderNode->FirstChild;
      Node != nullptr;
      Node = Node->Next)
  {
    if(Node->Name == "Input"_S)
    {
      InputNode = Node;
      break;
    }
  }

  if(InputNode == nullptr)
  {
    LogError("No Input node below VertexShader node.");
    return;
  }

  struct input_decl
  {
    slice<char const> TypeName;
    slice<char const> Identifier;
    int Location;
  };

  temp_allocator TempAllocator{};
  allocator_interface* Allocator = *TempAllocator;
  scoped_array<input_decl> Decls{ Allocator };

  for(auto Node = InputNode->FirstChild;
      Node != nullptr;
      Node = Node->Next)
  {
    auto& Decl = Expand(&Decls);
    Decl.TypeName = Node->Name.Value;
    Decl.Identifier = Convert<slice<char const>>(Node->Values[0]);
    Decl.Location = -1;
    for(auto& Attr : Slice(&Node->Attributes))
    {
      if(Attr.Name == "Location"_S)
      {
        Decl.Location = Convert<int>(Attr.Value);
      }
    }
    Assert(Decl.Location != -1);
  }

  // TODO: Sort by Location!

  uint32 CurrentOffset = 0;

  for(auto& Decl : Slice(&Decls))
  {
    auto& Desc = Expand(InputAttributes);
    Desc = InitStruct<VkVertexInputAttributeDescription>();
    Desc.binding = InputBinding.binding;
    Desc.location = Decl.Location;
    Desc.offset = CurrentOffset;
    uint32 Size{};
    MapShaderTypeNameToFormatAndSize(Decl.TypeName, &Desc.format, &Size);
    CurrentOffset += Convert<uint32>(Size);
  }

  Assert(InputBinding.stride >= CurrentOffset);
}

auto
::ShaderStageFromName(slice<char const> ShaderStageName)
  -> shader_stage
{
  if(SliceStartsWith(ShaderStageName, "Vertex"_S))   return shader_stage::Vertex;
  if(SliceStartsWith(ShaderStageName, "Fragment"_S)) return shader_stage::Fragment;

  // Invalid value.
  return Coerce<shader_stage>(-1);
}

auto
::ShaderStageToVulkan(shader_stage Stage)
  -> VkShaderStageFlagBits
{
  switch(Stage)
  {
    case shader_stage::Vertex:                 return VK_SHADER_STAGE_VERTEX_BIT;
    case shader_stage::TessellationControl:    return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    case shader_stage::TessellationEvaluation: return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    case shader_stage::Geometry:               return VK_SHADER_STAGE_GEOMETRY_BIT;
    case shader_stage::Fragment:               return VK_SHADER_STAGE_FRAGMENT_BIT;
    case shader_stage::Compute:                return VK_SHADER_STAGE_COMPUTE_BIT;

    default:
      break;
  };

  LogError("Unknown shader stage: %u", Stage);
  Assert(0);
  return VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
}

static VkDescriptorType
GetDescriptorTypeFromName(slice<char const> DescriptorName)
{
  if(DescriptorName == "sampler2D"_S)    return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
  else if(DescriptorName == "buffer"_S)  return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
  else if(DescriptorName == "uniform"_S) return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

  return VK_DESCRIPTOR_TYPE_MAX_ENUM;
}

auto
::GetDescriptorTypeCounts(compiled_shader* CompiledShader,
                          dictionary<VkDescriptorType, uint32>* DescriptorCounts)
  -> void
{
  if(CompiledShader == nullptr)
  {
    LogError("Invalid CompiledShader ptr.");
    return;
  }

  for(auto Node = CompiledShader->Cfg.Root->FirstChild;
      Node != nullptr;
      Node = Node->Next)
  {
    for(auto Descriptor = Node->FirstChild;
        Descriptor != nullptr;
        Descriptor = Descriptor->Next)
    {
      auto DescriptorType = GetDescriptorTypeFromName(Descriptor->Name.Value);
      if(DescriptorType != VK_DESCRIPTOR_TYPE_MAX_ENUM)
      {
        ++*GetOrCreate(DescriptorCounts, DescriptorType);
      }
      else
      {
        arc_string DescriptorName{ Descriptor->Name.Value };
        LogWarning("Unknown descriptor name: %s", StrPtr(DescriptorName));
      }
    }
  }
}

auto
::GetDescriptorSetLayoutBindings(compiled_shader* CompiledShader,
                                 dynamic_array<VkDescriptorSetLayoutBinding>* LayoutBindings)
  -> void
{
  if(CompiledShader == nullptr)
  {
    LogError("Invalid CompiledShader ptr.");
    return;
  }

  temp_allocator TempAllocator{};
  allocator_interface* Allocator = *TempAllocator;

  scoped_dictionary<uint32, VkDescriptorSetLayoutBinding> BindingSet{ Allocator };

  for(auto ShaderNode = CompiledShader->Cfg.Root->FirstChild;
      ShaderNode != nullptr;
      ShaderNode = ShaderNode->Next)
  {
    auto ShaderStage = ShaderStageFromName(ShaderNode->Name.Value);
    auto VulkanStageBit = ShaderStageToVulkan(ShaderStage);
    Assert(VulkanStageBit != VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM);

    for(auto Node = ShaderNode->FirstChild;
        Node != nullptr;
        Node = Node->Next)
    {
      if(Node->Values.Num == 0)
      {
        // Just ignore those nodes that don't have a value.
        continue;
      }

      for(auto Attr : Slice(&Node->Attributes))
      {
        if(Attr.Name == "Binding"_S)
        {
          auto BindingValue = Convert<uint32>(Attr.Value);
          auto Binding = Get(&BindingSet, BindingValue);
          if(Binding == nullptr)
          {
            Binding = GetOrCreate(&BindingSet, BindingValue);
            *Binding = InitStruct<VkDescriptorSetLayoutBinding>();
            Binding->binding = BindingValue;
            Binding->descriptorType = GetDescriptorTypeFromName(Node->Name.Value);
            Assert(Binding->descriptorType != VK_DESCRIPTOR_TYPE_MAX_ENUM);
          }

          ++Binding->descriptorCount;
          Binding->stageFlags |= VulkanStageBit;
          break;
        }
      }
    }
  }

  Append(LayoutBindings, AsConst(Values(&BindingSet)));
}
