
#include "String.hpp"

#include <stdio.h>

static char MutableEmptyString[] = { '\0' };

#define STR_LOG(...) NoOp
// #define STR_LOG(...) printf(__VA_ARGS__)

class string_allocator : public allocator_interface
{
  static_assert(alignof(arc_string::internal) != alignof(char),
                "Basic assumptions for this allocator are wrong?!");

  struct string_internal_bucket
  {
    bool IsFree = true;
    arc_string::internal Data;
  };

  enum { STRING_DATA_BUCKET_SIZE = 1024 };
  struct string_data_bucket
  {
    slice<char> AllocatedData{};
    fixed_block<STRING_DATA_BUCKET_SIZE, char> Data;
  };


  allocator_interface* BaseAllocator;

  array<string_internal_bucket> StringInternalBuckets;
  array<string_data_bucket>     StringDataBuckets;

public:
  string_allocator(allocator_interface& BaseAllocator)
    : BaseAllocator(&BaseAllocator)
    , StringInternalBuckets(BaseAllocator)
    , StringDataBuckets(BaseAllocator)
  {
    Reserve(this->StringInternalBuckets, 100); // Around 5KiB
    this->StringInternalBuckets.CanGrow = false;

    Reserve(this->StringDataBuckets, 100); // Around 100 KiB (a bit more)
    this->StringDataBuckets.CanGrow = false;
  }

  ~string_allocator()
  {
  }

  virtual void* Allocate(memory_size Size, size_t Alignment)
  {
    Assert(Size > 0);
    STR_LOG("string_allocator: ");

    void* Result{};

    Defer [&](){ STR_LOG("Ptr: 0x%zx\n", Reinterpret<size_t>(Result)); };

    //
    // Handle string data allocation
    //
    if(Alignment == 1)
    {
      STR_LOG("Allocating data ");

      if(Size > STRING_DATA_BUCKET_SIZE)
      {
        STR_LOG("using base allocator because the size is too big (%.3fKiB). ", ToKiB(Size));

        Result = this->BaseAllocator->Allocate(Size, Alignment);
        return Result;
      }

      auto NewBucket = GetFreeDataBucket();
      if(NewBucket)
      {
        STR_LOG("using bucket. ");

        NewBucket->AllocatedData = Slice(NewBucket->Data, 0, ToBytes(Size));
        Result = NewBucket->AllocatedData.Ptr;
        return Result;
      }

      STR_LOG("using base allocator because there's no free bucket. ");

      // Out of internal memory. Use the base allocator now.
      Result = this->BaseAllocator->Allocate(Size, Alignment);
      return Result;
    }

    //
    // Handle allocation of arc_string::internal
    //
    if(Alignment == alignof(arc_string::internal))
    {
      Assert(Size == SizeOf<arc_string::internal>());
      STR_LOG("Allocating internal ");

      auto NewBucket = GetFreeInternalBucket();
      if(NewBucket)
      {
        STR_LOG("using bucket. ");

        NewBucket->IsFree = false;
        Result = &NewBucket->Data;
        return Result;
      }

      // Out of internal buckets. Use the base allocator now.
      Result = this->BaseAllocator->Allocate(Size, Alignment);
      return Result;
    }

    // Unrecognized allocation. Use the base allocator for it.
    STR_LOG("Unrecognized allocation. ");
    Result = this->BaseAllocator->Allocate(Size, Alignment);
    return Result;
  }

  virtual void Deallocate(void* Ptr)
  {
    STR_LOG("string_allocator: Deallocation: 0x%zx ", Reinterpret<size_t>(Ptr));

    if(Ptr == nullptr)
      return;

    //
    // Handle deallocation of arc_string::internal
    //
    auto InternalBucket = MapToInternalBucket(Ptr);
    if(InternalBucket)
    {
      STR_LOG("is an internal bucket.\n");
      // Double free?
      Assert(!InternalBucket->IsFree);

      InternalBucket->IsFree = true;
      return;
    }

    //
    // Handle deallocation of data that fits in the bucket size
    //
    auto DataBucket = MapToDataBucket(Ptr);
    if(DataBucket)
    {
      STR_LOG("is a data bucket.\n");

      // Double free?
      Assert(!!DataBucket->AllocatedData);

      DataBucket->AllocatedData = {};
      return;
    }

    //
    // Handle all other deallocations
    //
    STR_LOG("is something allocated with the base allocator.\n");
    this->BaseAllocator->Deallocate(Ptr);
  }

  virtual bool Resize(void* Ptr, memory_size NewSize)
  {
    if(Ptr == nullptr)
      return false;

    if(NewSize == 0)
      return false;

    //
    // Only support resizing of string data (not internals) right now.
    //
    auto DataBucket = MapToDataBucket(Ptr);
    if(DataBucket)
    {
      if(NewSize > STRING_DATA_BUCKET_SIZE)
      {
        // Doesn't fit into the bucket.
        return false;
      }

      DataBucket->AllocatedData = Slice(DataBucket->Data, 0, NewSize);
      return true;
    }

    return this->BaseAllocator->Resize(Ptr, NewSize);
  }

  virtual memory_size AllocatationSize(void* Ptr)
  {
    if(Ptr == nullptr)
      return Bytes(0);

    //
    // Handle deallocation of arc_string::internal
    //
    auto InternalBucket = MapToInternalBucket(Ptr);
    if(InternalBucket)
      return SizeOf<string_internal_bucket>();

    //
    // Handle deallocation of data that fits in the bucket size
    //
    auto DataBucket = MapToDataBucket(Ptr);
    if(DataBucket)
      return Bytes(DataBucket->AllocatedData.Num);

    //
    // Handle all other deallocations
    //
    return this->BaseAllocator->AllocationSize(Ptr);
  }

private:
  string_internal_bucket*
  MapToInternalBucket(void* Ptr)
  {
    auto Location = Reinterpret<size_t>(Ptr);
    Location -= offsetof(string_internal_bucket, Data);

    auto const BucketsSlice = Slice(this->StringInternalBuckets);
    auto const FirstPossibleLocation = Reinterpret<size_t>(First(BucketsSlice));
    auto const FirstImpossibleLocation = Reinterpret<size_t>(OnePastLast(BucketsSlice));
    if(Location >= FirstPossibleLocation && Location < FirstImpossibleLocation)
    {
      auto Bucket = Reinterpret<string_internal_bucket*>(Location);
      return Bucket;
    }

    return nullptr;
  }

  string_data_bucket*
  MapToDataBucket(void* Ptr)
  {
    auto Location = Reinterpret<size_t>(Ptr);
    Location -= offsetof(string_data_bucket, Data);

    auto const BucketsSlice = Slice(this->StringDataBuckets);
    auto const FirstPossibleLocation = Reinterpret<size_t>(First(BucketsSlice));
    auto const FirstImpossibleLocation = Reinterpret<size_t>(OnePastLast(BucketsSlice));
    if(Location >= FirstPossibleLocation && Location < FirstImpossibleLocation)
    {
      auto Bucket = Reinterpret<string_data_bucket*>(Location);
      return Bucket;
    }

    return nullptr;
  }

  string_internal_bucket*
  GetFreeInternalBucket()
  {
    for(auto& Bucket : Slice(this->StringInternalBuckets))
    {
      if(Bucket.IsFree)
        return &Bucket;
    }

    if(this->StringInternalBuckets.Num == this->StringInternalBuckets.Capacity)
    {
      return nullptr;
    }

    return &Expand(this->StringInternalBuckets);
  }

  string_data_bucket*
  GetFreeDataBucket()
  {
    for(auto& Bucket : Slice(this->StringDataBuckets))
    {
      if(!Bucket.AllocatedData)
        return &Bucket;
    }

    if(this->StringDataBuckets.Num == this->StringDataBuckets.Capacity)
    {
      return nullptr;
    }

    return &Expand(this->StringDataBuckets);
  }
};

static allocator_interface* GlobalStringDefaultAllocator{};

auto
::StringDefaultAllocator()
  -> allocator_interface*
{
  if(GlobalStringDefaultAllocator == nullptr)
  {
    static mallocator BaseAllocator{};

    #if 0
      static string_allocator* TheAllocator{};
      if(TheAllocator == nullptr)
      {
        TheAllocator = ::Allocate<string_allocator>(BaseAllocator);
        MemConstruct(1, TheAllocator, BaseAllocator);
      }
      GlobalStringDefaultAllocator = TheAllocator;
    #else
      GlobalStringDefaultAllocator = &BaseAllocator;
    #endif
  }
  return GlobalStringDefaultAllocator;
}

auto
::SetStringDefaultAllocator(allocator_interface* Allocator)
  -> void
{
  GlobalStringDefaultAllocator = Allocator;
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

arc_string::arc_string(allocator_interface& Allocator)
  : Allocator{ &Allocator }
{
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
    String.Allocator = StringDefaultAllocator();

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
