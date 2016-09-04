#include "Stats.hpp"

#include <Core/Log.hpp>


auto
::FrameRegistryGetStats(frame_stats_registry* Registry, arc_string FrameName)
  -> frame_stats*
{
  for(auto& Frame : Slice(Registry->FrameStats))
  {
    if(Frame.Name == FrameName)
    {
      return &Frame;
    }
  }

  auto Result = &Expand(Registry->FrameStats);
  Result->Name = FrameName;
  return Result;
}

auto
::FrameEvaluateStats(frame_stats* Stats)
  -> frame_stats_evaluated
{
  frame_stats_evaluated Result;
  Result.FrameName = Stats->Name;
  Result.NumSamples = Stats->Samples.Num;
  Result.FastestSample = { Seconds(1e20f), Seconds(1e20f), Seconds(1e20f) };
  Result.SlowestSample = {};
  Result.AverageSample = {};

  for(auto& Sample : Slice(Stats->Samples))
  {
    Result.AverageSample.CpuTime += Sample.CpuTime;
    Result.AverageSample.GpuTime += Sample.GpuTime;
    Result.AverageSample.FrameTime += Sample.FrameTime;

    Result.FastestSample.CpuTime =   Min(Sample.CpuTime,   Result.FastestSample.CpuTime);
    Result.FastestSample.GpuTime =   Min(Sample.GpuTime,   Result.FastestSample.GpuTime);
    Result.FastestSample.FrameTime = Min(Sample.FrameTime, Result.FastestSample.FrameTime);
    Result.SlowestSample.CpuTime =   Max(Sample.CpuTime,   Result.SlowestSample.CpuTime);
    Result.SlowestSample.GpuTime =   Max(Sample.GpuTime,   Result.SlowestSample.GpuTime);
    Result.SlowestSample.FrameTime = Max(Sample.FrameTime, Result.SlowestSample.FrameTime);
  }

  Result.AverageSample.CpuTime /= Convert<double>(Stats->Samples.Num);
  Result.AverageSample.GpuTime /= Convert<double>(Stats->Samples.Num);
  Result.AverageSample.FrameTime /= Convert<double>(Stats->Samples.Num);

  return Result;
}

auto
::FrameStatsPrintEvaluated(frame_stats_evaluated Stats, log_data* Log)
  -> void
{
  LogBeginScope(Log, "Frame Time Stats of '%s' with %u samples", StrPtr(Stats.FrameName), Stats.NumSamples);
  {
    LogInfo(Log, "Fastest Frame: (%4.u FPS) %fs, CPU: %fs, GPU: %fs",
            Round<uint64>(1.0f / DurationAsSeconds(Stats.FastestSample.FrameTime)),
            DurationAsSeconds(Stats.FastestSample.FrameTime),
            DurationAsSeconds(Stats.FastestSample.CpuTime),
            DurationAsSeconds(Stats.FastestSample.GpuTime));

    LogInfo(Log, "Slowest Frame: (%4.u FPS) %fs, CPU: %fs, GPU: %fs",
            Round<uint64>(1.0f / DurationAsSeconds(Stats.SlowestSample.FrameTime)),
            DurationAsSeconds(Stats.SlowestSample.FrameTime),
            DurationAsSeconds(Stats.SlowestSample.CpuTime),
            DurationAsSeconds(Stats.SlowestSample.GpuTime));

    LogInfo(Log, "Average Frame: (%4.u FPS) %fs, CPU: %fs, GPU: %fs",
            Round<uint64>(1.0f / DurationAsSeconds(Stats.AverageSample.FrameTime)),
            DurationAsSeconds(Stats.AverageSample.FrameTime),
            DurationAsSeconds(Stats.AverageSample.CpuTime),
            DurationAsSeconds(Stats.AverageSample.GpuTime));
  }
  LogEndScope(Log, "Frame Time Stats");
}

auto
::FrameStatsAddSample(frame_stats* Stats, frame_sample Sample)
  -> void
{
  auto const RequiredNumArrayElements = Stats->SampleIndex + 1;
  if(RequiredNumArrayElements > Stats->Samples.Num)
  {
    ExpandBy(Stats->Samples, RequiredNumArrayElements - Stats->Samples.Num);
  }

  Stats->Samples[Stats->SampleIndex] = Sample;

  Stats->SampleIndex = Wrap(Stats->SampleIndex + 1, 0, Stats->MaxNumSamples);
}
