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
  SpirvShader.Code.Allocator = Allocator;
}

auto
Finalize(spirv_shader& SpirvShader)
  -> void
{
  Reset(SpirvShader.Code);
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
    int Binding = -1;
  };

  struct buffer : public declaration
  {
    array<declaration> InnerDeclarations{};
  };
}

static void
GetDeclarations(cfg_node const* FirstSibling, array<declaration>& OutDeclarations)
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

    for(auto& Attribute : Slice(Node->Attributes))
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
              array<declaration>& MiscGlobals,        // Out
              array<buffer>& Buffers,                 // Out
              array<declaration>& InputDeclarations,  // Out
              array<declaration>& OutputDeclarations, // Out
              arc_string* EntryPoint,                 // Out
              array<slice<char const>>& Code)         // Out
{
  if(ShaderNode->FirstChild == nullptr)
    return;

  for(auto Node = ShaderNode->FirstChild;
      Node != nullptr;
      Node = Node->Next)
  {
    if(Node->Name == "Input"_S)
    {
      GetDeclarations(Node->FirstChild, InputDeclarations);
    }
    else if(Node->Name == "Output"_S)
    {
      GetDeclarations(Node->FirstChild, OutputDeclarations);
    }
    else if(Node->Name == "Code"_S)
    {
      for(auto LineNode = Node->FirstChild; LineNode; LineNode = LineNode->Next)
      {
        if(LineNode->Name != ""_S)
        {
          LogWarning("Ignoring named child in Code node.");
          continue;
        }

        for(auto& Line : Slice(LineNode->Values))
        {
          Expand(Code) = Convert<slice<char const>>(Line);
        }
      }

      for(auto& Attribute : Slice(Node->Attributes))
      {
        if(Attribute.Name == "Entry"_S)
        {
          auto EntryPointValue = Convert<slice<char const>>(Attribute.Value);
          *EntryPoint = EntryPointValue;
        }
      }
    }
    else if(Node->Name == "uniform"_S || Node->Name == "buffer"_S)
    {
        auto& Buffer = Expand(Buffers);
        Buffer.TypeName = Node->Name.Value;

        if(Node->Values.Num == 0)
        {
          LogError("Buffer needs to have a name.");
          ShrinkBy(Buffers, 1);
          continue;
        }

        Buffer.Identifier = Convert<slice<char const>>(Node->Values[0]);

        for(auto& Attribute : Slice(Node->Attributes))
        {
          if(Attribute.Name == "Binding"_S)
          {
            Buffer.Binding = Convert<int>(Attribute.Value);
          }
        }

        Buffer.InnerDeclarations.Allocator = Allocator;
        GetDeclarations(Node->FirstChild, Buffer.InnerDeclarations);
    }
    else if(Node->Name == "sampler2D"_S)
    {
      auto& Sampler2D = Expand(MiscGlobals);
      Sampler2D.TypeName = Node->Name.Value;

      if(Node->Values.Num == 0)
      {
        LogError("sampler2D needs to have a name.");
        ShrinkBy(MiscGlobals, 1);
        continue;
      }

      Sampler2D.Identifier = Convert<slice<char const>>(Node->Values[0]);

      for(auto& Attribute : Slice(Node->Attributes))
      {
        if(Attribute.Name == "Binding"_S)
        {
          Sampler2D.Binding = Convert<int>(Attribute.Value);
        }
      }
    }
    else
    {
      arc_string NodeName = Node->Name.Value;
      LogWarning("Unrecognized top-level node: %s", StrPtr(NodeName));
    }
  }
}

static void
WriteDeclaration(slice<char const> Prefix, declaration const& Decl, arc_string& String, bool WriteLayout, bool WriteSemicolon = true)
{
  if(WriteLayout)
  {
    // Used to format integers to a string.
    fixed_block<256, char> FormattingFixedBuffer;
    auto FormattingBuffer = Slice(FormattingFixedBuffer);

    bool NeedComma = false;

    String += "layout(";

    if(Decl.Location != -1)
    {
      if(NeedComma)
        String += ", ";

      String += "location = ";
      auto LocationString = Convert<slice<char>>(Decl.Location, FormattingBuffer);
      String += LocationString;
      NeedComma = true;
    }

    if(Decl.Binding != -1)
    {
      if(NeedComma)
        String += ", ";

      String += "binding = ";
      auto BindingString = Convert<slice<char>>(Decl.Binding, FormattingBuffer);
      String += BindingString;
      NeedComma = true;
    }

    String += ") ";
  }

  if(Prefix)
  {
    String += Prefix;
    String += " ";
  }

  String += Decl.TypeName;
  String += " ";
  String += Decl.Identifier;

  if(WriteSemicolon)
    String += ";";

  String += "\n";
}

auto
::CompileCfgToGlsl(shader_compiler_context* Context, cfg_node const* ShaderRoot,
                   glsl_shader* GlslShader)
-> bool
{
  temp_allocator TempAllocator;
  allocator_interface* Allocator = *TempAllocator;

  StrClear(GlslShader->EntryPoint);
  StrClear(GlslShader->Code);

  array<declaration> MiscGlobals{ Allocator };
  array<declaration> InputDeclarations{ Allocator };
  array<declaration> OutputDeclarations{ Allocator };
  slice<char const> EntryPoint{};
  array<slice<char const>> ExtraCode{ Allocator };
  array<buffer> Buffers{ Allocator };

  GetShaderData(ShaderRoot,
                Allocator,
                MiscGlobals,
                Buffers,
                InputDeclarations,
                OutputDeclarations,
                &GlslShader->EntryPoint,
                ExtraCode);

  // TODO: Support reading #version and #extension values from cfg?
  // Add default version and extensions.
  GlslShader->Code += "#version 450\n\n"
                      "#extension GL_ARB_separate_shader_objects : enable\n"
                      "#extension GL_ARB_shading_language_420pack : enable\n\n";

  if(MiscGlobals.Num > 0)
  {
    GlslShader->Code += "\n";
    for(auto& Decl : Slice(MiscGlobals))
    {
      WriteDeclaration("uniform"_S, Decl, GlslShader->Code, true);
    }
    GlslShader->Code += "\n";
  }

  if(Buffers.Num)
  {
    GlslShader->Code += "\n"
                        "//\n"
                        "// Buffers\n"
                        "//\n";
    for(auto& Buffer : Slice(Buffers))
    {
      WriteDeclaration({}, Buffer, GlslShader->Code, true, false);

      GlslShader->Code += "{\n";
      for(auto& Decl : Slice(Buffer.InnerDeclarations))
      {
        if(Decl.Location != -1)
        {
          LogWarning("Ignoring layout spec `location` in buffer declaration.");
        }

        if(Decl.Binding != -1)
        {
          LogWarning("Ignoring layout spec `binding` in buffer declaration.");
        }

        GlslShader->Code += "  ";
        WriteDeclaration({}, Decl, GlslShader->Code, false);
      }
      GlslShader->Code += "};\n";
    }
    GlslShader->Code += "\n";
  }

  if(InputDeclarations.Num)
  {
    GlslShader->Code += "\n"
                        "//\n"
                        "// Input\n"
                        "//\n";

    for(auto& Decl : Slice(InputDeclarations))
    {
      WriteDeclaration("in"_S, Decl, GlslShader->Code, true);
    }
    GlslShader->Code += "\n";
  }

  if(OutputDeclarations.Num)
  {
    GlslShader->Code += "\n"
                        "//\n"
                        "// Output\n"
                        "//\n";

    for(auto& Decl : Slice(OutputDeclarations))
    {
      WriteDeclaration("out"_S, Decl, GlslShader->Code, true);
    }
    GlslShader->Code += "\n";
  }

  if(ExtraCode.Num)
  {
    GlslShader->Code += "\n"
                        "//\n"
                        "// Code\n"
                        "//\n";

    for(auto& Line : Slice(ExtraCode))
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
    case glsl_shader_stage::Vertex:                 return EShLangVertex;
    case glsl_shader_stage::TessellationControl:    return EShLangTessControl;
    case glsl_shader_stage::TessellationEvaluation: return EShLangTessEvaluation;
    case glsl_shader_stage::Geometry:               return EShLangGeometry;
    case glsl_shader_stage::Fragment:               return EShLangFragment;
    case glsl_shader_stage::Compute:                return EShLangCompute;
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
      SetNum(SpirvShader->Code, SpirvCodeVector.size());
      SliceCopy(Slice(SpirvShader->Code),
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
