#include "general.cpp"

#include "platform/platform.cpp"
#include "tools/profiler.cpp"

#include "base/arena.cpp"
#include "base/array.cpp"
#include "base/string.cpp"
#include "base/thread_context.cpp"

void VlibInit(i64 arenasCommit, i64 arenasReserve) {
	ThreadContextInit();
	ProfilerStart();
}

void VlibDestroy() {
	ThreadContextRelease();
}