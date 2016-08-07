#include "TestHeader.hpp"
#include <ShaderCompiler/ShaderCompiler.hpp>
#include <Cfg/Cfg.hpp>
#include <Cfg/CfgParser.hpp>

static char const CfgSource[] =
  "VertexShader {\n"
  "  uniform \"Globals\" Binding=0 {\n"
  "    mat4 \"ViewProjectionMatrix\"\n"
  "  }\n"
  "  Input {\n"
  "    vec3 \"Position\" Location=0\n"
  "    vec4 \"Color\" Location=1\n"
  "  }\n"
  "  Output {\n"
  "    vec4 \"OutColor\" Location=0\n"
  "  }\n"
  "  Code Entry=\"main\" {\n"
  "    `\n"
  "    void main()\n"
  "    {\n"
  "      gl_Position = ViewProjectionMatrix * vec4(Position, 1.0f);\n"
  "      OutColor = Color;\n"
  "    }\n"
  "    `\n"
  "  }\n"
  "}\n"
  "FragmentShader {\n"
  "  sampler2D \"Sampler\" Binding=1\n"
  "  Input {\n"
  "    vec4 \"Color\" Location=0\n"
  "  }\n"
  "  Output {\n"
  "    vec4 \"FragmentColor\" Location=0\n"
  "  }\n"
  "  Code Entry=\"main\" {\n"
  "    `\n"
  "    void main()\n"
  "    {\n"
  "      FragmentColor = Color;\n"
  "    }\n"
  "    `\n"
  "  }\n"
  "}\n";

static char const ExpectedGlslVertexShader[] =
  "#version 450\n"
  "\n"
  "#extension GL_ARB_separate_shader_objects : enable\n"
  "#extension GL_ARB_shading_language_420pack : enable\n"
  "\n\n"
  "//\n"
  "// Buffers\n"
  "//\n"
  "layout(binding = 0) uniform Globals\n"
  "{\n"
  "  mat4 ViewProjectionMatrix;\n"
  "};\n"
  "\n\n"
  "//\n"
  "// Input\n"
  "//\n"
  "layout(location = 0) in vec3 Position;\n"
  "layout(location = 1) in vec4 Color;\n"
  "\n\n"
  "//\n"
  "// Output\n"
  "//\n"
  "layout(location = 0) out vec4 OutColor;\n"
  "\n\n"
  "//\n"
  "// Code\n"
  "//\n"
  "\n"
  "    void main()\n"
  "    {\n"
  "      gl_Position = ViewProjectionMatrix * vec4(Position, 1.0f);\n"
  "      OutColor = Color;\n"
  "    }\n"
  "    \n"
  "\n"
  "  \n";

static char const ExpectedGlslFragmentShader[] =
  "#version 450\n"
  "\n"
  "#extension GL_ARB_separate_shader_objects : enable\n"
  "#extension GL_ARB_shading_language_420pack : enable\n"
  "\n\n"
  "layout(binding = 1) uniform sampler2D Sampler;\n"
  "\n\n"
  "//\n"
  "// Input\n"
  "//\n"
  "layout(location = 0) in vec4 Color;\n"
  "\n\n"
  "//\n"
  "// Output\n"
  "//\n"
  "layout(location = 0) out vec4 FragmentColor;\n"
  "\n\n"
  "//\n"
  "// Code\n"
  "//\n"
  "\n"
  "    void main()\n"
  "    {\n"
  "      FragmentColor = Color;\n"
  "    }\n"
  "    \n"
  "\n"
  "  \n";

TEST_CASE("Shader Compiler", "[ShaderCompiler]")
{
  test_allocator TestAllocator{};
  allocator_interface* Allocator = &TestAllocator;

  cfg_document Document{};
  Init(&Document, Allocator);
  Defer [&](){ Finalize(&Document); };

  {
    // TODO: Supply the GlobalLog here as soon as the cfg parser code is more robust.
    cfg_parsing_context Context{ "Shader Compiler Test"_S, nullptr };

    bool const ParsedDocumentSuccessfully = CfgDocumentParseFromString(&Document, SliceFromString(CfgSource), &Context);
    REQUIRE( ParsedDocumentSuccessfully );
  }

  cfg_node* VertexShaderNode = nullptr;
  cfg_node* FragmentShaderNode = nullptr;

  for(auto Node = Document.Root->FirstChild; Node; Node = Node->Next)
  {
    if     (Node->Name == "VertexShader"_S)   VertexShaderNode = Node;
    else if(Node->Name == "FragmentShader"_S) FragmentShaderNode = Node;
  }

  REQUIRE( VertexShaderNode != nullptr );
  REQUIRE( FragmentShaderNode != nullptr );

  auto Context = CreateShaderCompilerContext(Allocator);
  Defer [=](){ DestroyShaderCompilerContext(Allocator, Context); };

  //
  // Vertex Shader
  //
  glsl_shader GlslVertexShader{};
  Init(GlslVertexShader, Allocator, glsl_shader_stage::Vertex);
  Defer [&](){ Finalize(GlslVertexShader); };

  bool const CompiledGlslVertexShader = CompileCfgToGlsl(Context, VertexShaderNode, &GlslVertexShader);
  REQUIRE( CompiledGlslVertexShader );

  REQUIRE( Slice(GlslVertexShader.EntryPoint) == "main"_S );
  REQUIRE( GlslVertexShader.Stage == glsl_shader_stage::Vertex );
  REQUIRE( Slice(GlslVertexShader.Code) == SliceFromString(ExpectedGlslVertexShader) );

  //
  // Fragment Shader
  //
  glsl_shader GlslFragmentShader{};
  Init(GlslFragmentShader, Allocator, glsl_shader_stage::Fragment);
  Defer [&](){ Finalize(GlslFragmentShader); };

  bool const CompiledGlslFragmentShader = CompileCfgToGlsl(Context, FragmentShaderNode, &GlslFragmentShader);
  REQUIRE( CompiledGlslFragmentShader );

  REQUIRE( Slice(GlslFragmentShader.EntryPoint) == "main"_S );
  REQUIRE( GlslFragmentShader.Stage == glsl_shader_stage::Fragment );
  REQUIRE( Slice(GlslFragmentShader.Code) == SliceFromString(ExpectedGlslFragmentShader) );
}
