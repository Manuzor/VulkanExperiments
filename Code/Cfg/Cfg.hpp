#pragma once

#include "CfgAPI.hpp"

#include <Core/Allocator.hpp>
#include <Core/DynamicArray.hpp>


struct cfg_document;

struct cfg_identifier
{
  slice<char const> Value;
};

inline bool operator==(cfg_identifier const& Identifier, slice<char const> Slice) { return Identifier.Value == Slice; }
inline bool operator==(slice<char const> Slice, cfg_identifier const& Identifier) { return Slice == Identifier.Value; }
inline bool operator!=(cfg_identifier const& Identifier, slice<char const> Slice) { return !(Identifier == Slice); }
inline bool operator!=(slice<char const> Slice, cfg_identifier const& Identifier) { return !(Slice == Identifier); }


enum class cfg_literal_type
{
  INVALID,

  String,
  Number,
  Boolean,
  Binary
};

struct cfg_literal
{
  cfg_literal_type Type;
  union
  {
    slice<char const> String;
    slice<char const> NumberSource;
    bool Boolean;
    void* Binary; // TODO: Support this?
  };
};

struct cfg_attribute
{
  cfg_identifier Name;
  cfg_literal Value;
};

DefineOpaqueHandle(cfg_node_handle);

struct cfg_node
{
  cfg_document* Document;

  cfg_node_handle Next;
  cfg_node_handle Previous;
  cfg_node_handle Parent;
  cfg_node_handle FirstChild;

  cfg_identifier Name;

  dynamic_array<cfg_literal> Values;
  dynamic_array<cfg_attribute> Attributes;
};

// TODO: Memory management overhaul in cfg_documents.
struct cfg_document
{
  allocator_interface* Allocator;

  // This is where all nodes associated with this document live.
  dynamic_array<cfg_node> AllNodes;

  // The root node of this document. The node itself does not contain any
  // data, it merely serves as access point to the actual document data (the
  // children of this node).
  cfg_node_handle Root;

  // Counter to track CfgBeginNodeAccess and CfgEndNodeAccess calls.
  int NumExternalNodePtrs;
};

CFG_API void
Init(cfg_document* Document, allocator_interface* Allocator);

CFG_API void
Finalize(cfg_document* Document);

CFG_API cfg_node_handle
CfgCreateNode(cfg_document* Document);

inline bool
CfgLiteralIsValid(cfg_literal const& Literal)
{
  return Literal.Type != cfg_literal_type::INVALID;
}

CFG_API bool
CfgIdentifierMatchesFilter(cfg_identifier Identifier, slice<char const> Filter);

CFG_API cfg_node*
CfgBeginNodeAccess(cfg_document* Document, cfg_node_handle Handle);

CFG_API void
CfgEndNodeAccess(cfg_document* Document, cfg_node* Node);


//
// Conversion of Cfg literal
//

template<>
struct impl_convert<slice<char const>, cfg_literal>
{
  static inline slice<char const>
  Do(cfg_literal const& Literal, bool* Success = nullptr)
  {
    if(Literal.Type != cfg_literal_type::String)
    {
      if(Success)
        *Success = false;
      return {};
    }
    return Literal.String;
  }
};

template<>
struct impl_convert<bool, cfg_literal>
{
  static inline bool
  Do(cfg_literal const& Literal, bool* Success = nullptr)
  {
    if(Literal.Type != cfg_literal_type::Boolean)
    {
      if(Success)
        *Success = false;
      return {};
    }
    return Literal.Boolean;
  }
};

template<typename NumberType>
struct impl_convert_cfg_literal_number
{
  static inline NumberType
  Do(cfg_literal const& Literal, bool* Success = nullptr)
  {
    if(Literal.Type != cfg_literal_type::Number)
    {
      if(Success)
        *Success = false;
      return {};
    }
    return Convert<NumberType>(Literal.NumberSource, Success);
  }
};

template<> struct impl_convert<float,  cfg_literal> : public impl_convert_cfg_literal_number<float>  {};
template<> struct impl_convert<double, cfg_literal> : public impl_convert_cfg_literal_number<double> {};
template<> struct impl_convert<int8,   cfg_literal> : public impl_convert_cfg_literal_number<int8>   {};
template<> struct impl_convert<int16,  cfg_literal> : public impl_convert_cfg_literal_number<int16>  {};
template<> struct impl_convert<int32,  cfg_literal> : public impl_convert_cfg_literal_number<int32>  {};
template<> struct impl_convert<int64,  cfg_literal> : public impl_convert_cfg_literal_number<int64>  {};
template<> struct impl_convert<uint8,  cfg_literal> : public impl_convert_cfg_literal_number<uint8>  {};
template<> struct impl_convert<uint16, cfg_literal> : public impl_convert_cfg_literal_number<uint16> {};
template<> struct impl_convert<uint32, cfg_literal> : public impl_convert_cfg_literal_number<uint32> {};
template<> struct impl_convert<uint64, cfg_literal> : public impl_convert_cfg_literal_number<uint64> {};


//
// Node accessor functions
//

CFG_API cfg_node_handle
CfgNodeNext(cfg_document* Document, cfg_node_handle NodeHandle);

CFG_API cfg_node_handle
CfgNodePrevious(cfg_document* Document, cfg_node_handle NodeHandle);

CFG_API cfg_node_handle
CfgNodeParent(cfg_document* Document, cfg_node_handle NodeHandle);

CFG_API cfg_node_handle
CfgNodeFirstChild(cfg_document* Document, cfg_node_handle NodeHandle);

CFG_API cfg_identifier
CfgNodeName(cfg_document* Document, cfg_node_handle NodeHandle);

CFG_API slice<cfg_literal>
CfgNodeValues(cfg_document* Document, cfg_node_handle NodeHandle);

CFG_API slice<cfg_attribute>
CfgNodeAttributes(cfg_document* Document, cfg_node_handle NodeHandle);
