#include "CfgParser.hpp"

#include <Core/Log.hpp>


#define CfgSourceLogMessageDispatch(LogLevel, Context, Source, Message, ...) \
  LogMessageDispatch(LogLevel, \
                     (Context)->Log, \
                     "%*s(%u, %u): " Message, \
                     Convert<int>((Context)->Origin.Num), \
                     (Context)->Origin.Ptr, \
                     (Source).StartLocation.Line, \
                     (Source).StartLocation.Column, \
                     __VA_ARGS__)

#define CfgSourceLogBeginScope(Context, Source, Message, ...) CfgSourceLogMessageDispatch(log_level::ScopeBegin, Context, Source, Message, __VA_ARGS__)
#define CfgSourceLogEndScope(Context, Source, Message, ...)   CfgSourceLogMessageDispatch(log_level::ScopeEnd,   Context, Source, Message, __VA_ARGS__)
#define CfgSourceLogInfo(Context, Source, Message, ...)       CfgSourceLogMessageDispatch(log_level::Info,       Context, Source, Message, __VA_ARGS__)
#define CfgSourceLogWarning(Context, Source, Message, ...)    CfgSourceLogMessageDispatch(log_level::Warning,    Context, Source, Message, __VA_ARGS__)
#define CfgSourceLogError(Context, Source, Message, ...)      CfgSourceLogMessageDispatch(log_level::Error,      Context, Source, Message, __VA_ARGS__)


auto
::CfgSourceCurrentValue(cfg_source const& Source)
  -> slice<char const>
{
  return Slice(Source.Value, Source.StartLocation.SourceIndex, Source.EndLocation.SourceIndex);
}

auto
::CfgSourceCurrentChar(cfg_source const& Source)
  -> char
{
  return Source.Value[Source.StartLocation.SourceIndex];
}

auto
::CfgSourceAdvanceBy(cfg_source* Source, size_t N)
  -> cfg_source
{
  Assert(Source);
  BoundsCheck(N <= CfgSourceCurrentValue(*Source).Num);

  auto Copy = *Source;

  while(N)
  {
    if(CfgSourceCurrentChar(*Source) == '\n')
    {
      Source->StartLocation.Column = 1;
      Source->StartLocation.Line++;
    }
    else
    {
      Source->StartLocation.Column += 1;
    }

    Source->StartLocation.SourceIndex++;
    --N;
  }

  Copy.EndLocation = Source->StartLocation;
  return Copy;
}


auto
::CfgSourceIsAtWhiteSpace(cfg_source const& Source, cfg_parsing_context* Context)
  -> bool
{
  auto String = CfgSourceCurrentValue(Source);
  return String.Num && IsWhitespace(String[0]);
}

auto
::CfgSourceIsAtNewLine(cfg_source const& Source, cfg_parsing_context* Context)
  -> bool
{
  auto String = CfgSourceCurrentValue(Source);
  return String.Num && SliceStartsWith(String, SliceFromString("\n"));
}

auto
::CfgSourceSkipWhiteSpace(cfg_source* OriginalSource, cfg_parsing_context* Context, cfg_consume_newline ConsumeNewLine)
  -> cfg_source
{
  cfg_source Source = *OriginalSource;

  while(CfgSourceIsAtWhiteSpace(Source, Context))
  {
    if(ConsumeNewLine == cfg_consume_newline::No && CfgSourceIsAtNewLine(Source, Context))
      break;

    CfgSourceAdvanceBy(&Source, 1);
  }

  auto NumToAdvance = Source.StartLocation.SourceIndex - OriginalSource->StartLocation.SourceIndex;
  return CfgSourceAdvanceBy(OriginalSource, NumToAdvance);
}

auto
::CfgSourceIsAtLineComment(cfg_source const& Source, cfg_parsing_context* Context)
  -> bool
{
  auto String = CfgSourceCurrentValue(Source);
  return SliceStartsWith(String, SliceFromString("//")) ||
         SliceStartsWith(String, SliceFromString("#")) ||
         SliceStartsWith(String, SliceFromString("--"));
}

auto
::CfgSourceIsAtMultiLineComment(cfg_source const& Source, cfg_parsing_context* Context)
  -> bool
{
  auto String = CfgSourceCurrentValue(Source);
  return SliceStartsWith(String, SliceFromString("/*"));
}

auto
::CfgSourceIsAtComment(cfg_source const& Source, cfg_parsing_context* Context)
  -> bool
{
  return CfgSourceIsAtLineComment(Source, Context) || CfgSourceIsAtMultiLineComment(Source, Context);
}

auto
::CfgSourceSkipComments(cfg_source* OriginalSource, cfg_parsing_context* Context)
  -> cfg_source
{
  auto Source = *OriginalSource;

  while(true)
  {
    auto String = CfgSourceCurrentValue(Source);
    if(CfgSourceIsAtLineComment(Source, Context))
    {
      auto NumToSkip = String.Num - SliceFind(String, SliceFromString("\n")).Num;
      CfgSourceAdvanceBy(&Source, Min(NumToSkip + 1, String.Num));
    }
    else if(CfgSourceIsAtMultiLineComment(Source, Context))
    {
      auto NumToSkip = String.Num - SliceFind(String, SliceFromString("*/")).Num;
      CfgSourceAdvanceBy(&Source, Min(NumToSkip + 2, String.Num));
    }
    else
    {
      break;
    }
  }

  auto NumToSkip = Source.StartLocation.SourceIndex - OriginalSource->StartLocation.SourceIndex;
  return CfgSourceAdvanceBy(OriginalSource, NumToSkip);
}

auto
::CfgSourceSkipWhiteSpaceAndComments(cfg_source* OriginalSource, cfg_parsing_context* Context, cfg_consume_newline ConsumeNewLine)
  -> cfg_source
{
  auto Source = *OriginalSource;

  while(true)
  {
    if(CfgSourceIsAtWhiteSpace(Source, Context))
    {
      if(ConsumeNewLine == cfg_consume_newline::No && CfgSourceIsAtNewLine(Source, Context)) break;
      CfgSourceSkipWhiteSpace(&Source, Context, ConsumeNewLine);
    }
    else if(CfgSourceIsAtComment(Source, Context))
    {
      CfgSourceSkipComments(&Source, Context);
    }
    else
    {
      break;
    }
  }

  auto NumToSkip = Source.StartLocation.SourceIndex - OriginalSource->StartLocation.SourceIndex;
  return CfgSourceAdvanceBy(OriginalSource, NumToSkip);
}

/// Basically a new-line character or a semi-colon
auto
::CfgSourceIsAtSemanticLineDelimiter(cfg_source const& Source, cfg_parsing_context* Context)
  -> bool
{
  auto String = CfgSourceCurrentValue(Source);
  if(String.Num == 0)
    return true;

  return String[0] == ';' || CfgSourceIsAtNewLine(Source, Context);
}

auto
::CfgSourceParseNested(cfg_source* Source, cfg_parsing_context* Context,
                       slice<char const> OpeningSequence, slice<char const> ClosingSequence,
                       bool* OutFoundClosingSequence, int Depth)
  -> cfg_source
{
  auto const SourceString = CfgSourceCurrentValue(*Source);
  size_t const MaxNum = SourceString.Num;
  size_t NumToAdvance = 0;
  slice<char const> String{};
  while(NumToAdvance < MaxNum)
  {
    String = SliceTrimFront(SourceString, NumToAdvance);
    if(SliceStartsWith(String, OpeningSequence))
    {
      NumToAdvance += OpeningSequence.Num;
      Depth++;
    }
    else if(SliceStartsWith(String, ClosingSequence))
    {
      Depth--;
      if(Depth <= 0)
        break;

      NumToAdvance += ClosingSequence.Num;
    }
    else
    {
      NumToAdvance++;
    }
  }

  // It's possible the Source was exhausted before we found a closing sequence.
  if(OutFoundClosingSequence)
    *OutFoundClosingSequence = SliceStartsWith(String, ClosingSequence);

  auto Result = CfgSourceAdvanceBy(Source, NumToAdvance);
  auto CurrentSourceString = CfgSourceCurrentValue(*Source);
  CfgSourceAdvanceBy(Source, Min(CurrentSourceString.Num, ClosingSequence.Num));
  return Result;
}

auto
::CfgSourceParseEscaped(cfg_source* Source, cfg_parsing_context* Context,
                        char EscapeDelimiter, slice<char const> DelimiterSequence,
                        cfg_consume_newline ConsumeNewLine)
  -> cfg_source
{
  auto const SourceString = CfgSourceCurrentValue(*Source);
  size_t const MaxNum = SourceString.Num;
  size_t NumToAdvance = 0;

  while(NumToAdvance < MaxNum)
  {
    auto String = SliceTrimFront(SourceString, NumToAdvance);
    if(String[0] == EscapeDelimiter)
    {
      // TODO(Manu): Resolve escaped char. Skip for now.
      NumToAdvance = Min(NumToAdvance + 2, MaxNum);
    }
    else if(ConsumeNewLine == cfg_consume_newline::No && String[0] == '\n' ||
            SliceStartsWith(String, DelimiterSequence))
    {
      break;
    }
    else
    {
      NumToAdvance++;
    }
  }

  auto Result = CfgSourceAdvanceBy(Source, NumToAdvance);
  auto CurrentSourceString = CfgSourceCurrentValue(*Source);
  CfgSourceAdvanceBy(Source, Min(CurrentSourceString.Num, DelimiterSequence.Num));
  return Result;
}

auto
::CfgDocumentParseFromString(cfg_document* Document, slice<char const> SourceString, cfg_parsing_context* Context)
  -> bool
{
  cfg_source Source;
  Source.Value = SourceString;
  Source.StartLocation = { 1, 1,               0  };
  Source.EndLocation =   { 0, 0, SourceString.Num };

  return CfgDocumentParseFromSource(Document, &Source, Context);
}

auto
::CfgDocumentParseFromSource(cfg_document* Document, cfg_source* Source, cfg_parsing_context* Context)
  -> bool
{
  cfg_node_handle FirstChildOfRoot;
  auto Success = CfgDocumentParseInnerNodes(Document, Source, Context, &FirstChildOfRoot);
  if(Success)
  {
    auto RootNode = CfgBeginNodeAccess(Document, Document->Root);
    RootNode->FirstChild = FirstChildOfRoot;
    CfgEndNodeAccess(Document, RootNode);
    return true;
  }

  return false;
}

auto
::CfgDocumentParseInnerNodes(cfg_document* Document, cfg_source* Source, cfg_parsing_context* Context,
                             cfg_node_handle* FirstNode)
  -> bool
{
  if(FirstNode == nullptr)
  {
    LogWarning(Context->Log, "Passed a nullptr to CfgDocumentParseInnerNodes.");
    return false;
  }

  if(!CfgDocumentParseNode(Document, Source, Context, FirstNode))
  {
    return false;
  }

  cfg_node_handle PreviousNodeHandle = *FirstNode;
  cfg_node_handle NewNodeHandle;
  while(CfgDocumentParseNode(Document, Source, Context, &NewNodeHandle))
  {
    {
      auto PreviousNode = CfgBeginNodeAccess(Document, PreviousNodeHandle);
      PreviousNode->Next = NewNodeHandle;
      CfgEndNodeAccess(Document, PreviousNode);
    }
    {
      auto NewNode = CfgBeginNodeAccess(Document, NewNodeHandle);
      NewNode->Previous = PreviousNodeHandle;
      CfgEndNodeAccess(Document, NewNode);
    }
    PreviousNodeHandle = NewNodeHandle;
  }
  return true;
}

auto
::CfgDocumentParseIdentifier(cfg_document* Document, cfg_source* OriginalSource, cfg_parsing_context* Context,
                             cfg_identifier* Result)
  -> bool
{
  auto Source = *OriginalSource;
  CfgSourceSkipWhiteSpaceAndComments(&Source, Context, cfg_consume_newline::No);

  auto String = CfgSourceCurrentValue(Source);
  if(String.Num == 0 || CfgSourceIsAtSemanticLineDelimiter(Source, Context))
    return false;

  if(!CfgIsValidIdentifierFirstChar(String[0]))
  {
    CfgSourceLogWarning(Context, Source, "Invalid identifier ([A-z_][A-z0-9\\-_.$]*)");
    return false;
  }

  String = SliceTrimFront(String, 1);
  size_t Count = 1; // 1 because the first character is valid and already consumed.

  while(String.Num && CfgIsValidIdentifierMiddleChar(String[0]))
  {
    String = SliceTrimFront(String, 1);
    Count++;
  }

  auto IdentifierSource = CfgSourceAdvanceBy(&Source, Count);

  if(Result)
    Result->Value = CfgSourceCurrentValue(IdentifierSource);

  *OriginalSource = Source;
  return true;
}

auto
::CfgDocumentParseNode(cfg_document* Document, cfg_source* OriginalSource, cfg_parsing_context* Context,
                       cfg_node_handle* OutNode)
  -> bool
{
  auto Source = *OriginalSource;
  CfgSourceSkipWhiteSpaceAndComments(&Source, Context, cfg_consume_newline::Yes);

  auto SourceString = CfgSourceCurrentValue(Source);
  if(SourceString.Num == 0)
    return false;

  auto NodeHandle = CfgCreateNode(Document);
  auto Node = CfgBeginNodeAccess(Document, NodeHandle);

  //
  // Parse Node Name and Namespace
  //
  if(!CfgDocumentParseName(Document, &Source, Context, &Node->Name))
  {
    Node->Name.Value = SliceFromString("");
  }

  //Source.SkipWhiteSpaceAndComments(Context, cfg_consume_newline::No);

  //
  // Protect against anonymous nodes that only contain attributes.
  //
  SourceString = CfgSourceCurrentValue(Source);
  if(SourceString.Num && SourceString[0] == '=')
  {
    CfgSourceLogWarning(Context, Source, "Anonymous node must have at least 1 value. "
                                         "It appears you've only given it attributes.");
    return false;
  }

  //
  // Parsing Values
  //
  while(true)
  {
    cfg_literal Value;
    if(!CfgDocumentParseLiteral(Document, &Source, Context, &Value))
      break;

    Expand(&Node->Values) = Value;
  }

  //
  // Parsing Attributes
  //
  while(true)
  {
    cfg_attribute Attribute;

    if(!CfgDocumentParseAttribute(Document, &Source, Context, &Attribute))
    {
      // There are no more attributes.
      break;
    }

    Expand(&Node->Attributes) = Attribute;
  }

  // Check for validity by trying to parse a literal here. If it succeeds, the
  // document is malformed.
  {
    auto SourceCopy = Source;
    if(CfgDocumentParseLiteral(Document, &SourceCopy, Context, nullptr))
    {
      CfgSourceLogWarning(Context, Source, "Unexpected literal");
      return false;
    }
  }

  // CfgSourceSkipWhiteSpaceAndComments(&Source, Context, cfg_consume_newline::Yes);
  CfgSourceSkipWhiteSpaceAndComments(&Source, Context, cfg_consume_newline::No);

  CfgEndNodeAccess(Document, Node);

  SourceString = CfgSourceCurrentValue(Source);
  if(SourceString.Num && SourceString[0] == '{')
  {
    CfgSourceAdvanceBy(&Source, 1);
    bool FoundClosingBrace;
    auto ChildSource = CfgSourceParseNested(&Source, Context,
                                            SliceFromString("{"), SliceFromString("}"),
                                            &FoundClosingBrace);

    if(!FoundClosingBrace)
    {
      CfgSourceLogWarning(Context, Source, "The list of child nodes is not closed properly with curly braces.");
    }

    cfg_node_handle FirstChild;
    if(CfgDocumentParseInnerNodes(Document, &ChildSource, Context, &FirstChild))
    {
      Node = CfgBeginNodeAccess(Document, NodeHandle);
      Node->FirstChild = FirstChild;
      CfgEndNodeAccess(Document, Node);
    }
    else
    {
      // TODO: What to do if there are no inner nodes? Ignore it?
    }
  }

  if(OutNode)
    *OutNode = NodeHandle;

  *OriginalSource = Source;
  return true;
}

auto
::CfgDocumentParseLiteral(cfg_document* Document, cfg_source* OriginalSource, cfg_parsing_context* Context,
                          cfg_literal* OutLiteral)
  -> bool
{
  auto Source = *OriginalSource;
  CfgSourceSkipWhiteSpaceAndComments(&Source, Context, cfg_consume_newline::No);

  auto SourceString = CfgSourceCurrentValue(Source);
  if(SourceString.Num == 0 || CfgSourceIsAtSemanticLineDelimiter(Source, Context))
    return false;

  cfg_literal Result;

  char CurrentChar = SourceString[0];
  if(CurrentChar == '"')
  {
    CfgSourceAdvanceBy(&Source, 1);
    auto StringSource = CfgSourceParseEscaped(&Source, Context,
                                              '\\', SliceFromString("\""),
                                              cfg_consume_newline::No);

    Result.Type = cfg_literal_type::String;
    Result.String = CfgSourceCurrentValue(StringSource);
  }
  else if(CurrentChar == '`')
  {
    CfgSourceAdvanceBy(&Source, 1);
    auto StringSource = CfgSourceParseUntil(&Source, Context,
                                            [](slice<char const> Str) { return Str[0] == '`'; });

    Result.Type = cfg_literal_type::String;
    Result.String = CfgSourceCurrentValue(StringSource);
  }
  else if(CurrentChar == '[')
  {
    // TODO: Result.Binary = ???;
    CfgSourceLogWarning(Context, Source, "Binary values are not supported right now.");

    CfgSourceAdvanceBy(&Source, 1);
    auto StringSource = CfgSourceParseUntil(&Source, Context,
                                            [](slice<char const> Str){ return Str[0] == ']'; });

    Result.Type = cfg_literal_type::Binary;
    Result.Binary = nullptr;
  }
  else
  {
    auto WordSource = CfgSourceParseUntil(&Source, Context,
                                          [](slice<char const> Str){ return IsWhitespace(Str[0]); });
    auto Word = CfgSourceCurrentValue(WordSource);

    if(Word.Num == 0)
    {
      CfgSourceLogWarning(Context, Source, "Unexpected end of file.");
      return false;
    }

    if(IsDigit(Word[0]) || Word[0] == '.' || Word[0] == '+' || Word[0] == '-')
    {
      Result.Type = cfg_literal_type::Number;
      Result.NumberSource = Word;
    }
    else if(Word == SliceFromString("true") || Word == SliceFromString("on") || Word == SliceFromString("yes"))
    {
      Result.Type = cfg_literal_type::Boolean;
      Result.Boolean = true;
    }
    else if(Word == SliceFromString("false") || Word == SliceFromString("off") || Word == SliceFromString("no"))
    {
      Result.Type = cfg_literal_type::Boolean;
      Result.Boolean = false;
    }
    else
    {
      CfgSourceLogWarning(Context, Source, "Unable to parse value.");
      return false;
    }
  }

  if(OutLiteral)
    *OutLiteral = Result;

  *OriginalSource = Source;
  return true;
}

auto
::CfgDocumentParseAttribute(cfg_document* Document, cfg_source* OriginalSource, cfg_parsing_context* Context,
                            cfg_attribute* OutAttribute)
  -> bool
{
  auto Source = *OriginalSource;
  cfg_attribute Result;

  //
  // Parse the namespace and the name
  //
  if(!CfgDocumentParseName(Document, &Source, Context, &Result.Name))
  {
    return false;
  }

  //Source.SkipWhiteSpaceAndComments(Context, cfg_consume_newline::No);

  auto SourceString = CfgSourceCurrentValue(Source);
  if(SourceString.Num == 0 || CfgSourceIsAtSemanticLineDelimiter(Source, Context))
  {
    goto MalformedAttribute;
  }

  if(SourceString[0] != '=')
  {
    CfgSourceLogWarning(Context, Source, "Expected a '=' as the attribute's "
                                         "key-value delimiter here.");
    return false;
  }

  // Skip the '=' character.
  CfgSourceAdvanceBy(&Source, 1);

  //Source.SkipWhiteSpaceAndComments(Context, cfg_consume_newline::No);

  SourceString = CfgSourceCurrentValue(Source);
  if(SourceString.Num == 0 || CfgSourceIsAtSemanticLineDelimiter(Source, Context))
  {
    goto MalformedAttribute;
  }

  //
  // Parse the value
  //
  if(!CfgDocumentParseLiteral(Document, &Source, Context, &Result.Value))
  {
    goto MalformedAttribute;
  }

  if(OutAttribute)
    *OutAttribute = Result;

  *OriginalSource = Source;
  return true;

  MalformedAttribute:
  {
    CfgSourceLogWarning(Context, Source, "Malformed attribute.");
    return false;
  }
}

auto
::CfgDocumentParseName(cfg_document* Document, cfg_source* OriginalSource, cfg_parsing_context* Context,
                       cfg_identifier* OutName)
  -> bool
{
  auto Source = *OriginalSource;

  cfg_identifier Identifier;
  if(!CfgDocumentParseIdentifier(Document, &Source, Context, &Identifier))
  {
    // TODO(Manu): Logging.
    CfgSourceLogWarning(Context, Source, "Expected a name here (literal).");
    return false;
  }

  if(OutName)
    *OutName = Identifier;

  *OriginalSource = Source;
  return true;
}

#undef CfgSourceLogMessageDispatch
#undef CfgSourceLogBeginScope
#undef CfgSourceLogEndScope
#undef CfgSourceLogInfo
#undef CfgSourceLogWarning
#undef CfgSourceLogError
