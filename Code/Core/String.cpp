
#include "String.hpp"

static char MutableEmptyString[] = { '\0' };

static allocator_interface*
GetDefaultStringAllocator()
{
  static mallocator Allocator;
  return &Allocator;
}

static arc_string::internal*
CreateInternal(allocator_interface& Allocator)
{
  auto Internal = Allocate<arc_string::internal>(Allocator);
  Assert(Internal);
  MemConstruct(1, Internal);
  Internal->Data.Allocator = &Allocator;
  Internal->RefCount = 1;

  return Internal;
}

static void
DestroyInternal(allocator_interface& Allocator, arc_string::internal* Internal)
{
  if(Internal)
  {
    MemDestruct(1, Internal);
    Deallocate(Allocator, Internal);
  }
}

static bool
StrIsInitialized(arc_string const& String)
{
  return String.Internal != nullptr;
}

static void
StrAddRef(arc_string::internal* Internal)
{
  if(Internal == nullptr)
    return;

  // TODO: Make thread-safe?
  ++Internal->RefCount;
}

static void
StrReleaseRef(allocator_interface* Allocator, arc_string::internal* Internal)
{
  if(Internal == nullptr)
    return;

  // If there's an Internal pointer, there _must_ be an Allocator;
  Assert(Allocator);

  // TODO: Make thread-safe?
  --Internal->RefCount;

  if(Internal->RefCount <= 0)
  {
    DestroyInternal(*Allocator, Internal);
  }
}

arc_string::arc_string(arc_string const& Copy)
  : Allocator{ Copy.Allocator }
  , Internal{ Copy.Internal }
{
  StrAddRef(this->Internal);
}

arc_string::arc_string(char const* StringPtr)
{
  StrSet(*this, SliceFromString(StringPtr));
}

arc_string::arc_string(char const* StringPtr, allocator_interface& Allocator)
  : Allocator(&Allocator)
{
  StrSet(*this, SliceFromString(StringPtr));
}

arc_string::arc_string(slice<char const> Content)
{
  StrSet(*this, Content);
}

arc_string::arc_string(slice<char const> Content, allocator_interface& Allocator)
  : Allocator(&Allocator)
{
  StrSet(*this, Content);
}

arc_string::arc_string(slice<char> Content)
{
  StrSet(*this, AsConst(Content));
}

arc_string::arc_string(slice<char> Content, allocator_interface& Allocator)
  : Allocator(&Allocator)
{
  StrSet(*this, AsConst(Content));
}


arc_string::arc_string(arc_string&& Copy)
  : Internal{ Copy.Internal }
  , Allocator{ Copy.Allocator }
{
  // Steal it.
  Copy.Internal = nullptr;
}

arc_string::~arc_string()
{
  StrReleaseRef(this->Allocator, this->Internal);
  this->Internal = nullptr;
}

auto
arc_string::operator=(arc_string const& Copy)
  -> void
{
  if(this->Internal != Copy.Internal)
  {
    StrReleaseRef(this->Allocator, this->Internal);
    this->Internal = Copy.Internal;
    StrAddRef(this->Internal);
  }
}

auto
arc_string::operator=(arc_string&& Copy)
  -> void
{
  if(this->Internal != Copy.Internal)
  {
    StrReleaseRef(this->Allocator, this->Internal);
    this->Internal = Copy.Internal;
    Copy.Internal = nullptr;
  }
}

auto
arc_string::operator=(char const* StringPtr)
  -> void
{
  StrSet(*this, SliceFromString(StringPtr));
}

auto
arc_string::operator=(slice<char const> Content)
  -> void
{
  StrSet(*this, Content);
}

auto
arc_string::operator=(slice<char> Content)
  -> void
{
  StrSet(*this, AsConst(Content));
}

auto
::StrEnsureInitialized(arc_string& String)
  -> void
{
  if(StrIsInitialized(String))
    return;

  if(String.Allocator == nullptr)
    String.Allocator = GetDefaultStringAllocator();

  String.Internal = CreateInternal(*String.Allocator);
}

auto
::StrEnsureUnique(arc_string& String)
  -> void
{
  StrEnsureInitialized(String);
  if(String.Internal->RefCount > 1)
  {
    Assert(String.Allocator);
    auto Allocator = String.Allocator;
    auto OldInternal = String.Internal;
    StrReleaseRef(Allocator, OldInternal);

    auto NewInternal = CreateInternal(*Allocator);

    // Copy the old content over.
    SliceCopy(ExpandBy(NewInternal->Data, OldInternal->Data.Num),
              AsConst(Slice(OldInternal->Data)));

    // Apply the new Internal instance.
    String.Internal = NewInternal;
  }
}

auto
::StrEnsureZeroTerminated(arc_string& String)
  -> void
{
  StrEnsureInitialized(String);

  // Note: We don't consider the zero to be actual data, it is intentionally
  // outside of the data boundary.
  Reserve(String.Internal->Data, String.Internal->Data.Num + 1);
  auto const OnePastLastCharPtr = OnePastLast(Slice(String.Internal->Data));
  if(*OnePastLastCharPtr != '\0')
    *OnePastLastCharPtr = '\0';
}

auto
::StrInternalData(arc_string& String)
  -> array<char>&
{
  Assert(StrIsInitialized(String));
  return String.Internal->Data;
}

auto
::StrInternalData(arc_string const& String)
  -> array<char> const&
{
  Assert(StrIsInitialized(String));
  return String.Internal->Data;
}

void
StrInvalidateInternalData(arc_string& String)
{
  // TODO: Do more stuff here?
  StrEnsureZeroTerminated(String);
}

auto
::StrCapacity(arc_string const& String)
  -> size_t
{
  if(StrIsInitialized(String))
    return String.Internal->Data.Capacity;
  return 0;
}

auto
::StrNumBytes(arc_string const& String)
  -> size_t
{
  if(StrIsInitialized(String))
    return String.Internal->Data.Num;
  return 0;
}

auto
::StrNumChars(arc_string const& String)
  -> size_t
{
  // TODO: Decode?
  if(StrIsInitialized(String))
    return String.Internal->Data.Num;
  return 0;
}

auto
::StrIsEmpty(arc_string const& String)
  -> bool
{
  return StrNumBytes(String) == 0;
}

auto
::StrPtr(arc_string& String)
  -> char*
{
  if(!StrIsInitialized(String))
    return &MutableEmptyString[0];

  return String.Internal->Data.Ptr;
}

auto
::StrPtr(arc_string const& String)
  -> char const*
{
  if(!StrIsInitialized(String))
    return "";

  return String.Internal->Data.Ptr;
}

auto
::Slice(arc_string& String)
  -> slice<char>
{
  if(!StrIsInitialized(String))
    return SliceFromString(MutableEmptyString);

  return Slice(String.Internal->Data);
}

auto
::Slice(arc_string const& String)
  -> slice<char const>
{
  if(!StrIsInitialized(String))
    return ""_S;

  return AsConst(Slice(String.Internal->Data));
}

auto
::StrClear(arc_string& String)
  -> void
{
  if(!StrIsInitialized(String))
    return;

  StrEnsureUnique(String);
  Clear(String.Internal->Data);
  StrEnsureZeroTerminated(String);
}

auto
::StrReset(arc_string& String)
  -> void
{
  if(!StrIsInitialized(String))
    return;

  StrReleaseRef(String.Allocator, String.Internal);
  String.Internal = nullptr;
}

auto
::StrSet(arc_string& String, slice<char const> Content)
  -> void
{
  StrClear(String);
  StrAppend(String, Content);
}

auto
::StrAppend(arc_string& String, slice<char const> More)
  -> void
{
  // TODO: Validate `More` for proper encoding?
  StrEnsureUnique(String);
  auto const NewRegion = ExpandBy(String.Internal->Data, More.Num);
  SliceCopy(NewRegion, More);
  StrEnsureZeroTerminated(String);
}

auto
::StrPrepend(arc_string& String, slice<char const> More)
  -> void
{
  // TODO: Validate `More` for proper encoding?
  StrEnsureUnique(String);

  ExpandBy(String.Internal->Data, More.Num);
  auto Data = Slice(String.Internal->Data);
  auto OriginalData = Slice(Data, 0, Data.Num - More.Num);
  auto MovedOriginalData = Slice(OriginalData.Num, OriginalData.Ptr + More.Num);

  SliceMove(MovedOriginalData, OriginalData);

  auto NewData = Slice(Data, 0, More.Num);
  SliceCopy(NewData, More);

  StrEnsureZeroTerminated(String);
}

auto
::StrConcat(arc_string const& String, slice<char const> More)
  -> arc_string
{
  auto Copy = String;
  StrAppend(Copy, More);
  return Copy;
}

auto
::StrConcat(slice<char const> More, arc_string const& String)
  -> arc_string
{
  auto Copy = String;
  StrPrepend(Copy, More);
  return Copy;
}
