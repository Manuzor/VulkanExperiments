#pragma once

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

double
TimeAsSeconds(time Time);

double
TimeAsMilliseconds(time Time);


void
StopwatchStart(stopwatch* Stopwatch);

void
StopwatchStop(stopwatch* Stopwatch);

time
StopwatchTime(stopwatch* Stopwatch);
