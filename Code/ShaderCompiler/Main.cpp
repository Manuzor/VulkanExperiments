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
  // TODO: Set up logging.

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

  char const* FileName = Options.InputFilePath.Ptr; // It's 0-terminated.

  temp_allocator TempAllocator;
  allocator_interface* Allocator = *TempAllocator;

  scoped_array<uint8> Content{ Allocator };
  if(!ReadFileContentIntoArray(&Content, FileName))
  {
    printf("%s: Failed to read file\n", FileName);
    return 2;
  }

  cfg_document Document{};
  Init(&Document, Allocator);
  Defer [&](){ Finalize(&Document); };

  cfg_parsing_context Context{ SliceFromString(FileName), GlobalLog };

  if(!CfgDocumentParseFromString(&Document, SliceReinterpret<char const>(Slice(&Content)), &Context))
  {
    printf("An error occurred while parsing the document.\n");
    return 3;
  }

  cfg_node* VertexShaderNode = nullptr;
  cfg_node* FragmentShaderNode = nullptr;

  for(auto Node = Document.Root->FirstChild; Node; Node = Node->Next)
  {
    if     (Node->Name == "VertexShader"_S)   VertexShaderNode = Node;
    else if(Node->Name == "FragmentShader"_S) FragmentShaderNode = Node;
  }

  scoped_array<char> VertexShaderCode{ Allocator };
  if(VertexShaderNode)
  {
    if(!CompileCfgAsShader(VertexShaderNode, &VertexShaderCode))
    {
      printf("Failed to extract vertex shader from cfg.");
      return 4;
    }
    Expand(&VertexShaderCode) = '\0';
  }

  scoped_array<char> FragmentShaderCode{ Allocator };
  if(FragmentShaderNode)
  {
    if(!CompileCfgAsShader(FragmentShaderNode, &FragmentShaderCode))
    {
      printf("Failed to extract fragment shader from cfg.");
      return 4;
    }
    Expand(&FragmentShaderCode) = '\0';
  }

  // TODO: Write the results to the respective files.

  printf("%*s: %s",
         Convert<int>(Options.VertexShaderOutFilePath.Num), Options.VertexShaderOutFilePath.Ptr,
         VertexShaderCode.Ptr);

  printf("%*s: %s",
         Convert<int>(Options.FragmentShaderOutFilePath.Num), Options.FragmentShaderOutFilePath.Ptr,
         FragmentShaderCode.Ptr);

  return 0;
}
