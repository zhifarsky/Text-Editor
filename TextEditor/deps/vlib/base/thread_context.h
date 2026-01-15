#pragma once
#include "arena.h"

#define TC_ARENAS_COUNT 2

struct ThreadContext {
	Arena* arenas[TC_ARENAS_COUNT];
	bool isInitialized;
};

void ThreadContextInit();
void ThreadContextRelease();
ThreadContext* ThreadContextGet();
