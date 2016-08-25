#include "TestHeader.hpp"
#include <Cfg/Cfg.hpp>
#include <Cfg/CfgParser.hpp>

#include <Core/Log.hpp>


static cfg_source
MakeSourceDataForTesting(char const* String, size_t Offset)
{
  cfg_source Source;
  Source.Value = SliceFromString(String);
  Source.StartLocation = { 1, 1, Offset };
  Source.EndLocation   = { 0, 0, Source.Value.Num };

  return Source;
}

TEST_CASE("Cfg: Skip whitespace and comments", "[Cfg]")
{
  cfg_parsing_context Context{ "Cfg Test 1"_S, GlobalLog };

  SECTION("Simple")
  {
    auto Source = MakeSourceDataForTesting("// hello\nworld", 0);

    auto Result = CfgSourceSkipWhiteSpaceAndComments(&Source, &Context, cfg_consume_newline::Yes);
    REQUIRE( Result.StartLocation.SourceIndex == 0 );
    REQUIRE( Result.EndLocation.SourceIndex == 9 );
    REQUIRE( Source.StartLocation.SourceIndex == 9 );
    REQUIRE( Source.EndLocation.SourceIndex == Source.Value.Num );
  }

  SECTION("Many Different Comment Styles")
  {
    auto Source = MakeSourceDataForTesting(
      "// C++ style\n\n"
      "/*\n"
      "C style multiline\n"
      "*/\n\n"
      "/*foo=true*/\n\n"
      "# Shell style\n\n"
      "-- Lua style\n\n"
      "text",
      0);

    auto Result = CfgSourceSkipWhiteSpaceAndComments(&Source, &Context, cfg_consume_newline::Yes);
    REQUIRE( Result.StartLocation.SourceIndex == 0 );
    REQUIRE( Result.EndLocation.SourceIndex == 82 );
    REQUIRE( Source.StartLocation.SourceIndex == 82 );
    REQUIRE( Source.EndLocation.SourceIndex == Source.Value.Num );
    REQUIRE( CfgSourceCurrentValue(Source) == "text"_S );
  }
}

TEST_CASE("Cfg: Parse until", "[Cfg]")
{
  cfg_parsing_context Context{ "Cfg Test 2"_S, GlobalLog };

  auto Source = MakeSourceDataForTesting("foo \"bar\"", 0);

  auto Result = CfgSourceParseUntil(&Source, &Context, [](slice<char const> Str) { return IsWhitespace(Str[0]); });
  REQUIRE(Result.StartLocation.SourceIndex == 0);
  REQUIRE(Result.EndLocation.SourceIndex == 3);
  REQUIRE(Source.StartLocation.SourceIndex == 3);
  REQUIRE(Source.EndLocation.SourceIndex == Source.Value.Num);
}


TEST_CASE("Cfg: Parse nested", "[Cfg]")
{
  cfg_parsing_context Context{ "Cfg Test 3"_S, GlobalLog };

  {
    auto Source = MakeSourceDataForTesting("foo { bar }; baz", 5);

    auto Result = CfgSourceParseNested(&Source, &Context, "{"_S, "}"_S);
    REQUIRE( Result.StartLocation.SourceIndex == 5 );
    REQUIRE( Result.EndLocation.SourceIndex == 10 );
    REQUIRE( Source.StartLocation.SourceIndex == 11 );
    REQUIRE( Source.EndLocation.SourceIndex == Source.Value.Num );
  }

  {
    auto Source = MakeSourceDataForTesting("foo { bar { baz } }; qux", 5);

    auto Result = CfgSourceParseNested(&Source, &Context, "{"_S, "}"_S);
    REQUIRE( Result.StartLocation.SourceIndex == 5 );
    REQUIRE( Result.EndLocation.SourceIndex == 18 );
    REQUIRE( Source.StartLocation.SourceIndex == 19 );
    REQUIRE( Source.EndLocation.SourceIndex == Source.Value.Num );
  }
}

TEST_CASE("Cfg: Parse escaped", "[Cfg]")
{
  cfg_parsing_context Context{ "Cfg Test 4"_S, GlobalLog };

  {
    auto Source = MakeSourceDataForTesting("foo \"bar\" \"baz\"", 5);

    auto Result = CfgSourceParseEscaped(&Source, &Context, '\\', "\""_S, cfg_consume_newline::Yes);
    REQUIRE( Result.StartLocation.SourceIndex == 5 );
    REQUIRE( Result.EndLocation.SourceIndex   == 8 );
    REQUIRE( Source.StartLocation.SourceIndex == 9 );
    REQUIRE( Source.EndLocation.SourceIndex   == Source.Value.Num );
  }

  {
    auto Source = MakeSourceDataForTesting("foo \"bar\\\"baz\" \"qux\"", 5);

    auto Result = CfgSourceParseEscaped(&Source, &Context, '\\', "\""_S, cfg_consume_newline::Yes);
    REQUIRE( Result.StartLocation.SourceIndex == 5 );
    REQUIRE( Result.EndLocation.SourceIndex   == 13 );
    REQUIRE( Source.StartLocation.SourceIndex == 14 );
    REQUIRE( Source.EndLocation.SourceIndex   == Source.Value.Num );
  }
}

TEST_CASE("Cfg: Parse simple document", "[Cfg]")
{
  test_allocator Allocator{};
  cfg_parsing_context Context{ "Cfg Test 5"_S, GlobalLog };

  cfg_document Document{};
  Init(&Document, &Allocator);
  Defer [&](){ Finalize(&Document); };

  SECTION("String value")
  {
    CfgDocumentParseFromString(&Document, "foo \"bar\""_S, &Context);

    auto Node = Document.Root->FirstChild;
    REQUIRE( Node != nullptr );
    REQUIRE( Node->Name == "foo"_S );
    REQUIRE( Node->Values.Num == 1 );
    REQUIRE( Node->Values[0].Type == cfg_literal_type::String );
    REQUIRE( Node->Values[0].String == "bar"_S );
  }

  SECTION("Number value")
  {
    CfgDocumentParseFromString(&Document, "answer 42"_S, &Context);

    auto Node = Document.Root->FirstChild;
    REQUIRE( Node != nullptr );
    REQUIRE( Node->Name == "answer"_S );
    REQUIRE( Node->Values.Num == 1 );
    REQUIRE( Node->Values[0].Type == cfg_literal_type::Number );
    REQUIRE( Convert<int>(Node->Values[0]) == 42 );
  }
}

TEST_CASE("Cfg: Parse simple document with attributes", "[Cfg]")
{
  test_allocator Allocator{};
  cfg_parsing_context Context{ "Cfg Test 6"_S, GlobalLog };

  cfg_document Document{};
  Init(&Document, &Allocator);
  Defer [&](){ Finalize(&Document); };

  CfgDocumentParseFromString(&Document, "foo \"bar\" baz=\"qux\""_S, &Context);

  auto Node = Document.Root->FirstChild;
  REQUIRE( Node != nullptr );
  REQUIRE( Node->Name == "foo"_S );
  REQUIRE( Node->Values.Num == 1 );
  REQUIRE( Node->Values[0].Type == cfg_literal_type::String );
  REQUIRE( Node->Values[0].String == "bar"_S );
  REQUIRE( Node->Attributes.Num == 1 );
  REQUIRE( Node->Attributes[0].Name == "baz"_S );
  REQUIRE( Node->Attributes[0].Value.Type == cfg_literal_type::String );
  REQUIRE( Node->Attributes[0].Value.String == "qux"_S );
}

TEST_CASE("Cfg: Parse simple document with multiple nodes", "[Cfg]")
{
  auto Source = "foo \"bar\"\n"
                "baz \"qux\"\n"_S;
  test_allocator Allocator{};
  cfg_parsing_context Context{ "Cfg Test 7"_S, GlobalLog };

  cfg_document Document{};
  Init(&Document, &Allocator);
  Defer [&](){ Finalize(&Document); };

  CfgDocumentParseFromString(&Document, Source, &Context);

  auto Node = Document.Root->FirstChild;
  REQUIRE( Node != nullptr );
  REQUIRE( Node->Name == "foo"_S );
  REQUIRE( Node->Values.Num == 1 );
  REQUIRE( Node->Values[0].Type == cfg_literal_type::String );
  REQUIRE( Node->Values[0].String == "bar"_S );

  Node = Node->Next;
  REQUIRE( Node != nullptr );
  REQUIRE( Node->Name == "baz"_S );
  REQUIRE( Node->Values.Num == 1 );
  REQUIRE( Node->Values[0].Type == cfg_literal_type::String );
  REQUIRE( Node->Values[0].String == "qux"_S );
}

TEST_CASE("Cfg: Parse simple document with child nodes", "[Cfg]")
{
  auto Source = "foo \"bar\" {\n"
                "  baz \"qux\" {\n"
                "    baaz \"quux\"\n"
                "  }\n"
                "}\n"_S;
  test_allocator Allocator{};
  cfg_parsing_context Context{ "Cfg Test 8"_S, GlobalLog };

  cfg_document Document{};
  Init(&Document, &Allocator);
  Defer [&](){ Finalize(&Document); };

  CfgDocumentParseFromString(&Document, Source, &Context);

  auto Node = Document.Root->FirstChild;
  REQUIRE( Node != nullptr );
  REQUIRE( Node->Name == "foo"_S );
  REQUIRE( Node->Values.Num == 1 );
  REQUIRE( Node->Values[0].Type == cfg_literal_type::String );
  REQUIRE( Node->Values[0].String == "bar"_S );

  Node = Node->FirstChild;
  REQUIRE( Node != nullptr );
  REQUIRE( Node->Name == "baz"_S );
  REQUIRE( Node->Values.Num == 1 );
  REQUIRE( Node->Values[0].Type == cfg_literal_type::String );
  REQUIRE( Node->Values[0].String == "qux"_S );

  Node = Node->FirstChild;
  REQUIRE( Node != nullptr );
  REQUIRE( Node->Name == "baaz"_S );
  REQUIRE( Node->Values.Num == 1 );
  REQUIRE( Node->Values[0].Type == cfg_literal_type::String );
  REQUIRE( Node->Values[0].String == "quux"_S );
}

TEST_CASE("Cfg: Parse document from file", "[Cfg]")
{
  test_allocator Allocator{};

  auto FileName = "../Tests/TestData/Full.cfg";

  array<uint8> FileContent{ &Allocator };
  if(!ReadFileContentIntoArray(FileContent, FileName))
  {
    FAIL( FileName << ": Unable to find file. Wrong working directory?" );
  }

  cfg_parsing_context Context{ SliceFromString(FileName), GlobalLog };

  cfg_document Document{};
  Init(&Document, &Allocator);
  Defer [&](){ Finalize(&Document); };

  CfgDocumentParseFromString(&Document, SliceReinterpret<char const>(Slice(FileContent)), &Context);

  //
  // The actual test
  //
  // foo "bar"
  auto Node = Document.Root->FirstChild;
  REQUIRE( Node != nullptr );
  REQUIRE( Node->Name == "foo"_S );
  REQUIRE( Node->Values.Num == 1 );
  REQUIRE( Convert<slice<char const>>(Node->Values[0]) == "bar"_S );
  REQUIRE( Node->Attributes.Num == 0 );

  // foo "bar" "baz"
  Node = Node->Next;
  REQUIRE( Node != nullptr );
  REQUIRE( Node->Name == "foo"_S );
  REQUIRE( Node->Values.Num == 2 );
  REQUIRE( Convert<slice<char const>>(Node->Values[0]) == "bar"_S );
  REQUIRE( Convert<slice<char const>>(Node->Values[1]) == "baz"_S );
  REQUIRE( Node->Attributes.Num == 0 );

  // foo "bar" baz="qux"
  Node = Node->Next;
  REQUIRE( Node != nullptr );
  REQUIRE( Node->Name == "foo"_S );
  REQUIRE( Node->Values.Num == 1 );
  REQUIRE( Convert<slice<char const>>(Node->Values[0]) == "bar"_S );
  REQUIRE( Node->Attributes.Num == 1 );
  REQUIRE( Node->Attributes[0].Name == "baz"_S );
  REQUIRE( Convert<slice<char const>>(Node->Attributes[0].Value) == "qux"_S );

  // foo "bar" baz="qux" baaz="quux"
  Node = Node->Next;
  REQUIRE( Node != nullptr );
  REQUIRE( Node->Name == "foo"_S );
  REQUIRE( Node->Values.Num == 1 );
  REQUIRE( Convert<slice<char const>>(Node->Values[0]) == "bar"_S );
  REQUIRE( Node->Attributes.Num == 2 );
  REQUIRE( Node->Attributes[0].Name == "baz"_S );
  REQUIRE( Convert<slice<char const>>(Node->Attributes[0].Value) == "qux"_S );
  REQUIRE( Node->Attributes[1].Name == "baaz"_S );
  REQUIRE( Convert<slice<char const>>(Node->Attributes[1].Value) == "quux"_S );

  // foo bar="baz"
  Node = Node->Next;
  REQUIRE( Node != nullptr );
  REQUIRE( Node->Name == "foo"_S );
  REQUIRE( Node->Values.Num == 0 );
  REQUIRE( Node->Attributes.Num == 1 );
  REQUIRE( Node->Attributes[0].Name == "bar"_S );
  REQUIRE( Convert<slice<char const>>(Node->Attributes[0].Value) == "baz"_S );

  // "foo"
  Node = Node->Next;
  REQUIRE( Node != nullptr );
  REQUIRE( Node->Name == ""_S );
  REQUIRE( Node->Values.Num == 1 );
  REQUIRE( Convert<slice<char const>>(Node->Values[0]) == "foo"_S );
  REQUIRE( Node->Attributes.Num == 0 );

  // "foo" bar="baz"
  Node = Node->Next;
  REQUIRE( Node != nullptr );
  REQUIRE( Node->Name == ""_S );
  REQUIRE( Node->Values.Num == 1 );
  REQUIRE( Convert<slice<char const>>(Node->Values[0]) == "foo"_S );
  REQUIRE( Node->Attributes.Num == 1 );
  REQUIRE( Node->Attributes[0].Name == "bar"_S );
  REQUIRE( Convert<slice<char const>>(Node->Attributes[0].Value) == "baz"_S );

  /*
    foo {
      baz "baz"
    }
  */
  Node = Node->Next;
  REQUIRE( Node != nullptr );
  REQUIRE( Node->Name == "foo"_S );
  REQUIRE( Node->Values.Num == 0 );
  REQUIRE( Node->Attributes.Num == 0 );
  {
    auto Child = Node->FirstChild;
    REQUIRE( Child != nullptr );
    REQUIRE( Child->Name == "bar"_S );
    REQUIRE( Child->Values.Num == 1 );
    REQUIRE( Convert<slice<char const>>(Child->Values[0]) == "baz"_S );
    REQUIRE( Child->Attributes.Num == 0 );
  }

  // foo /*
  // This is
  // what you get
  // when you support multi-line comments
  // in a whitespace sensitive language. */ answer=42
  Node = Node->Next;
  REQUIRE( Node != nullptr );
  REQUIRE( Node->Name == "foo"_S );
  REQUIRE( Node->Values.Num == 0 );
  REQUIRE( Node->Attributes.Num == 1 );
  REQUIRE( Node->Attributes[0].Name == "answer"_S );
  REQUIRE( Convert<int>(Node->Attributes[0].Value) == 42 );

  /*
    foo 1 2 "bar" baz="qux" {
      inner { 0 1 2 }
      "anon value"
      "anon value with nesting" {
        another-foo "bar" 1337 -92 "baz" qux="baaz"
      }
    }
  */
  Node = Node->Next;
  REQUIRE( Node != nullptr );
  REQUIRE( Node->Name == "foo"_S );
  REQUIRE( Node->Values.Num == 3 );
  REQUIRE( Convert<int>(Node->Values[0]) == 1 );
  REQUIRE( Convert<int>(Node->Values[1]) == 2 );
  REQUIRE( Convert<slice<char const>>(Node->Values[2]) == "bar"_S );
  REQUIRE( Node->Attributes.Num == 1 );
  REQUIRE( Node->Attributes[0].Name == "baz"_S );
  REQUIRE( Convert<slice<char const>>(Node->Attributes[0].Value) == "qux"_S );
  {
    // inner { 0 1 2 }
    auto Child = Node->FirstChild;
    REQUIRE( Child != nullptr );
    REQUIRE( Child->Name == "inner"_S );
    REQUIRE( Child->Values.Num == 0 );
    REQUIRE( Child->Attributes.Num == 0 );
    {
      auto ChildsChild = Child->FirstChild;
      REQUIRE( ChildsChild != nullptr );
      REQUIRE( ChildsChild->Values.Num == 3 );
      REQUIRE( Convert<int>(ChildsChild->Values[0]) == 0 );
      REQUIRE( Convert<int>(ChildsChild->Values[1]) == 1 );
      REQUIRE( Convert<int>(ChildsChild->Values[2]) == 2 );
      REQUIRE( ChildsChild->Attributes.Num == 0 );
    }

    // "anon value"
    Child = Child->Next;
    REQUIRE( Child != nullptr );
    REQUIRE( Child->Values.Num == 1 );
    REQUIRE( Convert<slice<char const>>(Child->Values[0]) == "anon value"_S );
    REQUIRE( Child->Attributes.Num == 0 );

    /*
      "anon value with nesting" {
        another-foo "bar" 1337 -92 "baz" qux="baaz"
      }
    */
    Child = Child->Next;
    REQUIRE( Child != nullptr );
    REQUIRE( Child->Name == ""_S );
    REQUIRE( Child->Values.Num == 1 );
    REQUIRE( Convert<slice<char const>>(Child->Values[0]) == "anon value with nesting"_S );
    REQUIRE( Child->Attributes.Num == 0 );
    {
      // another-foo "bar" 1337 -92 "baz" qux="baaz"
      auto ChildsChild = Child->FirstChild;
      REQUIRE( ChildsChild != nullptr );
      REQUIRE( ChildsChild->Name == "another-foo"_S );
      REQUIRE( ChildsChild->Values.Num == 4 );
      REQUIRE( Convert<slice<char const>>(ChildsChild->Values[0]) == "bar"_S );
      REQUIRE( Convert<int>(ChildsChild->Values[1]) == 1337 );
      REQUIRE( Convert<int>(ChildsChild->Values[2]) == -92 );
      REQUIRE( Convert<slice<char const>>(ChildsChild->Values[3]) == "baz"_S );
      REQUIRE( ChildsChild->Attributes.Num == 1 );
      REQUIRE( ChildsChild->Attributes[0].Name == "qux"_S );
      REQUIRE( Convert<slice<char const>>(ChildsChild->Attributes[0].Value) == "baaz"_S );
    }
  }

  REQUIRE( Node->Next == nullptr );
}

// Below are the unported unit tests from krepel.
#if 0

// Node iterator test
unittest
{
  auto TestAllocator = CreateTestAllocator!StackMemory();

  const SourceString = q"(
    foo "bar" {
      baz "qux" key="value" {
        baaz "quux" answer=42
      }
    }
  )";

  auto Document = TestAllocator.New!cfg_document(TestAllocator);
  scope(exit) TestAllocator.Delete(Document);

  cfg_parsing_context ParsingContext{ "Convenience", GlobalLog };
  Document.ParseDocumentFromString(SourceString, ParsingContext);

  REQUIRE( cast(string)Document.Root->Nodes["foo"][0].Values[0] == "bar"_S );
  REQUIRE( cast(string)Document.Root->Nodes["foo"][0].Nodes["baz"][0].Values[0] == "qux"_S );
  REQUIRE( cast(string)Document.Root->Nodes["foo"][0].Nodes["baz"][0].Nodes["baaz"][0].Values[0] == "quux"_S );
  REQUIRE( Document.Root->Nodes["baz"].Num == 0 );

  foreach(Node; Document.Root->Nodes)
  {
    REQUIRE( Convert<slice<char const>>(Node->Values[0]) == "bar"_S );
  }

  REQUIRE( !Document.Root->Nodes["foo"][0].Attribute("bar").IsValid );
  REQUIRE( cast(string)Document.Root->Nodes["foo"][0].Nodes["baz"][0].Attribute("key") == "value" );
  REQUIRE( cast(int)Document.Root->Nodes["foo"][0].Nodes["baz"][0].Nodes["baaz"][0].Attribute("answer") == 42 );
}

// Query
unittest
{
  auto TestAllocator = CreateTestAllocator!StackMemory();

  cfg_parsing_context Context{ "Query Test 1", GlobalLog };

  auto Document = TestAllocator.New!cfg_document(TestAllocator);
  auto Source = q"(
    Foo "Bar" "Bar?" "Bar!" Key="Value" {
      Baz "Qux" {
        Baaz "Quux" 1337 answer=42
        Baaz "Fuux" 123  answer=43
      }

      Baz "Sup" {
        Baaz "Quux, again"
      }
    }
  )";
  Document.ParseDocumentFromString(Source, Context);

  REQUIRE( Document.Query!string("Foo") == "Bar" );
  REQUIRE( Document.Query!string("Foo#0") == "Bar" );
  REQUIRE( Document.Query!string("Foo#1") == "Bar?" );
  REQUIRE( Document.Query!string("Foo#2") == "Bar!" );
  REQUIRE( Document.Query!string("Foo[0]") == "Bar" );
  REQUIRE( Document.Query!string("Foo[0]#0") == "Bar" );
  REQUIRE( Document.Query!string("Foo[0]#1") == "Bar?" );
  REQUIRE( Document.Query!string("Foo[0]#2") == "Bar!" );
  REQUIRE( Document.Query!string("Foo@Key") == "Value" );
  REQUIRE( Document.Query!string("Foo/Baz") == "Qux" );
  REQUIRE( Document.Query!string("Foo/Baz[1]") == "Sup" );
  REQUIRE( Document.Query!string("Foo/Baz/Baaz") == "Quux" );
  REQUIRE( Document.Query!string("Foo/Baz[1]/Baaz") == "Quux, again" );
  REQUIRE( Document.Query!int("Foo/Baz/Baaz#1") == 1337 );
  REQUIRE( Document.Query!int("Foo/Baz/Baaz@answer") == 42 );
  REQUIRE( Document.Query!string("Foo/Baz/Baaz[1]") == "Fuux" );
  REQUIRE( Document.Query!int("Foo/Baz/Baaz[1]#1") == 123 );
  REQUIRE( Document.Query!int("Foo/Baz/Baaz[1]@answer") == 43 );

  auto Foo = Document.Query!SDLNodeHandle("Foo");
  REQUIRE( Foo != nullptr );
  REQUIRE( Foo.Values.Num == 3 );
  REQUIRE( Foo.Attributes.Num == 1 );
  REQUIRE( cast(string)Foo.Attribute("Key") == "Value" );
}

#endif
