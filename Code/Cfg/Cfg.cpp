#include "Cfg.hpp"

#include <Core/Log.hpp>


auto
::Init(cfg_document* Document, allocator_interface* Allocator)
  -> void
{
  Init(&Document->AllNodes, Allocator);
  Reserve(&Document->AllNodes, 32);
  Document->Root = CfgCreateNode(Document);
}

auto
::Finalize(cfg_document* Document)
  -> void
{
  Finalize(&Document->AllNodes);
}

static cfg_node_handle
CreateHandleFromIndex(size_t Index)
{
  return Reinterpret<cfg_node_handle>(Index + 1);
}

static size_t
GetIndexFromHandle(cfg_node_handle Handle)
{
  return Reinterpret<size_t>(Handle) - 1;
}

auto
::CfgCreateNode(cfg_document* Document)
  -> cfg_node_handle
{
  if(Document->NumExternalNodePtrs > 0)
  {
    LogError("Unable to create node while nodes are still accessed externally by raw pointers. "
             "Did you forget to call `CfgEndNodeAccess()`?");
    return nullptr;
  }

  // Create a new handle and fill it with proper data.
  cfg_node_handle Handle{ CreateHandleFromIndex(Document->AllNodes.Num) };

  // Create a new node by expanding the current list of nodes and initialize
  // that node.
  auto NewNode = &Expand(&Document->AllNodes);
  NewNode->Document = Document;
  Init(&NewNode->Values, Document->Allocator);
  Init(&NewNode->Attributes, Document->Allocator);

  // Return the handle to the new node.
  return Handle;
}

auto
::CfgIdentifierMatchesFilter(cfg_identifier Identifier, slice<char const> Filter)
  -> bool
{
  if(Filter.Num == 0)
    return true;

  // TODO: Allow for some kind of patterns.
  return Identifier.Value == Filter;
}

auto
::CfgBeginNodeAccess(cfg_document* Document, cfg_node_handle Handle)
  -> cfg_node*
{
  if(Handle == nullptr)
    return nullptr;
  ++Document->NumExternalNodePtrs;
  return &Document->AllNodes[GetIndexFromHandle(Handle)];
}

auto
::CfgEndNodeAccess(cfg_document* Document, cfg_node* Node)
  -> void
{
  if(Document->NumExternalNodePtrs > 0)
    --Document->NumExternalNodePtrs;
  else
    LogWarning("CfgEndNodeAccess() was called but NumExternalNodePtrs was 0.");
}
