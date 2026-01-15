#pragma once
#include "../general.h"

// TODO:
// chained arena

#define ARENA_DEFAULT_ALIGNMENT (sizeof(void*) * 2)

#define ARENA_COMMIT_SIZE Kilobytes(64)	 // TODO: сделать переменной платформы
#define ARENA_DEFAULT_COMMIT (ARENA_COMMIT_SIZE)
#define ARENA_DEFAULT_RESERVE (Gigabytes(64))

#define ARENA_GROWTH_FACTOR 1

//
// Arena
//

struct Arena {
	u8* base;
	i64 used, commited, reserved;
};

#define ARENA_HEADER_SIZE (sizeof(Arena))

#define ArenaSpaceLeft(arena) ((arena)->reserved - (arena)->used)

Arena* ArenaAlloc(i64 commit = ARENA_DEFAULT_COMMIT, i64 reserve = ARENA_DEFAULT_RESERVE);
void ArenaRelease(Arena* arena);

#define ArenaReallocArray(arena, array, oldCount, newCount, type) \
	(type*)_ArenaRealloc((arena), (array), (oldCount) * sizeof(type), (newCount) * sizeof(type), alignof(type), false);
#define ArenaReallocArrayZero(arena, array, oldCount, newCount, type) \
	(type*)_ArenaRealloc((arena), (array), (oldCount) * sizeof(type), (newCount) * sizeof(type), alignof(type), true);
void* _ArenaRealloc(Arena* arena, const void* data, i64 oldSize, i64 newSize, i64 alignment, bool clearToZero);

#define ArenaPush(arena, size) _ArenaPush((arena), (size), ARENA_DEFAULT_ALIGNMENT, false);
#define ArenaPushAligned(arena, size, align) _ArenaPush((arena), (size), align, false);
#define ArenaPushZero(arena, size) _ArenaPush((arena), (size), ARENA_DEFAULT_ALIGNMENT, true);
#define ArenaPushStruct(arena, type) (type*)_ArenaPush((arena), sizeof(type), alignof(type), true);
#define ArenaPushArray(arena, count, type) (type*)_ArenaPush((arena), sizeof(type) * (count), alignof(type), false);
#define ArenaPushArrayZero(arena, count, type) (type*)_ArenaPush((arena), sizeof(type) * (count), alignof(type), true);
void* _ArenaPush(Arena* arena, i64 size, i64 alignment, bool clearToZero);

// Выделить структуру, владеющую ареной
template <typename T>
T* ArenaBoostrapStruct(i64 commit = ARENA_DEFAULT_COMMIT, i64 reserve = ARENA_DEFAULT_RESERVE) {
	Arena* arena = ArenaAlloc(commit, reserve);
	assert(ArenaSpaceLeft(arena) >= sizeof(T));
	T* s = ArenaPushStruct(arena, T);
	s->arena = arena;
	return s;
}

void ArenaClear(Arena* arena);

//
// Scratch Arena
//

struct Scratch {
	Arena* arena;
	i64 pos;

	~Scratch();
};

Scratch ArenaGetScratch(Arena* arena);

// в conflicts необходимо передать арены, исполюзущиеся в течении действия srcatch-арены для других аллокаций
// позволяет избежать ошибки при создании вложенных scratch-арен
// https://www.rfleury.com/p/untangling-lifetimes-the-arena-allocator
Scratch GetScratch(Arena** conflicts = 0, i64 conflictsCount = 0);
