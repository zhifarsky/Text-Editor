#include "../platform/platform.h"
#include "../tools/profiler.h"

#include "arena.h"

#include "thread_context.h"

//
// Arena
//

// ВАЖНО:
// размер инициализированной арены не должен быть меньше ARENA_HEADER_SIZE,
// т.к. иначе при ArenaPush() будет перезаписываться заголовок

Arena* ArenaAlloc(i64 commit, i64 reserve) {
	reserve = RoundToMultiple(reserve, ARENA_COMMIT_SIZE);
	commit = RoundToMultiple(commit, ARENA_COMMIT_SIZE);
	assert(commit <= reserve);
	assert(commit >= sizeof(Arena));

	u8* base = (u8*)os_MemReserve(reserve);
	os_MemCommit(base, commit);

	Arena* arena = (Arena*)base;
	*arena = {
			.base = base,
			.used = ARENA_HEADER_SIZE,
			.commited = commit,
			.reserved = reserve,
	};

	return arena;
}

void ArenaRelease(Arena* arena) {
	if (arena) {
		void* base = arena->base;
		*arena = {0};			 // TODO: все равно остаются мусорные значения?
		os_MemFree(base);	 // проверка на NULL уже есть в MemFree()
	}
}

void* _ArenaPush(Arena* arena, i64 size, i64 alignment, bool clearToZero) {
	ProfileFunction();
	assert(IsPowerOfTwo(alignment));

	u8* result = arena->base + arena->used;
	i64 padding = MemoryGetForwardPadding(result, alignment);

	result += padding;

	// commit memory
	if (result + size >= arena->base + arena->commited) {
		i64 commitSize = RoundToMultiple((size + padding) * ARENA_GROWTH_FACTOR, ARENA_COMMIT_SIZE);
		assert(arena->reserved >= arena->commited + commitSize && "Arena overflow. Increase reserve size.");

		os_MemCommit(arena->base + arena->commited, commitSize);
		arena->commited += commitSize;
	}

	arena->used += size + padding;

	if (clearToZero) {
		MemZero(result, size);
	}

	return result;
}

void* _ArenaRealloc(Arena* arena, const void* mem, i64 oldSize, i64 newSize, i64 alignment, bool clearToZero) {
	ProfileFunction();
	void* newMem = _ArenaPush(arena, newSize, alignment, false);
	memcpy(newMem, mem, oldSize);
	if (clearToZero) {
		MemZero((u8*)newMem + oldSize, newSize - oldSize);
	}
	return newMem;
}

void ArenaClear(Arena* arena) {
	if (arena) {
		arena->used = ARENA_HEADER_SIZE;
	}
}

//
// Scratch Arena
//

Scratch::~Scratch() {
	arena->used = pos;
}

Scratch ArenaGetScratch(Arena* arena) {
	return {
			.arena = arena, .pos = arena->used};
}

Scratch GetScratch(Arena** conflicts, i64 conflictsCount) {
	ThreadContext* tc = ThreadContextGet();

	i32 arenaIndex = -1;

	// если переданы конфликтующие арены, ищем арену, которой нет среди конфликтущих
	for (size_t i = 0; i < TC_ARENAS_COUNT; i++) {
		bool conflictFound = false;
		for (size_t j = 0; j < conflictsCount; j++) {
			if (tc->arenas[i] == conflicts[j]) {
				conflictFound = true;
				break;
			}
		}

		if (!conflictFound) {
			arenaIndex = i;
			break;
		}
	}

	if (arenaIndex == -1) {
		assert(false && "Arena not found");
		return {0};
	}

	return ArenaGetScratch(tc->arenas[arenaIndex]);
}
