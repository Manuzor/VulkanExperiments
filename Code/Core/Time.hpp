#pragma once

#include "CoreAPI.hpp"

#include <Backbone.hpp>

struct time
{
  double InternalData;
};

struct stopwatch
{
  uint64 StartTimestamp;
  uint64 EndTimestamp;
  uint64 Frequency;
};


double CORE_API
TimeAsSeconds(time Time);

double CORE_API
TimeAsMilliseconds(time Time);


void CORE_API
StopwatchStart(stopwatch* Stopwatch);

void CORE_API
StopwatchStop(stopwatch* Stopwatch);

time CORE_API
StopwatchTime(stopwatch* Stopwatch);
