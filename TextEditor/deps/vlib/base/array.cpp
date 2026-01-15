#include <cstring>

#include "../tools/profiler.h"
#include "arena.h"
#include "array.h"

void _ArrayGrow(Arena* arena, _array_header_* header, i64 count, i64 itemSize, i64 align, i64 growthFactor) {
	ProfileFunction();
	i64 newCount = header->count + count;
	if (newCount > header->capacity) {
		i64 newCapacity = newCount * growthFactor;
		header->items = _ArenaRealloc(arena, header->items, header->count * itemSize, newCapacity * itemSize, align, true);
		header->capacity = newCapacity;
	}
}

void _ArrayShift(Arena* arena, _array_header_* header, i64 idx, i64 count, i64 itemSize, i64 align) {
	_ArrayGrow(arena, header, count, itemSize, align);
	u8* ptr = (u8*)header->items;
	memmove(ptr + (idx + count) * itemSize,
					ptr + idx * itemSize,
					(header->count - idx) * itemSize);
	header->count += count;
}

void _ArraySort(_array_header_* header, i64 itemSize, i64 align, bool (*compare)(void*, void*), bool descending) {
	if (header->count < 2)
		return;

	Scratch scrath = GetScratch();
	void* temp = ArenaPushAligned(scrath.arena, itemSize, align);
	u8* base = (u8*)header->items;

	for (i64 i = 0; i < header->count; i++) {
		for (i64 j = 0; j < header->count - i - 1; j++) {
			u8* first = base + (j * itemSize);
			u8* second = first + itemSize;

			bool doSwap = descending ? compare(first, second) : compare(second, first);

			if (doSwap) {
				memcpy(temp, second, itemSize);
				memcpy(second, first, itemSize);
				memcpy(first, temp, itemSize);
			}
		}
	}
}
