#include "ShaderManager.hpp"

#include <ShaderCompiler/ShaderCompiler.hpp>
#include <Cfg/Cfg.hpp>
#include <Cfg/CfgParser.hpp>

struct compiled_shader
{
  arc_string Id;
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
#include <stdio.h>

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

  {
    // TODO: Supply the GlobalLog here as soon as the cfg parser code is more robust.
    cfg_parsing_context ParsingContext{ InputFilePath, Log };

    if(!CfgDocumentParseFromString(&CompiledShader->Cfg, SliceReinterpret<char const>(Slice(&Content)), &ParsingContext))
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
::GetGlslVertexShader(compiled_shader* CompiledShader)
  -> glsl_shader*
{
  return &CompiledShader->GlslVertexShader;
}

auto
::GetGlslFragmentShader(compiled_shader* CompiledShader)
  -> glsl_shader*
{
  return &CompiledShader->GlslFragmentShader;
}

auto
::GetSpirvVertexShader(compiled_shader* CompiledShader)
  -> spirv_shader*
{
  return &CompiledShader->SpirvVertexShader;
}

auto
::GetSpirvFragmentShader(compiled_shader* CompiledShader)
  -> spirv_shader*
{
  return &CompiledShader->SpirvFragmentShader;
}
