#include <Backbone.hpp>
#include <Core/Array.hpp>
#include <Core/Log.hpp>

#include <Cfg/Cfg.hpp>
#include <Cfg/CfgParser.hpp>

#include "ShaderCompiler.hpp"

#include <stdio.h>


static bool
ReadFileContentIntoArray(arc_string FileName, array<uint8>& Array)
{
  Clear(Array);

  auto File = std::fopen(StrPtr(FileName), "rb");
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
      Array.Num -= Delta;

      return true;
    }

    if(Delta > 0)
    {
      LogError("Didn't reach the end of file but failed to read any more bytes: %s", StrPtr(FileName));
      return false;
    }
  }
}

template<typename T>
bool
WriteArrayContentToFile(slice<T> Content, arc_string FileName, size_t* NumBytesWritten = nullptr)
{
  auto File = std::fopen(StrPtr(FileName), "wb");
  if(File == nullptr)
    return false;

  Defer [File](){ std::fclose(File); };

  auto LocalNumBytesWritten = std::fwrite(Content.Ptr, SizeOf<T>(), Content.Num, File);
  if(NumBytesWritten)
    *NumBytesWritten = LocalNumBytesWritten;
  return true;
}

struct shader_output_path
{
  slice<char const> Glsl;
  slice<char const> Spirv;
};

struct cmd_options
{
  slice<char const> InputFilePath;
  shader_output_path VertexShaderOutFilePath;
  shader_output_path FragmentShaderOutFilePath;
};

void
LogUsage(log_data* Log)
{
  LogInfo("Usage: (<Argument>|<Option)...");
  LogInfo("");
  LogBeginScope("Arguments");
    LogInfo("InputFilePath    The input config file to compile.");
  LogEndScope("");
  LogBeginScope("Options");
    LogInfo("-glsl-vert <FilePath>     Vertex shader output file as GLSL code.");
    LogInfo("-spirv-vert <FilePath>    Vertex shader output file as SPIR-V bytecode.");
    LogInfo("-glsl-frag <FilePath>     Fragment shader output file as GLSL code.");
    LogInfo("-spirv-frag <FilePath>    Fragment shader output file as SPIR-V bytecode.");
    LogInfo("-help                     Show this hint text and terminate.");
  LogEndScope("");
  LogBeginScope("Notes");
    LogInfo("Additional prefixed '-' characters are ignored. "
            "This means that -help is equivalent to --help.");
  LogEndScope("");
}

bool
ParseCommandLineOptions(slice<char const*> Args, cmd_options* Options)
{
  for(size_t Index = 0; Index < Args.Num; ++Index)
  {
    auto Arg = SliceFromString(Args[Index]);
    if(SliceStartsWith(Arg, "-"_S))
    {
      do
      {
        Arg = SliceTrimFront(Arg, 1);
      } while(SliceStartsWith(Arg, "-"_S));

      if(Arg == "help"_S || Arg == "h"_S)
      {
        LogUsage(GlobalLog);
        return false;
      }
      else if(Arg == "glsl-vert"_S)
      {
        ++Index;
        if(Index >= Args.Num)
        {
          LogError("Missing argument for -glsl-vert.");
          LogUsage(GlobalLog);
          return false;
        }

        Options->VertexShaderOutFilePath.Glsl = SliceFromString(Args[Index]);
      }
      else if(Arg == "spirv-vert"_S)
      {
        ++Index;
        if(Index >= Args.Num)
        {
          LogError("Missing argument for -spirv-vert.");
          LogUsage(GlobalLog);
          return false;
        }

        Options->VertexShaderOutFilePath.Spirv = SliceFromString(Args[Index]);
      }
      else if(Arg == "glsl-frag"_S)
      {
        ++Index;
        if(Index >= Args.Num)
        {
          LogError("Missing argument for -glsl-frag.");
          LogUsage(GlobalLog);
          return false;
        }

        Options->FragmentShaderOutFilePath.Glsl = SliceFromString(Args[Index]);
      }
      else if(Arg == "spirv-frag"_S)
      {
        ++Index;
        if(Index >= Args.Num)
        {
          LogError("Missing argument for -spirv-frag.");
          LogUsage(GlobalLog);
          return false;
        }

        Options->FragmentShaderOutFilePath.Spirv = SliceFromString(Args[Index]);
      }
      else
      {
        LogError("Unknown argument: %s", Args[Index]);
        LogUsage(GlobalLog);
        return false;
      }
    }
    else
    {
      if(!Options->InputFilePath)
      {
        Options->InputFilePath = Arg;
      }
      else
      {
        LogError("Expected only 1 positional argument (InputFilePath).");
        LogUsage(GlobalLog);
        return false;
      }
    }
  }

  if(!Options->InputFilePath)
  {
    LogError("Missing required positional argument (InputFilePath).");
    LogUsage(GlobalLog);
    return false;
  }

  return true;
}


int
main(int NumArgs, char const* Args[])
{
  mallocator Mallocator{};
  allocator_interface& Allocator = Mallocator;

  //
  // Set up a logger
  //
  log_data Log{};
  {
    auto DefaultSinkSlots = ExpandBy(Log.Sinks, 2);
    DefaultSinkSlots[0] = GetStdoutLogSink(stdout_log_sink_enable_prefixes::No);
    DefaultSinkSlots[1] = log_sink(VisualStudioLogSink);
  }

  //
  // Set the global log
  //
  GlobalLog = &Log;
  Defer [=](){ GlobalLog = nullptr; };

  cmd_options Options{};
  if(!ParseCommandLineOptions(Slice(NumArgs - 1, &Args[1]), &Options))
  {
    return -1;
  }

  auto const InputFilePath = Options.InputFilePath;

  array<uint8> Content{ Allocator };
  if(!ReadFileContentIntoArray(InputFilePath, Content))
  {
    LogError("Failed to read file: %*s", Convert<int>(InputFilePath.Num), InputFilePath.Ptr);
    LogUsage(GlobalLog);
    return 2;
  }

  cfg_document Document{};
  Init(Document, Allocator);
  Defer [&](){ Finalize(Document); };

  {
    // TODO: Supply the GlobalLog here as soon as the cfg parser code is more
    // robust and doesn't produce as many warnings.
    cfg_parsing_context Context{ InputFilePath, nullptr };

    if(!CfgDocumentParseFromString(Document, SliceReinterpret<char const>(Slice(Content)), &Context))
    {
      LogError("Failed to parse cfg.");
      return 3;
    }
  }

  cfg_node* VertexShaderNode = nullptr;
  cfg_node* FragmentShaderNode = nullptr;

  for(auto Node = Document.Root->FirstChild; Node; Node = Node->Next)
  {
    if     (Node->Name == "VertexShader"_S)   VertexShaderNode = Node;
    else if(Node->Name == "FragmentShader"_S) FragmentShaderNode = Node;
  }

  auto Context = CreateShaderCompilerContext(Allocator);
  Defer [&](){ DestroyShaderCompilerContext(Allocator, Context); };


  //
  // Vertex Shader
  //
  glsl_shader GlslVertexShader{};
  Init(GlslVertexShader, Allocator, glsl_shader_stage::Vertex);
  Defer [&](){ Finalize(GlslVertexShader); };

  spirv_shader SpirvVertexShader{};
  Init(SpirvVertexShader, Allocator);
  Defer [&](){ Finalize(SpirvVertexShader); };

  if(VertexShaderNode)
  {
    //
    // Compile to GLSL
    //
    LogBeginScope("Compiling vertex shader from Cfg to GLSL");
    bool const CompiledGLSL = CompileCfgToGlsl(*Context, *VertexShaderNode, GlslVertexShader);
    LogEndScope("Finished compiling vertex shader from Cfg to GLSL");

    //
    // Write GLSL file
    //
    if(CompiledGLSL && Options.VertexShaderOutFilePath.Glsl)
    {
      auto const FileName = Options.VertexShaderOutFilePath.Glsl;
      if(FileName == "-"_S)
      {
        printf("%s", StrPtr(GlslVertexShader.Code));
      }
      else if(!WriteArrayContentToFile(Slice(GlslVertexShader.Code), FileName))
      {
        LogWarning("%*s: Failed to write glsl vertex shader file.", Convert<int>(FileName.Num), FileName.Ptr);
      }
    }

    //
    // Compile to SPIR-V
    //
    if(CompiledGLSL)
    {
      LogBeginScope("Compiling vertex shader from GLSL to SPIR-V");
      bool const CompiledSpv = CompileGlslToSpv(*Context, GlslVertexShader, SpirvVertexShader);
      LogEndScope("Finished compiling vertex shader from GLSL to SPIR-V");

      //
      // Write SPIR-V file
      //
      if(CompiledSpv && Options.VertexShaderOutFilePath.Spirv)
      {
        auto const FileName = Options.VertexShaderOutFilePath.Spirv;
        if(FileName == "-"_S)
        {
          printf("%*s", Convert<int>(SpirvVertexShader.Code.Num * 4), Reinterpret<char const*>(SpirvVertexShader.Code.Ptr));
        }
        else if(!WriteArrayContentToFile(Slice(SpirvVertexShader.Code), FileName))
        {
          LogWarning("%*s: Failed to write spv vertex shader file.", Convert<int>(FileName.Num), FileName.Ptr);
        }
      }
    }
  }


  //
  // Fragment Shader
  //
  glsl_shader GlslFragmentShader{};
  Init(GlslFragmentShader, Allocator, glsl_shader_stage::Fragment);
  Defer [&](){ Finalize(GlslFragmentShader); };

  spirv_shader SpirvFragmentShader{};
  Init(SpirvFragmentShader, Allocator);
  Defer [&](){ Finalize(SpirvFragmentShader); };

  if(FragmentShaderNode)
  {
    //
    // Compile to GLSL
    //
    LogBeginScope("Compiling fragment shader from Cfg to GLSL");
    bool const CompiledGLSL = CompileCfgToGlsl(*Context, *FragmentShaderNode, GlslFragmentShader);
    LogEndScope("Finished compiling fragment shader from Cfg to GLSL");

    //
    // Write GLSL file
    //
    if(CompiledGLSL && Options.FragmentShaderOutFilePath.Glsl)
    {
      auto const FileName = Options.FragmentShaderOutFilePath.Glsl;
      if(FileName == "-"_S)
      {
        printf("%s", StrPtr(GlslVertexShader.Code));
      }
      else if(!WriteArrayContentToFile(Slice(GlslFragmentShader.Code), FileName))
      {
        LogWarning("%*s: Failed to write glsl fragment shader file.", Convert<int>(FileName.Num), FileName.Ptr);
      }
    }

    //
    // Compile to SPIR-V
    //
    if(CompiledGLSL)
    {
      LogBeginScope("Compiling fragment shader from GLSL to SPIR-V");
      bool const CompiledSpv = CompileGlslToSpv(*Context, GlslFragmentShader, SpirvFragmentShader);
      LogEndScope("Finished compiling fragment shader from GLSL to SPIR-V");

      //
      // Write SPIR-V file
      //
      if(CompiledSpv && Options.FragmentShaderOutFilePath.Spirv)
      {
        auto const FileName = Options.FragmentShaderOutFilePath.Spirv;
        if(FileName == "-"_S)
        {
          printf("%*s", Convert<int>(SpirvFragmentShader.Code.Num * 4), Reinterpret<char const*>(SpirvFragmentShader.Code.Ptr));
        }
        else if(!WriteArrayContentToFile(Slice(SpirvFragmentShader.Code), FileName))
        {
          LogWarning("%*s: Failed to write spv fragment shader file.", Convert<int>(FileName.Num), FileName.Ptr);
        }
      }
    }
  }

  return 0;
}
