#include "thread_context.h"

thread_local ThreadContext* tl_ThreadContext;

void ThreadContextInit() {
	if (tl_ThreadContext != NULL) {
		LogWarning("Thread context already initalized");
		return;
	}

	Arena* arena = ArenaAlloc();
	tl_ThreadContext = ArenaPushStruct(arena, ThreadContext);
	tl_ThreadContext->arenas[0] = arena;
	tl_ThreadContext->arenas[1] = ArenaAlloc();
}

void ThreadContextRelease() {
	if (tl_ThreadContext == NULL) {
		LogWarning("Thread context not initialized");
		return;
	}

	for (i32 i = TC_ARENAS_COUNT - 1; i >= 0; i--) {
		ArenaRelease(tl_ThreadContext->arenas[i]);
	}

	tl_ThreadContext = NULL;
}

ThreadContext* ThreadContextGet() {
	return tl_ThreadContext;
}
