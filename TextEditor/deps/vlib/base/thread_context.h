#pragma once
#include "arena.h"

#define TC_ARENAS_COUNT 2

struct ThreadContext {
	Arena* arenas[TC_ARENAS_COUNT];
	bool isInitialized;
};

void ThreadContextInit(i64 arenasCommit = ARENA_COMMIT_SIZE, i64 arenasReserve = ARENA_DEFAULT_RESERVE);
void ThreadContextRelease();
ThreadContext* ThreadContextGet();
