#pragma once

#include "Cfg.hpp"

struct log_data;


struct cfg_source_location
{
  size_t Line;        // 1-based.
  size_t Column;      // 1-based.
  size_t SourceIndex; // 0-based.
};

struct cfg_source
{
  slice<char const> Value;
  cfg_source_location StartLocation;
  cfg_source_location EndLocation; // Exclusive
};

struct cfg_parsing_context
{
  /// An identifier for the string source, e.g. a file name. Used in log
  /// messages.
  slice<char const> Origin;

  log_data* Log;
};

enum class cfg_consume_newline : bool { No = false, Yes = true };


//
// Source Functions
//

CFG_API slice<char const>
CfgSourceCurrentValue(cfg_source const& Source);

CFG_API char
CfgSourceCurrentChar(cfg_source const& Source);

CFG_API cfg_source
CfgSourceAdvanceBy(cfg_source* Source, size_t N);

CFG_API bool
CfgSourceIsAtWhiteSpace(cfg_source const& Source, cfg_parsing_context* Context);

CFG_API bool
CfgSourceIsAtNewLine(cfg_source const& Source, cfg_parsing_context* Context);

CFG_API cfg_source
CfgSourceSkipWhiteSpace(cfg_source* OriginalSource, cfg_parsing_context* Context, cfg_consume_newline ConsumeNewLine);

CFG_API bool
CfgSourceIsAtLineComment(cfg_source const& Source, cfg_parsing_context* Context);

CFG_API bool
CfgSourceIsAtMultiLineComment(cfg_source const& Source, cfg_parsing_context* Context);

CFG_API bool
CfgSourceIsAtComment(cfg_source const& Source, cfg_parsing_context* Context);

CFG_API cfg_source
CfgSourceSkipComments(cfg_source* OriginalSource, cfg_parsing_context* Context);

CFG_API cfg_source
CfgSourceSkipWhiteSpaceAndComments(cfg_source* OriginalSource, cfg_parsing_context* Context, cfg_consume_newline ConsumeNewLine);

CFG_API bool
CfgSourceIsAtSemanticLineDelimiter(cfg_source const& Source, cfg_parsing_context* Context);

template<typename PredicateType>
cfg_source
CfgSourceParseUntil(cfg_source* Source, cfg_parsing_context* Context, PredicateType Predicate)
{
  auto const SourceString = CfgSourceCurrentValue(*Source);
  size_t const MaxNum = SourceString.Num;
  size_t NumToAdvance = 0;
  while(NumToAdvance < MaxNum)
  {
    auto String = SliceTrimFront(SourceString, NumToAdvance);
    if(Predicate(String))
    {
      break;
    }

    ++NumToAdvance;
  }

  return CfgSourceAdvanceBy(Source, NumToAdvance);
}

CFG_API cfg_source
CfgSourceParseNested(cfg_source* Source, cfg_parsing_context* Context,
                     slice<char const> OpeningSequence, slice<char const> ClosingSequence,
                     bool* OutFoundClosingSequence = nullptr, int Depth = 1);

CFG_API cfg_source
CfgSourceParseEscaped(cfg_source* Source, cfg_parsing_context* Context,
                      char EscapeDelimiter, slice<char const> DelimiterSequence,
                      cfg_consume_newline ConsumeNewLine);


//
// Document Parse Functions
//

/// Convenience overload to accept a plain string instead of cfg_source.
CFG_API bool
CfgDocumentParseFromString(cfg_document& Document, slice<char const> SourceString, cfg_parsing_context* Context);

CFG_API bool
CfgDocumentParseFromSource(cfg_document& Document, cfg_source* Source, cfg_parsing_context* Context);

CFG_API bool
CfgDocumentParseInnerNodes(cfg_document& Document, cfg_source* Source, cfg_parsing_context* Context,
                           cfg_node** FirstNode);

constexpr bool
CfgIsValidIdentifierFirstChar(char Char)
{
  return (Char >= 'A' && Char <= 'Z') ||
         (Char >= 'a' && Char <= 'z') ||
         Char == '_';
}

constexpr bool
CfgIsValidIdentifierMiddleChar(char Char)
{
  return CfgIsValidIdentifierFirstChar(Char) ||
         IsDigit(Char) ||
         Char == '-' ||
         Char == '.' ||
         Char == '$';
}

CFG_API bool
CfgDocumentParseIdentifier(cfg_document& Document, cfg_source* OriginalSource, cfg_parsing_context* Context,
                           cfg_identifier* Result);

CFG_API bool
CfgDocumentParseNode(cfg_document& Document, cfg_source* OriginalSource, cfg_parsing_context* Context,
                     cfg_node** OutNode);


CFG_API bool
CfgDocumentParseIdentifier(cfg_document& Document, cfg_source* OriginalSource, cfg_parsing_context* Context,
                           cfg_identifier* Result);

CFG_API bool
CfgDocumentParseLiteral(cfg_document& Document, cfg_source* OriginalSource, cfg_parsing_context* Context,
                        cfg_literal* OutLiteral);

CFG_API bool
CfgDocumentParseAttribute(cfg_document& Document, cfg_source* OriginalSource, cfg_parsing_context* Context,
                          cfg_attribute* OutAttribute);

CFG_API bool
CfgDocumentParseName(cfg_document& Document, cfg_source* OriginalSource, cfg_parsing_context* Context,
                     cfg_identifier* OutName);
