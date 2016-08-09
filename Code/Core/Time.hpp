#pragma once

#include "CoreAPI.hpp"

#include <Backbone.hpp>


struct time
{
  double InternalData;
};

constexpr time operator + (time   A, time   B) { return { A.InternalData + B.InternalData }; }
inline    void operator +=(time&  A, time   B) { A.InternalData += B.InternalData; }
constexpr time operator - (time   A, time   B) { return { A.InternalData - B.InternalData }; }
inline    void operator -=(time&  A, time   B) { A.InternalData -= B.InternalData; }
constexpr time operator * (time   A, time   B) { return { A.InternalData * B.InternalData }; }
constexpr time operator * (time   A, double B) { return { A.InternalData * B }; }
constexpr time operator * (double A, time   B) { return { A * B.InternalData }; }
inline    void operator *=(time&  A, time   B) { A.InternalData *= B.InternalData; }
inline    void operator *=(time&  A, double B) { A.InternalData *= B; }
constexpr time operator / (time   A, time   B) { return { A.InternalData / B.InternalData }; }
constexpr time operator / (time   A, double B) { return { A.InternalData / B }; }
inline    void operator /=(time&  A, time   B) { A.InternalData /= B.InternalData; }
inline    void operator /=(time&  A, double B) { A.InternalData /= B; }
constexpr bool operator ==(time&  A, time   B) { return A.InternalData == B.InternalData; }
constexpr bool operator !=(time&  A, time   B) { return A.InternalData != B.InternalData; }
constexpr bool operator < (time&  A, time   B) { return A.InternalData <  B.InternalData; }
constexpr bool operator <=(time&  A, time   B) { return A.InternalData <= B.InternalData; }
constexpr bool operator > (time&  A, time   B) { return A.InternalData >  B.InternalData; }
constexpr bool operator >=(time&  A, time   B) { return A.InternalData >= B.InternalData; }

struct stopwatch
{
  uint64 StartTimestamp;
  uint64 EndTimestamp;
  uint64 Frequency;
};


//
// Time creation and extraction
//

time constexpr
Seconds(double Amount)
{
  return { Amount };
}

time constexpr
Milliseconds(double Amount)
{
  return Seconds(Amount / 1000.0);
}

time constexpr
Microseconds(double Amount)
{
  return Milliseconds(Amount / 1000.0);
}

time constexpr
Nanoseconds(double Amount)
{
  return Microseconds(Amount / 1000.0);
}

double constexpr
TimeAsSeconds(time Time)
{
  return Time.InternalData;
}

double constexpr
TimeAsMilliseconds(time Time)
{
  return TimeAsSeconds(Time) * 1000.0;
}

double constexpr
TimeAsMicroseconds(time Time)
{
  return TimeAsMilliseconds(Time) * 1000.0;
}

double constexpr
TimeAsNanoseconds(time Time)
{
  return TimeAsMicroseconds(Time) * 1000.0;
}


//
// Stopwatch functions
//

void CORE_API
StopwatchStart(stopwatch* Stopwatch);

void CORE_API
StopwatchStop(stopwatch* Stopwatch);

time CORE_API
StopwatchTime(stopwatch* Stopwatch);
