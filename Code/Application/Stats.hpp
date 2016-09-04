#pragma once

#include <Backbone.hpp>
#include <Core/Array.hpp>
#include <Core/String.hpp>
#include <Core/Time.hpp>

RESERVE_PREFIX(Frame);

struct frame_sample
{
  duration CpuTime;
  duration GpuTime;
  duration FrameTime;
};

struct frame_stats
{
  arc_string Name;
  array<frame_sample> Samples;

  size_t MaxNumSamples{};
  size_t SampleIndex{};
};

struct frame_stats_evaluated
{
  arc_string FrameName;
  size_t NumSamples;
  frame_sample FastestSample{};
  frame_sample SlowestSample{};
  frame_sample AverageSample{};
};

struct frame_stats_registry
{
  array<frame_stats> FrameStats{};
};

frame_stats*
FrameRegistryGetStats(frame_stats_registry* Registry, arc_string FrameName);

frame_stats_evaluated
FrameEvaluateStats(frame_stats* Stats);

void
FrameStatsPrintEvaluated(frame_stats_evaluated Stats, struct log_data* Log);

void
FrameStatsAddSample(frame_stats* Stats, frame_sample Sample);
