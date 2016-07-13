#include "Time.hpp"

#include <Windows.h>

auto
::StopwatchStart(stopwatch* Stopwatch)
  -> void
{
  LARGE_INTEGER Data;

  QueryPerformanceFrequency(&Data);
  Stopwatch->Frequency = Cast<uint64>(Data.QuadPart);

  QueryPerformanceCounter(&Data);
  Stopwatch->StartTimestamp = Cast<uint64>(Data.QuadPart);
}

auto
::StopwatchStop(stopwatch* Stopwatch)
  -> void
{
  LARGE_INTEGER Data;
  QueryPerformanceCounter(&Data);
  Stopwatch->EndTimestamp = Cast<uint64>(Data.QuadPart);
}

auto
::StopwatchTime(stopwatch* Stopwatch)
  -> time
{
  auto Delta = Stopwatch->EndTimestamp - Stopwatch->StartTimestamp;
  time Time;
  Time.InternalData = Cast<double>(Delta) / Cast<double>(Stopwatch->Frequency);
  return Time;
}
