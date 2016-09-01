#pragma once

#include "CoreAPI.hpp"

#include <Backbone.hpp>

// TODO: Change the name of this struct to something else as it might conflict
// with std stuff.
struct duration { double InternalData; };

constexpr duration operator + (duration A, duration B) { return { A.InternalData + B.InternalData }; }
constexpr duration operator - (duration A, duration B) { return { A.InternalData - B.InternalData }; }
constexpr duration operator * (duration A, duration B) { return { A.InternalData * B.InternalData }; }
constexpr duration operator * (duration A, double   B) { return { A.InternalData * B }; }
constexpr duration operator * (double   A, duration B) { return { A * B.InternalData }; }
constexpr duration operator / (duration A, duration B) { return { A.InternalData / B.InternalData }; }
constexpr duration operator / (duration A, double   B) { return { A.InternalData / B }; }

inline void operator +=(duration& A, duration B) { A.InternalData += B.InternalData; }
inline void operator -=(duration& A, duration B) { A.InternalData -= B.InternalData; }
inline void operator *=(duration& A, duration B) { A.InternalData *= B.InternalData; }
inline void operator *=(duration& A, double   B) { A.InternalData *= B; }
inline void operator /=(duration& A, duration B) { A.InternalData /= B.InternalData; }
inline void operator /=(duration& A, double   B) { A.InternalData /= B; }

constexpr bool operator ==(duration& A, duration B) { return A.InternalData == B.InternalData; }
constexpr bool operator !=(duration& A, duration B) { return A.InternalData != B.InternalData; }
constexpr bool operator < (duration& A, duration B) { return A.InternalData <  B.InternalData; }
constexpr bool operator <=(duration& A, duration B) { return A.InternalData <= B.InternalData; }
constexpr bool operator > (duration& A, duration B) { return A.InternalData >  B.InternalData; }
constexpr bool operator >=(duration& A, duration B) { return A.InternalData >= B.InternalData; }

struct stopwatch
{
  uint64 StartTimestamp;
  uint64 EndTimestamp;
  uint64 Frequency;
};


//
// Duration creation and extraction
//

duration constexpr
Seconds(double Amount)
{
  return { Amount };
}

duration constexpr
Milliseconds(double Amount)
{
  return Seconds(Amount / 1000.0);
}

duration constexpr
Microseconds(double Amount)
{
  return Milliseconds(Amount / 1000.0);
}

duration constexpr
Nanoseconds(double Amount)
{
  return Microseconds(Amount / 1000.0);
}

double constexpr
DurationAsSeconds(duration Duration)
{
  return Duration.InternalData;
}

double constexpr
DurationAsMilliseconds(duration Duration)
{
  return DurationAsSeconds(Duration) * 1000.0;
}

double constexpr
DurationAsMicroseconds(duration Duration)
{
  return DurationAsMilliseconds(Duration) * 1000.0;
}

double constexpr
DurationAsNanoseconds(duration Duration)
{
  return DurationAsMicroseconds(Duration) * 1000.0;
}


//
// Stopwatch functions
//

void CORE_API
StopwatchStart(stopwatch* Stopwatch);

void CORE_API
StopwatchStop(stopwatch* Stopwatch);

duration CORE_API
StopwatchDuration(stopwatch const* Stopwatch);
