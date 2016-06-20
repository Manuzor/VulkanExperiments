#include "Log.hpp"

#include <Windows.h>
#include <stdio.h>


log_data* GlobalLog = nullptr;

void
LogInit(log_data* Log, allocator_interface* Allocator)
{
  Log->MessageBuffer.Allocator = Allocator;
  Log->Sinks.Allocator = Allocator;
}

void
LogIndent(log_data* Log, int By)
{
  if(Log)
  {
    Log->Indentation += By;
    Assert(Log->Indentation >= 0);
  }
}

void
LogDedent(log_data* Log, int By)
{
  if(Log)
  {
    Log->Indentation -= By;
    Assert(Log->Indentation >= 0);
  }
}

static void
LogMessageDispatch_Helper(log_data* Log, log_level LogLevel, slice<char const> Message)
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

void
LogMessageDispatch(log_level LogLevel, char const* Message, ...)
{
  if(GlobalLog == nullptr)
    return;

  Clear(&GlobalLog->MessageBuffer);
  // TODO Format message

  auto SlicedMessage = SliceFromString(Message);
  LogMessageDispatch_Helper(GlobalLog, LogLevel, SlicedMessage);
}

void
LogMessageDispatch(log_level LogLevel, slice<char const> Message, ...)
{
  if(GlobalLog == nullptr)
    return;

  Clear(&GlobalLog->MessageBuffer);
  // TODO Format message

  LogMessageDispatch_Helper(GlobalLog, LogLevel, Message);
}


void
LogMessageDispatch(log_data* Log, log_level LogLevel, char const* Message, ...)
{
  if(Log == nullptr)
    return;

  Clear(&Log->MessageBuffer);
  // TODO Format message

  auto SlicedMessage = SliceFromString(Message);
  LogMessageDispatch_Helper(Log, LogLevel, SlicedMessage);
}

void
LogMessageDispatch(log_data* Log, log_level LogLevel, slice<char const> Message, ...)
{
  if(Log == nullptr)
    return;

  Clear(&Log->MessageBuffer);
  // TODO Format message

  LogMessageDispatch_Helper(Log, LogLevel, Message);
}

void
StdoutLogSink(log_sink_args Args)
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

void
VisualStudioLogSink(log_sink_args Args)
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
