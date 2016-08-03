#include "Cfg.hpp"

#include <Core/Log.hpp>


auto
::Init(cfg_document* Document, allocator_interface* Allocator)
  -> void
{
  Document->Allocator = Allocator;
  Init(&Document->Nodes, Allocator);
  Reserve(&Document->Nodes, 32);
  Document->Root = CfgCreateNode(Document);
}

auto
::Finalize(cfg_document* Document)
  -> void
{
  for(auto Node : Slice(&Document->Nodes))
  {
    CfgDestroyNode(Document, Node);
  }
  Finalize(&Document->Nodes);
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
::CfgCreateNode(cfg_document* Document)
  -> cfg_node*
{
  auto Node = Allocate<cfg_node>(Document->Allocator);
  *Node = {};
  Node->Document = Document;
  Init(&Node->Values, Document->Allocator);
  Init(&Node->Attributes, Document->Allocator);
  Expand(&Document->Nodes) = Node;
  return Node;
}

auto
::CfgDestroyNode(cfg_document* Document, cfg_node* Node)
  -> void
{
  if(Node == nullptr)
    return;

  if(RemoveFirst(&Document->Nodes, Node))
  {
    Finalize(&Node->Values);
    Finalize(&Node->Attributes);
    Deallocate(Document->Allocator, Node);
  }
  else
  {
    LogWarning("Attempt to destroy node in this document that does not belong in it.");
  }
}
