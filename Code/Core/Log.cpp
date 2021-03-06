#include "Log.hpp"

#if defined(BB_Platform_Windows)
  #include <Windows.h>
#endif

#include <stdio.h>


CORE_API log_data* GlobalLog = nullptr;

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
FormatLogMessage(arc_string& FormattedMessage, char const* Format, va_list Args)
{
  // We're tinkering around with the string internals so we signalize that to
  // the string itself once we're done with the data.
  Defer [&](){ StrInvalidateInternalData(FormattedMessage); };

  StrEnsureUnique(FormattedMessage);
  auto& MessageBuffer = StrInternalData(FormattedMessage);
  Clear(MessageBuffer);
  Reserve(MessageBuffer, 1);

  int Result = ::vsnprintf(MessageBuffer.Ptr, MessageBuffer.Capacity, Format, Args);

  if(Result < 0)
  {
    // TODO Handle this somehow?
    // An error occurred.
    return false;
  }

  if(Result < MessageBuffer.Capacity)
  {
    MessageBuffer.Num = Cast<size_t>(Result);
    return true;
  }

  Reserve(MessageBuffer, Cast<size_t>(Result + 1));

  Result = ::vsnprintf(MessageBuffer.Ptr, MessageBuffer.Capacity, Format, Args);

  if(Result >= 0 || Result < MessageBuffer.Capacity)
  {
    MessageBuffer.Num = Cast<size_t>(Result);
    return true;
  }

  return false;
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

  for(auto LogSink : Slice(Log->Sinks))
  {
    LogSink(LogSinkArgs);
  }

  if(LogLevel == log_level::ScopeBegin)
  {
    LogIndent(Log);
  }
}

auto
::LogMessageDispatch(log_level LogLevel, arc_string Message, ...)
  -> void
{
  log_data* Log = GlobalLog;

  if(Log == nullptr)
    return;

  va_list Args;
  va_start(Args, Message);
  FormatLogMessage(Log->MessageBuffer, StrPtr(Message), Args);
  va_end(Args);

  auto FormattedMessage = AsConst(Slice(Log->MessageBuffer));
  LogMessageDispatch_Helper(Log, LogLevel, FormattedMessage);
}

auto
::LogMessageDispatch(log_level LogLevel, log_data* Log, arc_string Message, ...)
  -> void
{
  if(Log == nullptr)
    return;

  va_list Args;
  va_start(Args, Message);
  FormatLogMessage(Log->MessageBuffer, StrPtr(Message), Args);
  va_end(Args);

  auto FormattedMessage = AsConst(Slice(Log->MessageBuffer));
  LogMessageDispatch_Helper(Log, LogLevel, FormattedMessage);
}

static void
ImplStdoutLogSink_WithPrefixes(log_sink_args Args)
{
  #if defined(BB_Platform_Windows)
    auto const ConsoleHandle = GetStdHandle(STD_ERROR_HANDLE);

    CONSOLE_SCREEN_BUFFER_INFO CurrentConsoleInfo;
    GetConsoleScreenBufferInfo(ConsoleHandle, &CurrentConsoleInfo);

    // Restore the current console attributes at the end of the function.
    Defer [=](){ SetConsoleTextAttribute(ConsoleHandle, CurrentConsoleInfo.wAttributes); };

    auto Color = CurrentConsoleInfo.wAttributes;
    switch(Args.LogLevel)
    {
      case log_level::Info:    Color = Color | FOREGROUND_INTENSITY; break;
      case log_level::Warning: Color = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN; break;
      case log_level::Error:   Color = FOREGROUND_INTENSITY | FOREGROUND_RED; break;
    }

    SetConsoleTextAttribute(ConsoleHandle, Color);

    switch(Args.LogLevel)
    {
      case log_level::Info:       WriteConsole(ConsoleHandle, "Ifo", 3, nullptr, nullptr); break;
      case log_level::Warning:    WriteConsole(ConsoleHandle, "Wrn", 3, nullptr, nullptr); break;
      case log_level::Error:      WriteConsole(ConsoleHandle, "Err", 3, nullptr, nullptr); break;
      case log_level::ScopeBegin: WriteConsole(ConsoleHandle, ">>>", 3, nullptr, nullptr); break;
      case log_level::ScopeEnd:   WriteConsole(ConsoleHandle, "<<<", 3, nullptr, nullptr); break;
    }

    if(Args.Message)
    {
      WriteConsole(ConsoleHandle, ": ", 2, nullptr, nullptr);

      while(Args.Indentation > 0)
      {
        WriteConsole(ConsoleHandle, "  ", 2, nullptr, nullptr);
        --Args.Indentation;
      }

      WriteConsole(ConsoleHandle, Args.Message.Ptr, Cast<DWORD>(Args.Message.Num), nullptr, nullptr);
    }

    WriteConsole(ConsoleHandle, "\n", 1, nullptr, nullptr);
  #else
    switch(Args.LogLevel)
    {
      case log_level::Info:       fprintf(stderr, "Ifo"); break;
      case log_level::Warning:    fprintf(stderr, "Wrn"); break;
      case log_level::Error:      fprintf(stderr, "Err"); break;
      case log_level::ScopeBegin: fprintf(stderr, ">>>"); break;
      case log_level::ScopeEnd:   fprintf(stderr, "<<<"); break;
    }

    if(Args.Message)
    {
      fprintf(stderr, ": ");

      while(Args.Indentation > 0)
      {
        fprintf(stderr, "  ");
        --Args.Indentation;
      }

      fprintf(stderr, "%*s", Cast<int>(Args.Message.Num), Args.Message.Ptr);
    }

    fprintf(stderr, "\n");
  #endif
}

static void
ImplStdoutLogSink_WithoutPrefixes(log_sink_args Args)
{
  #if defined(BB_Platform_Windows)
    auto const ConsoleHandle = GetStdHandle(STD_ERROR_HANDLE);

    CONSOLE_SCREEN_BUFFER_INFO CurrentConsoleInfo;
    GetConsoleScreenBufferInfo(ConsoleHandle, &CurrentConsoleInfo);

    // Restore the current console attributes at the end of the function.
    Defer [=](){ SetConsoleTextAttribute(ConsoleHandle, CurrentConsoleInfo.wAttributes); };

    auto Color = CurrentConsoleInfo.wAttributes;
    switch(Args.LogLevel)
    {
      case log_level::Info:    Color = Color | FOREGROUND_INTENSITY; break;
      case log_level::Warning: Color = FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN; break;
      case log_level::Error:   Color = FOREGROUND_INTENSITY | FOREGROUND_RED; break;
    }

    SetConsoleTextAttribute(ConsoleHandle, Color);

    if(Args.Message)
    {
      while(Args.Indentation > 0)
      {
        WriteConsole(ConsoleHandle, "  ", 2, nullptr, nullptr);
        --Args.Indentation;
      }

      WriteConsole(ConsoleHandle, Args.Message.Ptr, Cast<DWORD>(Args.Message.Num), nullptr, nullptr);
    }

    WriteConsole(ConsoleHandle, "\n", 1, nullptr, nullptr);
  #else
    if(Args.Message)
    {
      while(Args.Indentation > 0)
      {
        fprintf(stderr, "  ");
        --Args.Indentation;
      }

      fprintf(stderr, "%*s", Cast<int>(Args.Message.Num), Args.Message.Ptr);
    }

    fprintf(stderr, "\n");
  #endif
}

auto
::GetStdoutLogSink(stdout_log_sink_enable_prefixes EnablePrefixes)
  -> log_sink
{
  if(EnablePrefixes == stdout_log_sink_enable_prefixes::Yes)
    return ImplStdoutLogSink_WithPrefixes;

  return ImplStdoutLogSink_WithoutPrefixes;
}

auto
::VisualStudioLogSink(log_sink_args Args)
  -> void
{
  switch(Args.LogLevel)
  {
    case log_level::Info:       OutputDebugStringA("[VKExp] Ifo"); break;
    case log_level::Warning:    OutputDebugStringA("[VKExp] Wrn"); break;
    case log_level::Error:      OutputDebugStringA("[VKExp] Err"); break;
    case log_level::ScopeBegin: OutputDebugStringA("[VKExp] >>>"); break;
    case log_level::ScopeEnd:   OutputDebugStringA("[VKExp] <<<"); break;
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


auto
::Win32LogErrorCode(log_data* Log, DWORD ErrorCode)
  -> void
{
  if(ErrorCode == 0 || Log == nullptr)
  {
    return;
  }

  LPSTR Message = {};
  // NOTE(Manu): Free the message when leaving this function.
  Defer [Message](){ if(Message) LocalFree(Message); };

  DWORD const FormatMessageFlags = FORMAT_MESSAGE_ALLOCATE_BUFFER |
                                   FORMAT_MESSAGE_FROM_SYSTEM |
                                   FORMAT_MESSAGE_IGNORE_INSERTS;
  DWORD const LanguageIdentifier = MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT);
  DWORD const MessageSize = FormatMessageA(
        FormatMessageFlags,           // _In_     DWORD   dwFlags
        nullptr,                      // _In_opt_ LPCVOID lpSource
        ErrorCode,                    // _In_     DWORD   dwMessageId
        LanguageIdentifier,           // _In_     DWORD   dwLanguageId
        Reinterpret<LPSTR>(&Message), // _Out_    LPTSTR  lpBuffer
        0,                            // _In_     DWORD   nSize
        nullptr);                     // _In_opt_ va_list *Argument
  if(MessageSize == 0)
  {
    LogError(Log, "Failed to format the win32 error.");
  }
  else
  {
    LogError(Log, "[Win32] %*s", Convert<int>(MessageSize), Message);
  }
}

auto
::Win32LogErrorCode(DWORD ErrorCode)
  -> void
{
  Win32LogErrorCode(GlobalLog, ErrorCode);
}
