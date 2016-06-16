#pragma once

#include "Allocator.hpp"
#include "DynamicArray.hpp"

#include <functional>


enum class log_level
{
  Info,
  Warning,
  Error,

  ScopeBegin,
  ScopeEnd,
};

struct log_sink_args
{
  log_level LogLevel;
  slice<char const> Message;
  int Indentation;
};

using log_sink = std::function<void(log_sink_args)>;

struct log_data
{
  dynamic_array<char> MessageBuffer;
  dynamic_array<log_sink> Sinks;
  int Indentation;
};

void
LogInit(log_data* Log, allocator_interface* Allocator);

void
LogClearMessageBuffer(log_data* Log);

void
LogIndent(log_data* Log, int By = 1);

void
LogDedent(log_data* Log, int By = 1);

void
LogMessageDispatch(log_data* Log, log_level LogLevel, char const* Message, ...);

void
LogMessageDispatch(log_data* Log, log_level LogLevel, slice<char const> Message, ...);

#define LogBeginScope(Log, ...) LogMessageDispatch(Log, log_level::ScopeBegin, __VA_ARGS__);
#define LogEndScope(Log, ...)   LogMessageDispatch(Log, log_level::ScopeEnd,   __VA_ARGS__);
#define LogInfo(Log, ...)       LogMessageDispatch(Log, log_level::Info,       __VA_ARGS__);
#define LogWarning(Log, ...)    LogMessageDispatch(Log, log_level::Warning,    __VA_ARGS__);
#define LogError(Log, ...)      LogMessageDispatch(Log, log_level::Error,      __VA_ARGS__);

//
// Default Log Sinks
//

void
StdoutLogSink(log_sink_args Args);

void
VisualStudioLogSink(log_sink_args Args);
