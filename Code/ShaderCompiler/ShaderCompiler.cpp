#include "ShaderCompiler.hpp"

#include <Core/Log.hpp>

template<typename T, typename U>
void
ArrayAppend(dynamic_array<T>* Array, slice<U> ToAppend)
{
  auto NewSlice = ExpandBy(Array, ToAppend.Num);
  SliceCopy(NewSlice, ToAppend);
}

cfg_node const*
FindSibling(cfg_node const* Node, char const* Name)
{
  auto const NameSlice = SliceFromString(Name);
  while(Node)
  {
    if(Node->Name == NameSlice)
      return Node;
    Node = Node->Next;
  }

  return nullptr;
}

namespace
{
  struct declaration
  {
    slice<char const> TypeName = {};
    slice<char const> Identifier = {};
    int Location = -1;
  };

  struct buffer
  {
    slice<char const> TypeName = {};
    slice<char const> Identifier = {};
    int Binding = -1;
    dynamic_array<declaration> Declarations = {};
  };
}

static void
GetDeclarations(cfg_node const* FirstSibling, dynamic_array<declaration>* OutDeclarations)
{
  auto Node = FirstSibling;
  while(Node)
  {
    declaration& Declaration = Expand(OutDeclarations);

    if(Node->Name == "float"_S ||
       Node->Name == "int"_S ||
       Node->Name == "vec2"_S ||
       Node->Name == "vec3"_S ||
       Node->Name == "vec4"_S ||
       Node->Name == "mat4"_S ||
       Node->Name == "sampler2D"_S)
    {
      Declaration.TypeName = Node->Name.Value;
    }
    else
    {
      LogError("Invalid declaration type name: %*s",
               Convert<int>(Node->Name.Value.Num), Node->Name.Value.Ptr);
      ShrinkBy(OutDeclarations, 1);
      continue;
    }

    if(Node->Values.Num == 0)
    {
      LogError("Expected at least 1 value.");
      ShrinkBy(OutDeclarations, 1);
      continue;
    }

    // TODO: What to do with the other values?
    Declaration.Identifier = Convert<slice<char const>>(Node->Values[0]);

    for(auto& Attribute : Slice(&Node->Attributes))
    {
      if(Attribute.Name == "Location"_S)
      {
        Declaration.Location = Convert<int>(Attribute.Value);
      }
    }

    Node = Node->Next;
  }
}

static void
GetShaderData(cfg_node const* ShaderNode,
              allocator_interface* Allocator,
              dynamic_array<declaration>* Sampler2Ds,         // Out
              dynamic_array<buffer>* Buffers,                 // Out
              dynamic_array<declaration>* InputDeclarations,  // Out
              dynamic_array<declaration>* OutputDeclarations, // Out
              slice<char const>* CodeEntry,                   // Out
              dynamic_array<slice<char const>>* Code)         // Out
{
  if(ShaderNode->FirstChild == nullptr)
    return;

  //
  // Buffers
  //
  {
    auto BuffersNode = FindSibling(ShaderNode->FirstChild, "Buffers");
    if(BuffersNode)
    {
      auto Node = BuffersNode->FirstChild;
      while(Node)
      {
        auto& Buffer = Expand(Buffers);

        if(Node->Name == "uniform"_S ||
           Node->Name == "buffer"_S)
        {
          Buffer.TypeName = Node->Name.Value;
        }
        else
        {
          LogError("Unknown buffer type: %*s",
                   Convert<int>(Node->Name.Value.Num), Node->Name.Value.Ptr);
          ShrinkBy(Buffers, 1);
          continue;
        }

        if(Node->Values.Num == 0)
        {
          LogError("Buffer needs to have a name.");
          ShrinkBy(Buffers, 1);
          continue;
        }

        Buffer.Identifier = Convert<slice<char const>>(Node->Values[0]);

        for(auto& Attribute : Slice(&Node->Attributes))
        {
          if(Attribute.Name == "Binding"_S)
          {
            Buffer.Binding = Convert<int>(Attribute.Value);
          }
        }

        Init(&Buffer.Declarations, Allocator);
        GetDeclarations(Node->FirstChild, &Buffer.Declarations);

        Node = Node->Next;
      }
    }
  }

  //
  // Input
  //
  {
    auto InputNode = FindSibling(ShaderNode->FirstChild, "Input");
    if(InputNode)
    {
      GetDeclarations(InputNode->FirstChild, InputDeclarations);
    }
    else
    {
      LogWarning("No input declarations in vertex shader.");
    }
  }


  //
  // Output
  //
  {
    auto OutputNode = FindSibling(ShaderNode->FirstChild, "Output");
    if(OutputNode)
    {
      GetDeclarations(OutputNode->FirstChild, OutputDeclarations);
    }
    else
    {
      LogWarning("No input declarations in vertex shader.");
    }
  }


  //
  // Code
  //
  {
    auto CodeNode = FindSibling(ShaderNode->FirstChild, "Code");
    if(CodeNode)
    {

      for(auto LineNode = CodeNode->FirstChild; LineNode; LineNode = LineNode->Next)
      {
        if(LineNode->Name != ""_S)
        {
          LogWarning("Ignoring named child in Code node.");
          continue;
        }

        for(auto& Line : Slice(&LineNode->Values))
        {
          Expand(Code) = Convert<slice<char const>>(Line);
        }
      }

      for(auto& Attribute : Slice(&CodeNode->Attributes))
      {
        if(Attribute.Name == "Entry"_S)
        {
          *CodeEntry = Convert<slice<char const>>(Attribute.Value);
        }
      }
    }
  }


  //
  // sampler2D
  //
  {
    cfg_node const* Sampler2DNode = ShaderNode->FirstChild;
    while(true)
    {
      Sampler2DNode = FindSibling(Sampler2DNode->Next, "sampler2D");

      if(Sampler2DNode == nullptr)
        break;
    }
  }
}

auto
::CompileCfgAsShader(cfg_node const* ShaderRoot, dynamic_array<char>* Result)
-> bool
{
  temp_allocator TempAllocator;
  allocator_interface* Allocator = *TempAllocator;

  fixed_block<256, char> FormattingFixedBuffer;
  auto FormattingBuffer = Slice(FormattingFixedBuffer);

  Clear(Result);

  scoped_array<declaration> Sampler2Ds{ Allocator };
  scoped_array<declaration> InputDeclarations{ Allocator };
  scoped_array<declaration> OutputDeclarations{ Allocator };
  slice<char const> CodeEntry{};
  scoped_array<slice<char const>> Code{ Allocator };
  scoped_array<buffer> Buffers{ Allocator };
  Defer [&Buffers]()
  {
    for(auto& Buffer : Slice(&Buffers))
    {
      Finalize(&Buffer.Declarations);
    }
  };

  GetShaderData(ShaderRoot,
                Allocator,
                &Sampler2Ds,
                &Buffers,
                &InputDeclarations,
                &OutputDeclarations,
                &CodeEntry,
                &Code);


  if(Sampler2Ds.Num)
  {
    for(auto& Decl : Slice(&Sampler2Ds))
    {
      ArrayAppend(Result, "layout("_S);
      bool NeedComma = false;
      if(Decl.Location != -1)
      {
        ArrayAppend(Result, "location="_S);
        auto LocationString = Convert<slice<char>>(Decl.Location, FormattingBuffer);
        ArrayAppend(Result, AsConst(LocationString));
        NeedComma = true;
      }
      ArrayAppend(Result, ") in "_S);
      ArrayAppend(Result, Decl.TypeName);
      ArrayAppend(Result, " "_S);
      ArrayAppend(Result, Decl.Identifier);
      ArrayAppend(Result, ";\n"_S);
    }
  }

  if(Buffers.Num)
  {
    ArrayAppend(Result, "\n"
                        "//\n"
                        "// Buffers\n"
                        "//\n"_S);
    for(auto& Buffer : Slice(&Buffers))
    {
      ArrayAppend(Result, "layout("_S);
      bool NeedComma = false;
      if(Buffer.Binding != -1)
      {
        ArrayAppend(Result, "binding="_S);
        auto BindingString = Convert<slice<char>>(Buffer.Binding, FormattingBuffer);
        ArrayAppend(Result, AsConst(BindingString));
        NeedComma = true;
      }
      ArrayAppend(Result, ") "_S);
      ArrayAppend(Result, Buffer.TypeName);
      ArrayAppend(Result, " "_S);
      ArrayAppend(Result, Buffer.Identifier);
      ArrayAppend(Result, "\n{\n"_S);
      for(auto& Decl : Slice(&Buffer.Declarations))
      {
        if(Decl.Location != -1)
        {
          LogWarning("Ignoring layout spec `location` in buffer declaration.");
        }

        ArrayAppend(Result, "  "_S);
        ArrayAppend(Result, Decl.TypeName);
        ArrayAppend(Result, " "_S);
        ArrayAppend(Result, Decl.Identifier);
        ArrayAppend(Result, ";\n"_S);
      }
      ArrayAppend(Result, "};\n"_S);
    }
  }

  if(InputDeclarations.Num)
  {
    ArrayAppend(Result, "\n"
                        "//\n"
                        "// Input\n"
                        "//\n"_S);

    for(auto& Decl : Slice(&InputDeclarations))
    {
      ArrayAppend(Result, "layout("_S);
      bool NeedComma = false;
      if(Decl.Location != -1)
      {
        // TODO: Location is an integer, convert it to string somehow.
        ArrayAppend(Result, "location="_S);
        auto LocationString = Convert<slice<char>>(Decl.Location, FormattingBuffer);
        ArrayAppend(Result, AsConst(LocationString));
        NeedComma = true;
      }
      ArrayAppend(Result, ") in "_S);
      ArrayAppend(Result, Decl.TypeName);
      ArrayAppend(Result, " "_S);
      ArrayAppend(Result, Decl.Identifier);
      ArrayAppend(Result, ";\n"_S);
    }
  }

  if(OutputDeclarations.Num)
  {
    ArrayAppend(Result, "\n"
                        "//\n"
                        "// Output\n"
                        "//\n"_S);

    for(auto& Decl : Slice(&OutputDeclarations))
    {
      ArrayAppend(Result, "layout("_S);
      bool NeedComma = false;
      if(Decl.Location != -1)
      {
        // TODO: Location is an integer, convert it to string somehow.
        ArrayAppend(Result, "location="_S);
        auto LocationString = Convert<slice<char>>(Decl.Location, FormattingBuffer);
        ArrayAppend(Result, AsConst(LocationString));
        NeedComma = true;
      }
      ArrayAppend(Result, ") out "_S);
      ArrayAppend(Result, Decl.TypeName);
      ArrayAppend(Result, " "_S);
      ArrayAppend(Result, Decl.Identifier);
      ArrayAppend(Result, ";\n"_S);
    }
    ArrayAppend(Result, "\n"_S);
  }

  if(Code.Num)
  {
    ArrayAppend(Result, "\n"
                        "//\n"
                        "// Code\n"
                        "//\n"_S);

    for(auto& Line : Slice(&Code))
    {
      ArrayAppend(Result, Line);
      ArrayAppend(Result, "\n"_S);
    }
  }

  return true;
}
