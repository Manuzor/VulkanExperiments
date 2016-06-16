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

extern log_data* GlobalLog;

void
LogInit(log_data* Log, allocator_interface* Allocator);

void
LogIndent(log_data* Log, int By = 1);

void
LogDedent(log_data* Log, int By = 1);

/// Globally log a string literal or zero terminated string.
void
LogMessageDispatch(log_level LogLevel, char const* Message, ...);

/// Globally log a string slice.
void
LogMessageDispatch(log_level LogLevel, slice<char const> Message, ...);

/// Log a string literal or zero terminated string with the given log.
void
LogMessageDispatch(log_level LogLevel, log_data* Log, char const* Message, ...);

/// Log a string slice with the given log.
void
LogMessageDispatch(log_level LogLevel, log_data* Log, slice<char const> Message, ...);

#define LogBeginScope(...) LogMessageDispatch(log_level::ScopeBegin, __VA_ARGS__);
#define LogEndScope(...)   LogMessageDispatch(log_level::ScopeEnd,   __VA_ARGS__);
#define LogInfo(...)       LogMessageDispatch(log_level::Info,       __VA_ARGS__);
#define LogWarning(...)    LogMessageDispatch(log_level::Warning,    __VA_ARGS__);
#define LogError(...)      LogMessageDispatch(log_level::Error,      __VA_ARGS__);

//
// Default Log Sinks
//

void
StdoutLogSink(log_sink_args Args);

void
VisualStudioLogSink(log_sink_args Args);
