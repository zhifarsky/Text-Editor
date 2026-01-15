#include <stdio.h>

#include "../platform/platform.h"
#include "profiler.h"

global_variable hf_timer g_Timer;
global_variable profiler_state g_Profiler = {0};

#define RdtscFrequency() (g_Timer.rtdscFrequency)
#define TimerIsCalibrated() (g_Timer.IsCalibrated)
#define ProfilerIsInitialized() (g_Profiler.isInitialized)

void TimerCalibrate(f64 calibrateTimeSeconds) {
#ifdef USE_PROFILER
	hf_timer* timer = &g_Timer;

	u64 OSFreq = os_GetTimerFreq(),
			OSStart = os_ReadTimer(),
			OSEnd = 0,
			OSElapsed = 0;
	// printf("OS Freq: %llu\n", OSFreq);

	// sampling start
	u64 rdtscStart = os_GetRDTSC();
	f64 OSMeasureTicks = (f64)OSFreq * calibrateTimeSeconds;
	while (OSElapsed < OSMeasureTicks) {
		OSEnd = os_ReadTimer();
		OSElapsed = OSEnd - OSStart;
	}
	u64 rdtscEnd = os_GetRDTSC();
	// sampling end

	// printf("OS Timer: %llu -> %llu = %llu elapsed\n", OSStart, OSEnd, OSElapsed);
	// printf("OS Seconds: %.4f\n", (f64)OSElapsed / (f64)OSFreq);

	timer->rtdscFrequency = (f64)(rdtscEnd - rdtscStart) / calibrateTimeSeconds;
	// printf("RDTCS: %llu -> %llu = %llu elapsed\n", rdtscStart, rdtscEnd, rdtscEnd - rdtscStart);

	timer->IsCalibrated = true;
#endif
}

void timer_timestamp::Start() {
#ifdef USE_PROFILER
	assert(TimerIsCalibrated());
	stop = 0;
	start = os_GetRDTSC();
#endif
}

void timer_timestamp::Stop() {
#ifdef USE_PROFILER
	stop = os_GetRDTSC();
#endif
}

f64 timer_timestamp::TimeElapsedSeconds() {
#ifdef USE_PROFILER
	f64 timeElapsed = (f64)(stop - start) / (f64)RdtscFrequency();
	return timeElapsed;
#else
	return 0;
#endif
}

f64 timer_timestamp::TimeElapsedMilliseconds() {
#ifdef USE_PROFILER
	f64 timeElapsed = (f64)(stop - start) / ((f64)RdtscFrequency() / 1000.0);
	return timeElapsed;
#else
	return 0;
#endif
}

void timer_timestamp::StopAndPrint(const char* blockName) {
#ifdef USE_PROFILER
	Stop();
	printf("[PROFILER] %s: %fms\n", blockName, TimeElapsedMilliseconds());
#endif
}

void ProfilerStart(f64 calibrationTime) {
#ifdef USE_PROFILER
	if (!TimerIsCalibrated()) {
		TimerCalibrate(calibrationTime);
	}

	profiler_state* profiler = &g_Profiler;
	*profiler = {0};

	profiler->anchors[0].name = "_root";
	profiler->isInitialized = true;
	profiler->start = os_GetRDTSC();
#endif
}

void ProfilerPrintResults() {
#ifdef USE_PROFILER
	profiler_state* profiler = &g_Profiler;
	hf_timer* timer = &g_Timer;

	assert(ProfilerIsInitialized());
	assert(TimerIsCalibrated());

	profiler->end = os_GetRDTSC();
	u64 totalClocks = profiler->end - profiler->start;

	profiler->anchors[0].clocksExclusive += totalClocks;
	profiler->anchors[0].clocksInclusive += totalClocks;

	u64 rdtscFrequency = timer->rtdscFrequency;

	printf("\n[PROFILER]\n");

	for (size_t i = 0; i < ArrayCount(profiler->anchors); i++) {
		profiler_anchor* anchor = &profiler->anchors[i];
		if (!anchor->name) {
			continue;
		}

		f64 elapsedExclusive = (f64)anchor->clocksExclusive / (f64)rdtscFrequency;
		f64 percentExclusive = 100.0 * (f64)anchor->clocksExclusive / (f64)totalClocks;

		printf(
				"%s [%llu]"
				"\n\telapsed: %fs %f%%",
				anchor->name,
				anchor->timesCalled,
				elapsedExclusive, percentExclusive);

		if (anchor->clocksInclusive != anchor->clocksExclusive) {
			f64 elapsedInclusive = (f64)anchor->clocksInclusive / (f64)rdtscFrequency;
			f64 percentInclusive = 100.0 * (f64)anchor->clocksInclusive / (f64)totalClocks;

			printf(
					"\n\tinclusive: %fs %f%%",
					elapsedInclusive, percentInclusive);
		}

		printf("\n");
	}

	f64 totalTimeSpendS = (f64)totalClocks / ((f64)rdtscFrequency);
	printf("\nTotal time: %fs\n", totalTimeSpendS);
#endif
}

profiler_block::profiler_block(const char* anchorName, i64 anchorIdx_) {
#ifdef USE_PROFILER
	if (!ProfilerIsInitialized())
		return;

	profiler_state* profiler = &g_Profiler;

	anchorIdx = anchorIdx_;
	parentAnchorIdx = profiler->lastAnchorIdx;
	profiler->lastAnchorIdx = anchorIdx;

	profiler_anchor* anchor = &profiler->anchors[anchorIdx];
	anchor->name = anchorName;

	oldInclusiveClocks = anchor->clocksInclusive;

	start = os_GetRDTSC();
#endif
}

profiler_block::~profiler_block() {
#ifdef USE_PROFILER
	end = os_GetRDTSC();

	if (!ProfilerIsInitialized())
		return;

	profiler_state* profiler = &g_Profiler;

	profiler->lastAnchorIdx = parentAnchorIdx;

	u64 elapsed = end - start;

	profiler_anchor* anchor = &profiler->anchors[anchorIdx];
	anchor->timesCalled++;
	anchor->clocksExclusive += elapsed;
	anchor->clocksInclusive = oldInclusiveClocks + elapsed;

	profiler_anchor* parentAnchor = &profiler->anchors[parentAnchorIdx];
	parentAnchor->clocksExclusive -= elapsed;
#endif
}
