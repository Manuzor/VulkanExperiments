#include "Log.hpp"

#include <Windows.h>
#include <stdio.h>


CORE_API log_data* GlobalLog = nullptr;

auto
::Init(log_data* Log, allocator_interface* Allocator)
  -> void
{
  Log->MessageBuffer.Allocator = Allocator;
  Log->TempBuffer.Allocator = Allocator;
  Log->Sinks.Allocator = Allocator;
}

auto
::Finalize(log_data* Log)
  -> void
{
  Finalize(&Log->MessageBuffer);
  Finalize(&Log->TempBuffer);
  Finalize(&Log->Sinks);
}

auto
::LogIndent(log_data* Log, int By)
  -> void
{
  if(Log)
  {
    Log->Indentation += By;
    Assert(Log->Indentation >= 0);
  }
}

auto
::LogDedent(log_data* Log, int By)
  -> void
{
  if(Log)
  {
    Log->Indentation -= By;
    Assert(Log->Indentation >= 0);
  }
}

static bool
FormatLogMessage(dynamic_array<char>* MessageBuffer, char const* Format, va_list Args)
{
  Clear(MessageBuffer);
  Reserve(MessageBuffer, 1);

  int Result = ::vsnprintf(MessageBuffer->Ptr, MessageBuffer->Capacity, Format, Args);

  if(Result < 0)
  {
    // TODO Handle this somehow?
    // An error occurred.
    return false;
  }

  if(Result < MessageBuffer->Capacity)
  {
    MessageBuffer->Num = Cast<size_t>(Result);
    MessageBuffer->Ptr[Result + 1] = '\0';
    return true;
  }

  Reserve(MessageBuffer, Cast<size_t>(Result + 1));

  Result = ::vsnprintf(MessageBuffer->Ptr, MessageBuffer->Capacity, Format, Args);

  return Result >= 0 || Result < MessageBuffer->Capacity;
}

static auto
LogMessageDispatch_Helper(log_data* Log, log_level LogLevel, slice<char const> Message)
  -> void
{
  if(LogLevel == log_level::ScopeEnd)
  {
    LogDedent(Log);
  }

  log_sink_args LogSinkArgs;
  LogSinkArgs.LogLevel = LogLevel;
  LogSinkArgs.Message = Message;
  LogSinkArgs.Indentation = Log->Indentation;

  for(auto LogSink : Slice(&Log->Sinks))
  {
    LogSink(LogSinkArgs);
  }

  if(LogLevel == log_level::ScopeBegin)
  {
    LogIndent(Log);
  }
}

static bool
SliceIsZeroTerminated(slice<char const> String)
{
  return String && String.Ptr + String.Num == '\0';
}

auto
::LogMessageDispatch(log_level LogLevel, char const* Message, ...)
  -> void
{
  if(GlobalLog == nullptr)
    return;

  va_list Args;
  va_start(Args, Message);
  FormatLogMessage(&GlobalLog->MessageBuffer, Message, Args);
  va_end(Args);

  auto FormattedMessage = AsConst(Slice(&GlobalLog->MessageBuffer));
  LogMessageDispatch_Helper(GlobalLog, LogLevel, FormattedMessage);
}

auto
::LogMessageDispatch(log_level LogLevel, slice<char const> Message, ...)
  -> void
{
  if(GlobalLog == nullptr)
    return;

  char const* MessagePtr;
  if(SliceIsZeroTerminated(Message))
  {
    MessagePtr = Message.Ptr;
  }
  else
  {
    Clear(&GlobalLog->TempBuffer);
    auto ZeroTerminatedMessage = ExpandBy(&GlobalLog->TempBuffer, Message.Num + 1);
    SliceCopy(ZeroTerminatedMessage, Message);
    ZeroTerminatedMessage[ZeroTerminatedMessage.Num - 1] = '\0';
    MessagePtr = ZeroTerminatedMessage.Ptr;
  }

  va_list Args;
  va_start(Args, Message);
  FormatLogMessage(&GlobalLog->MessageBuffer, MessagePtr, Args);
  va_end(Args);

  auto FormattedMessage = AsConst(Slice(&GlobalLog->MessageBuffer));
  LogMessageDispatch_Helper(GlobalLog, LogLevel, FormattedMessage);
}


auto
::LogMessageDispatch(log_level LogLevel, log_data* Log, char const* Message, ...)
  -> void
{
  if(Log == nullptr)
    return;

  va_list Args;
  va_start(Args, Message);
  FormatLogMessage(&Log->MessageBuffer, Message, Args);
  va_end(Args);

  auto FormattedMessage = AsConst(Slice(&GlobalLog->MessageBuffer));
  LogMessageDispatch_Helper(Log, LogLevel, FormattedMessage);
}

auto
::LogMessageDispatch(log_level LogLevel, log_data* Log, slice<char const> Message, ...)
  -> void
{
  if(Log == nullptr)
    return;

  char const* MessagePtr;
  if(SliceIsZeroTerminated(Message))
  {
    MessagePtr = Message.Ptr;
  }
  else
  {
    Clear(&Log->TempBuffer);
    auto ZeroTerminatedMessage = ExpandBy(&Log->TempBuffer, Message.Num + 1);
    SliceCopy(ZeroTerminatedMessage, Message);
    ZeroTerminatedMessage[ZeroTerminatedMessage.Num - 1] = '\0';
    MessagePtr = ZeroTerminatedMessage.Ptr;
  }

  va_list Args;
  va_start(Args, Message);
  FormatLogMessage(&Log->MessageBuffer, MessagePtr, Args);
  va_end(Args);

  auto FormattedMessage = AsConst(Slice(&GlobalLog->MessageBuffer));
  LogMessageDispatch_Helper(Log, LogLevel, FormattedMessage);
}

auto
::StdoutLogSink(log_sink_args Args)
  -> void
{
  switch(Args.LogLevel)
  {
    case log_level::Info:       printf("Info"); break;
    case log_level::Warning:    printf("Warn"); break;
    case log_level::Error:      printf("Err "); break;
    case log_level::ScopeBegin: printf(" >>>"); break;
    case log_level::ScopeEnd:   printf(" <<<"); break;
  }

  if(Args.Message)
  {
    printf(": ");

    while(Args.Indentation > 0)
    {
      printf("  ");
      --Args.Indentation;
    }

    printf("%*s\n", Cast<int>(Args.Message.Num), Args.Message.Ptr);
  }
  else
  {
    printf("\n");
  }
}

auto
::VisualStudioLogSink(log_sink_args Args)
  -> void
{
  switch(Args.LogLevel)
  {
    case log_level::Info:       OutputDebugStringA("[VKExp] Info"); break;
    case log_level::Warning:    OutputDebugStringA("[VKExp] Warn"); break;
    case log_level::Error:      OutputDebugStringA("[VKExp] Err "); break;
    case log_level::ScopeBegin: OutputDebugStringA("[VKExp]  >>>"); break;
    case log_level::ScopeEnd:   OutputDebugStringA("[VKExp]  <<<"); break;
  }

  if(Args.Message)
  {
    OutputDebugStringA(": ");

    while(Args.Indentation > 0)
    {
      OutputDebugStringA("  ");
      --Args.Indentation;
    }

    fixed_block<1024, char> BufferMemory;
    auto Buffer = Slice(BufferMemory);
    while(Args.Message)
    {
      auto Amount = Min(Buffer.Num - 1, Args.Message.Num);

      SliceCopy(Slice(Buffer, 0, Amount), Slice(Args.Message, 0, Amount));
      Buffer[Amount] = '\0';

      OutputDebugStringA(Buffer.Ptr);

      Args.Message = Slice(Args.Message, Amount, Args.Message.Num);
    }
  }

  OutputDebugStringA("\n");
}
