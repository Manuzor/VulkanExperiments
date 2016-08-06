#include "ShaderCompiler.hpp"

#include <Core/Log.hpp>

#include <glslang/Public/ShaderLang.h>
#include <SPIRV/GlslangToSpv.h>

struct shader_compiler_context
{
};

auto
::Init(glsl_shader& GlslShader, allocator_interface* Allocator, glsl_shader_stage Stage)
  -> void
{
  GlslShader.Stage = Stage;
  GlslShader.EntryPoint.Allocator = Allocator;
  GlslShader.Code.Allocator = Allocator;
}

auto
::Finalize(glsl_shader& GlslShader)
  -> void
{
  StrClear(GlslShader.EntryPoint);
  StrClear(GlslShader.Code);
}

auto
Init(spirv_shader& SpirvShader, allocator_interface* Allocator)
  -> void
{
  Init(&SpirvShader.Code, Allocator);
}

auto
Finalize(spirv_shader& SpirvShader)
  -> void
{
  Finalize(&SpirvShader.Code);
}

int GlobalShaderCompilerCount = 0;

auto
::CreateShaderCompilerContext(allocator_interface* Allocator)
  -> shader_compiler_context*
{
  if(Allocator == nullptr)
    return nullptr;

  auto Context = Allocate<shader_compiler_context>(Allocator);

  if(Context == nullptr)
    return nullptr;

  *Context = {};

  if(GlobalShaderCompilerCount == 0)
  {
    glslang::InitializeProcess();
  }
  ++GlobalShaderCompilerCount;

  return Context;
}

auto
::DestroyShaderCompilerContext(allocator_interface* Allocator, shader_compiler_context* Context)
  -> void
{
  if(Allocator == nullptr || Context == nullptr)
    return;

  Deallocate(Allocator, Context);

  --GlobalShaderCompilerCount;
  if(GlobalShaderCompilerCount == 0)
  {
    glslang::FinalizeProcess();
  }
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
              arc_string* EntryPoint,                         // Out
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
          auto EntryPointValue = Convert<slice<char const>>(Attribute.Value);
          *EntryPoint = EntryPointValue;
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
::CompileCfgToGlsl(shader_compiler_context* Context, cfg_node const* ShaderRoot,
                   glsl_shader* GlslShader)
-> bool
{
  temp_allocator TempAllocator;
  allocator_interface* Allocator = *TempAllocator;

  fixed_block<256, char> FormattingFixedBuffer;
  auto FormattingBuffer = Slice(FormattingFixedBuffer);

  StrClear(GlslShader->EntryPoint);
  StrClear(GlslShader->Code);

  scoped_array<declaration> Sampler2Ds{ Allocator };
  scoped_array<declaration> InputDeclarations{ Allocator };
  scoped_array<declaration> OutputDeclarations{ Allocator };
  slice<char const> EntryPoint{};
  scoped_array<slice<char const>> ExtraCode{ Allocator };
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
                &GlslShader->EntryPoint,
                &ExtraCode);

  // TODO: Support reading #version and #extension values from cfg?
  // Add default version and extensions.
  GlslShader->Code += "#version 450\n\n"
                      "#extension GL_ARB_separate_shader_objects : enable\n"
                      "#extension GL_ARB_shading_language_420pack : enable\n\n";

  if(Sampler2Ds.Num)
  {
    for(auto& Decl : Slice(&Sampler2Ds))
    {
      GlslShader->Code += "layout(";
      bool NeedComma = false;
      if(Decl.Location != -1)
      {
        GlslShader->Code += "location=(";
        auto LocationString = Convert<slice<char>>(Decl.Location, FormattingBuffer);
        GlslShader->Code += LocationString;
        NeedComma = true;
      }
      GlslShader->Code += ") in ";
      GlslShader->Code += Decl.TypeName;
      GlslShader->Code += " ";
      GlslShader->Code += Decl.Identifier;
      GlslShader->Code += ";\n";
    }
  }

  if(Buffers.Num)
  {
    GlslShader->Code += "\n"
                        "//\n"
                        "// Buffers\n"
                        "//\n";
    for(auto& Buffer : Slice(&Buffers))
    {
      GlslShader->Code += "layout(";
      bool NeedComma = false;
      if(Buffer.Binding != -1)
      {
        GlslShader->Code += "binding=";
        auto BindingString = Convert<slice<char>>(Buffer.Binding, FormattingBuffer);
        GlslShader->Code += BindingString;
        NeedComma = true;
      }
      GlslShader->Code += ") ";
      GlslShader->Code += Buffer.TypeName;
      GlslShader->Code += " ";
      GlslShader->Code += Buffer.Identifier;
      GlslShader->Code += "\n{\n";
      for(auto& Decl : Slice(&Buffer.Declarations))
      {
        if(Decl.Location != -1)
        {
          LogWarning("Ignoring layout spec `location` in buffer declaration.");
        }

        GlslShader->Code += "  ";
        GlslShader->Code += Decl.TypeName;
        GlslShader->Code += " ";
        GlslShader->Code += Decl.Identifier;
        GlslShader->Code += ";\n";
      }
      GlslShader->Code += "};\n";
    }
  }

  if(InputDeclarations.Num)
  {
    GlslShader->Code += "\n"
                        "//\n"
                        "// Input\n"
                        "//\n";

    for(auto& Decl : Slice(&InputDeclarations))
    {
      GlslShader->Code += "layout(";
      bool NeedComma = false;
      if(Decl.Location != -1)
      {
        // TODO: Location is an integer, convert it to string somehow.
        GlslShader->Code += "location=";
        auto LocationString = Convert<slice<char>>(Decl.Location, FormattingBuffer);
        GlslShader->Code += LocationString;
        NeedComma = true;
      }
      GlslShader->Code += ") in ";
      GlslShader->Code += Decl.TypeName;
      GlslShader->Code += " ";
      GlslShader->Code += Decl.Identifier;
      GlslShader->Code += ";\n";
    }
  }

  if(OutputDeclarations.Num)
  {
    GlslShader->Code += "\n"
                        "//\n"
                        "// Output\n"
                        "//\n";

    for(auto& Decl : Slice(&OutputDeclarations))
    {
      GlslShader->Code += "layout(";
      bool NeedComma = false;
      if(Decl.Location != -1)
      {
        // TODO: Location is an integer, convert it to string somehow.
        GlslShader->Code += "location=";
        auto LocationString = Convert<slice<char>>(Decl.Location, FormattingBuffer);
        GlslShader->Code += AsConst(LocationString);
        NeedComma = true;
      }
      GlslShader->Code += ") out ";
      GlslShader->Code += Decl.TypeName;
      GlslShader->Code += " ";
      GlslShader->Code += Decl.Identifier;
      GlslShader->Code += ";\n";
    }
    GlslShader->Code += "\n";
  }

  if(ExtraCode.Num)
  {
    GlslShader->Code += "\n"
                        "//\n"
                        "// Code\n"
                        "//\n";

    for(auto& Line : Slice(&ExtraCode))
    {
      GlslShader->Code += Line;
      GlslShader->Code += "\n";
    }
  }

  return true;
}

static EShLanguage
ToShLanguage(glsl_shader_stage ShaderStage)
{
  switch(ShaderStage)
  {
    case glsl_shader_stage::Vertex:                return EShLangVertex;
    case glsl_shader_stage::TesselationControl:    return EShLangTessControl;
    case glsl_shader_stage::TesselationEvaluation: return EShLangTessEvaluation;
    case glsl_shader_stage::Geometry:              return EShLangGeometry;
    case glsl_shader_stage::Fragment:              return EShLangFragment;
    case glsl_shader_stage::Compute:               return EShLangCompute;
    default: return EShLangCount;
  };
}

// This is defined at the end of this file.
extern const TBuiltInResource GlobalDefaultGlslangBuiltInResources;

auto
::CompileGlslToSpv(shader_compiler_context* Context, glsl_shader const* GlslShader,
                   spirv_shader* SpirvShader)
  -> bool
{
  {
    glslang::TShader Shader{ ToShLanguage(GlslShader->Stage) };

    // Note: Program needs to be destructed before all shaders it references.
    glslang::TProgram Program;


    int NumBytes = Convert<int>(StrNumBytes(GlslShader->Code));
    auto StringPtr = StrPtr(GlslShader->Code);
    Shader.setStringsWithLengths(&StringPtr, &NumBytes, 1);
    Shader.setEntryPoint(StrPtr(GlslShader->EntryPoint));

    const int DefaultVersion = 110; // For Desktop;
    EShMessages Messages{ Coerce<EShMessages>(EShMsgDefault | EShMsgSpvRules | EShMsgVulkanRules) };
    if(!Shader.parse(&GlobalDefaultGlslangBuiltInResources, DefaultVersion, false, Messages))
    {
      LogError("Shader compilation failed.");

      auto ShaderInfo = Shader.getInfoLog();
      if(ShaderInfo)
        LogError(ShaderInfo);

      auto ShaderDebugInfo = Shader.getInfoDebugLog();
      if(ShaderDebugInfo)
        LogError(ShaderDebugInfo);

      return false;
    }

    Program.addShader(&Shader);

    if(!Program.link(Messages))
    {
      LogError("Shader linking failed.");

      auto ProgramInfo = Program.getInfoLog();
      if(ProgramInfo)
        LogError(ProgramInfo);

      auto ProgramDebugInfo = Program.getInfoDebugLog();
      if(ProgramDebugInfo)
        LogError(ProgramDebugInfo);

      return false;
    }

    // Now the SPIR-V part
    {
      auto Intermediate = Program.getIntermediate(Shader.getStage());
      if(!Intermediate)
      {
        LogError("wtf?!");
        return false;
      }

      std::vector<unsigned int> SpirvCodeVector;
      spv::SpvBuildLogger Logger;
      glslang::GlslangToSpv(*Intermediate, SpirvCodeVector, &Logger);

      auto BuildMessages = Logger.getAllMessages();
      if(!BuildMessages.empty())
      {
        LogInfo("SPIR-V compilation messages:\n%s", BuildMessages.c_str());
      }

      // Copy over the result.
      SetNum(&SpirvShader->Code, SpirvCodeVector.size());
      SliceCopy(Slice(&SpirvShader->Code),
                AsConst(Slice(SpirvCodeVector.size(), SpirvCodeVector.data())));
    }
  }

  return true;
}

auto
CompileCfgToGlslAndSpv(shader_compiler_context* Context, cfg_node const* ShaderRoot,
                       glsl_shader* GlslShader, spirv_shader* SpirvShader)
  -> bool
{
  return CompileCfgToGlsl(Context, ShaderRoot, GlslShader) && CompileGlslToSpv(Context, GlslShader, SpirvShader);
}


const TBuiltInResource GlobalDefaultGlslangBuiltInResources = {
    /* .MaxLights = */ 32,
    /* .MaxClipPlanes = */ 6,
    /* .MaxTextureUnits = */ 32,
    /* .MaxTextureCoords = */ 32,
    /* .MaxVertexAttribs = */ 64,
    /* .MaxVertexUniformComponents = */ 4096,
    /* .MaxVaryingFloats = */ 64,
    /* .MaxVertexTextureImageUnits = */ 32,
    /* .MaxCombinedTextureImageUnits = */ 80,
    /* .MaxTextureImageUnits = */ 32,
    /* .MaxFragmentUniformComponents = */ 4096,
    /* .MaxDrawBuffers = */ 32,
    /* .MaxVertexUniformVectors = */ 128,
    /* .MaxVaryingVectors = */ 8,
    /* .MaxFragmentUniformVectors = */ 16,
    /* .MaxVertexOutputVectors = */ 16,
    /* .MaxFragmentInputVectors = */ 15,
    /* .MinProgramTexelOffset = */ -8,
    /* .MaxProgramTexelOffset = */ 7,
    /* .MaxClipDistances = */ 8,
    /* .MaxComputeWorkGroupCountX = */ 65535,
    /* .MaxComputeWorkGroupCountY = */ 65535,
    /* .MaxComputeWorkGroupCountZ = */ 65535,
    /* .MaxComputeWorkGroupSizeX = */ 1024,
    /* .MaxComputeWorkGroupSizeY = */ 1024,
    /* .MaxComputeWorkGroupSizeZ = */ 64,
    /* .MaxComputeUniformComponents = */ 1024,
    /* .MaxComputeTextureImageUnits = */ 16,
    /* .MaxComputeImageUniforms = */ 8,
    /* .MaxComputeAtomicCounters = */ 8,
    /* .MaxComputeAtomicCounterBuffers = */ 1,
    /* .MaxVaryingComponents = */ 60,
    /* .MaxVertexOutputComponents = */ 64,
    /* .MaxGeometryInputComponents = */ 64,
    /* .MaxGeometryOutputComponents = */ 128,
    /* .MaxFragmentInputComponents = */ 128,
    /* .MaxImageUnits = */ 8,
    /* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
    /* .MaxCombinedShaderOutputResources = */ 8,
    /* .MaxImageSamples = */ 0,
    /* .MaxVertexImageUniforms = */ 0,
    /* .MaxTessControlImageUniforms = */ 0,
    /* .MaxTessEvaluationImageUniforms = */ 0,
    /* .MaxGeometryImageUniforms = */ 0,
    /* .MaxFragmentImageUniforms = */ 8,
    /* .MaxCombinedImageUniforms = */ 8,
    /* .MaxGeometryTextureImageUnits = */ 16,
    /* .MaxGeometryOutputVertices = */ 256,
    /* .MaxGeometryTotalOutputComponents = */ 1024,
    /* .MaxGeometryUniformComponents = */ 1024,
    /* .MaxGeometryVaryingComponents = */ 64,
    /* .MaxTessControlInputComponents = */ 128,
    /* .MaxTessControlOutputComponents = */ 128,
    /* .MaxTessControlTextureImageUnits = */ 16,
    /* .MaxTessControlUniformComponents = */ 1024,
    /* .MaxTessControlTotalOutputComponents = */ 4096,
    /* .MaxTessEvaluationInputComponents = */ 128,
    /* .MaxTessEvaluationOutputComponents = */ 128,
    /* .MaxTessEvaluationTextureImageUnits = */ 16,
    /* .MaxTessEvaluationUniformComponents = */ 1024,
    /* .MaxTessPatchComponents = */ 120,
    /* .MaxPatchVertices = */ 32,
    /* .MaxTessGenLevel = */ 64,
    /* .MaxViewports = */ 16,
    /* .MaxVertexAtomicCounters = */ 0,
    /* .MaxTessControlAtomicCounters = */ 0,
    /* .MaxTessEvaluationAtomicCounters = */ 0,
    /* .MaxGeometryAtomicCounters = */ 0,
    /* .MaxFragmentAtomicCounters = */ 8,
    /* .MaxCombinedAtomicCounters = */ 8,
    /* .MaxAtomicCounterBindings = */ 1,
    /* .MaxVertexAtomicCounterBuffers = */ 0,
    /* .MaxTessControlAtomicCounterBuffers = */ 0,
    /* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
    /* .MaxGeometryAtomicCounterBuffers = */ 0,
    /* .MaxFragmentAtomicCounterBuffers = */ 1,
    /* .MaxCombinedAtomicCounterBuffers = */ 1,
    /* .MaxAtomicCounterBufferSize = */ 16384,
    /* .MaxTransformFeedbackBuffers = */ 4,
    /* .MaxTransformFeedbackInterleavedComponents = */ 64,
    /* .MaxCullDistances = */ 8,
    /* .MaxCombinedClipAndCullDistances = */ 8,
    /* .MaxSamples = */ 4,
    /* .limits = */ {
        /* .nonInductiveForLoops = */ 1,
        /* .whileLoops = */ 1,
        /* .doWhileLoops = */ 1,
        /* .generalUniformIndexing = */ 1,
        /* .generalAttributeMatrixVectorIndexing = */ 1,
        /* .generalVaryingIndexing = */ 1,
        /* .generalSamplerIndexing = */ 1,
        /* .generalVariableIndexing = */ 1,
        /* .generalConstantMatrixVectorIndexing = */ 1,
    }};
