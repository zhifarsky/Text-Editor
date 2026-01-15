#pragma once
#include "../general.h"

//
// High Frequency Timer
//

struct hf_timer {
	u64 rtdscFrequency;
	bool IsCalibrated;
};

void TimerCalibrate(f64 calibrateTimeSeconds);

struct timer_timestamp {
	u64 start, stop;

	void Start();
	void Stop();
	f64 TimeElapsedSeconds();
	f64 TimeElapsedMilliseconds();
	void StopAndPrint(const char* blockName = "");
};

//
// Profiler
//

struct profiler_anchor {
	const char* name;
	i64 clocksExclusive;	// исключая профилируемые подфункции
	i64 clocksInclusive;	// включая профилируемые подфункции
	u64 timesCalled;
};

struct profiler_state {
	profiler_anchor anchors[4096];
	i64 lastAnchorIdx;
	u64 start, end;
	bool isInitialized;
};

void ProfilerStart(f64 calibrationTime = 0.01);
void ProfilerPrintResults();

struct profiler_block {
	i64 anchorIdx, parentAnchorIdx;
	u64 start, end;
	u64 oldInclusiveClocks;

	profiler_block(const char* anchorName, i64 anchorIdx_);
	~profiler_block();
};

#ifdef USE_PROFILER
#pragma message("[VLIB] Profiler included.")
#define ProfileBlock(name) profiler_block NameConcat(profilerBlock, __LINE__)(name, __COUNTER__ + 1)
#define ProfileFunction() ProfileBlock(__FUNCTION__)
#else
#define ProfileBlock(name)
#define ProfileFunction()
#endif
