#include <Backbone.hpp>
#include <Core/DynamicArray.hpp>
#include <Core/Log.hpp>

#include <Cfg/Cfg.hpp>
#include <Cfg/CfgParser.hpp>

#include "ShaderCompiler.hpp"

#include <stdio.h>


static bool
ReadFileContentIntoArray(dynamic_array<uint8>* Array, char const* FileName)
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
      LogError("Didn't reach the end of file but failed to read any more bytes: %s", FileName);
      return false;
    }
  }
}

struct cmd_options
{
  slice<char const> InputFilePath;
  slice<char const> VertexShaderOutFilePath;
  slice<char const> FragmentShaderOutFilePath;
};

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
        LogInfo("Args: (-h | -help) | <InputFilePath> [-vert <VertexShaderOutFilePath>] [-frag <FragmentShaderOutFilePath>]");
        return false;
      }
      else if(Arg == "vert"_S)
      {
        ++Index;
        if(Index >= Args.Num)
        {
          LogError("Missing argument for -vert.");
          return false;
        }

        Options->VertexShaderOutFilePath = SliceFromString(Args[Index]);
      }
      else if(Arg == "frag"_S)
      {
        ++Index;
        if(Index >= Args.Num)
        {
          LogError("Missing argument for -frag.");
          return false;
        }

        Options->FragmentShaderOutFilePath = SliceFromString(Args[Index]);
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
        return false;
      }
    }
  }

  if(!Options->InputFilePath)
  {
    LogError("Missing required positional argument (InputFilePath).");
    return false;
  }

  return true;
}


int
main(int NumArgs, char const* Args[])
{
  mallocator Mallocator = {};
  allocator_interface* Allocator = &Mallocator;
  log_data Log = {};
  GlobalLog = &Log;
  Defer [=](){ GlobalLog = nullptr; };

  Init(GlobalLog, Allocator);
  Defer [=](){ Finalize(GlobalLog); };
  {
    auto DefaultSinkSlots = ExpandBy(&GlobalLog->Sinks, 2);
    DefaultSinkSlots[0] = log_sink(StdoutLogSink);
    DefaultSinkSlots[1] = log_sink(VisualStudioLogSink);
  }

  cmd_options Options{};
  if(!ParseCommandLineOptions(Slice(NumArgs - 1, &Args[1]), &Options))
  {
    return -1;
  }

  if(!Options.VertexShaderOutFilePath)
  {
    // TODO: Use the InputFilePath name and change the extension.
    Options.VertexShaderOutFilePath = "Shader.vert"_S;
  }

  if(!Options.FragmentShaderOutFilePath)
  {
    // TODO: Use the InputFilePath name and change the extension.
    Options.FragmentShaderOutFilePath = "Shader.frag"_S;
  }

  char const* InputFilePath = Options.InputFilePath.Ptr; // It's 0-terminated.

  scoped_array<uint8> Content{ Allocator };
  if(!ReadFileContentIntoArray(&Content, InputFilePath))
  {
    printf("%s: Failed to read file\n", InputFilePath);
    return 2;
  }

  cfg_document Document{};
  Init(&Document, Allocator);
  Defer [&](){ Finalize(&Document); };

  {
    // TODO: Supply the GlobalLog here as soon as the cfg parser code is more robust.
    cfg_parsing_context Context{ SliceFromString(InputFilePath), nullptr };

    if(!CfgDocumentParseFromString(&Document, SliceReinterpret<char const>(Slice(&Content)), &Context))
    {
      printf("An error occurred while parsing the document.\n");
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
  Defer [=](){ DestroyShaderCompilerContext(Allocator, Context); };

  //
  // Vertex Shader
  //
  glsl_shader VertexShader{ Allocator };
  scoped_array<uint32> VertexShaderByteCode{ Allocator };
  if(VertexShaderNode)
  {
    if(!CompileCfgToGlsl(Context, VertexShaderNode, &VertexShader))
    {
      printf("Failed to compile vertex shader from cfg.");
      return 4;
    }

    auto const FileName = Options.VertexShaderOutFilePath;
    LogBeginScope("Compiling To Spv: %*s", Convert<int>(FileName.Num), FileName.Ptr);
    if(!CompileGlslToSpv(Context, &VertexShader, &VertexShaderByteCode))
    {
      printf("Failed to compile generated GLSL code to SPIR-V bytecode.");
      return 42;
    }
    LogEndScope("Compiled: %*s", Convert<int>(FileName.Num), FileName.Ptr);
  }


  //
  // Fragment Shader
  //
  glsl_shader FragmentShader{ Allocator };
  if(FragmentShaderNode)
  {
    if(!CompileCfgToGlsl(Context, FragmentShaderNode, &FragmentShader))
    {
      printf("Failed to extract fragment shader from cfg.");
      return 4;
    }
  }

  // TODO: Write the results to the respective files.

  // printf("%*s: %s",
  //        Convert<int>(Options.VertexShaderOutFilePath.Num), Options.VertexShaderOutFilePath.Ptr,
  //        VertexShader.Code.Ptr);

  // printf("%*s: %s",
  //        Convert<int>(Options.FragmentShaderOutFilePath.Num), Options.FragmentShaderOutFilePath.Ptr,
  //        FragmentShader.Code.Ptr);

  return 0;
}
