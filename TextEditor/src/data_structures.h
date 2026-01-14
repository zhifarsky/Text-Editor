#pragma once
#include "editor.h"
#include "tools.h"
#include "input.h"

void platform_Print(const char* msg);
void* platform_debug_Malloc(s64 size);
void platform_debug_Free(void* memory);
void* MemReserve(s64 size);
void MemCommit(void *memory, s64 size); 

#define GROWTH_FACTOR 2

//
// Event Queue
//

enum event_type : u8 {
	Event_None = 0,
	Event_Char,
	Event_Key
};

#define EVENT_HEADER event_type eventType;

struct char_event {
	EVENT_HEADER
	code_point utf8CodePoint;
	bool wasDown, isDown;
};

char_event CharEvent(code_point utf8CodePoint, bool wasDown, bool isDown) {
	char_event event;
	event.eventType = Event_Char;
	event.utf8CodePoint = utf8CodePoint;
	event.wasDown = wasDown;
	event.isDown = isDown;
	return event;
}

struct key_event {
	EVENT_HEADER
	te_Key key;
	bool wasDown, isDown;
};

key_event KeyEvent(te_Key key, bool wasDown, bool isDown) {
	key_event event;
	event.key = key;
	event.eventType = Event_Key;
	event.isDown = isDown;
	event.wasDown = wasDown;
	return event;
}

struct event_queue {
	void *base;
	s64 size, capacity;
};

event_queue EventQueue(void* base, s64 capacity) {
	event_queue queue;
	queue.base = base;
	queue.size = 0;
	queue.capacity = capacity;
	return queue;
}

#define PUSH_EVENT(queue, event) Push(&queue, &event, sizeof(event))
void Push(event_queue* queue, void* data, s64 size) {
	if (queue->size + size <= queue->capacity) {
		MemCopy(&((u8*)queue->base)[queue->size], data, size);
		queue->size += size;
	}
}

void Clear(event_queue *queue) {
	queue->size = 0;
}

//
// Arena
//

// TODO: посмотреть про temp arena, scratch arena, free list

#define DEFAULT_ALIGNMENT 16 // TODO: какое значение лучше?
#define DEFAULT_RESERVE Gigabytes(64)
#define COMMIT_SIZE Kilobytes(64)

#define RoundToMultiple(number, multiple) (((number) + (multiple) - 1) / (multiple)) * (multiple)

struct memory_arena {
	u8* base;
	s64 size, commited, reserved;
};

// memory_arena Arena(void* base, s64 capacity, s64 commited, s64 reserved = DEFAULT_RESERVE) {
// 	memory_arena arena;
// 	arena.base = (u8*)base;
// 	arena.size = 0;
// 	arena.capacity = capacity;
// 	return arena;
// }

memory_arena ArenaAlloc(s64 commit, s64 reserve = DEFAULT_RESERVE) {
	memory_arena arena;
	arena.reserved = RoundToMultiple(reserve, COMMIT_SIZE);
	arena.commited = RoundToMultiple(commit, COMMIT_SIZE);
	
	te_assert(arena.commited <= arena.reserved);
	
	arena.base = (u8*)MemReserve(arena.reserved);
	MemCommit(arena.base, arena.commited);
	
	arena.size = 0;
	return arena;
}

void ArenaRelease(memory_arena* arena) {
	platform_debug_Free(arena->base);
	ZeroStruct(*arena);
}

#define ArenaPush(arena, size) _ArenaPush(arena, size, DEFAULT_ALIGNMENT, true)
#define ArenaPushStruct(arena, type) _ArenaPush(arena, sizeof(type), alignof(type), true)
#define ArenaPushZeroArray(arena, count, type) _ArenaPush(arena, sizeof(type) * (count), alignof(type), true)
#define ArenaPushArray(arena, count, type) _ArenaPush(arena, sizeof(type) * (count), alignof(type), false)
void* _ArenaPush(memory_arena* arena, s64 size, s64 alignment, bool clearToZero) {
	u8* result = arena->base + arena->size;
	s64 padding = -(s64)result & (alignment - 1); // работает только со степенями двойки
	
	//te_assert(arena->size + size + padding <= arena->capacity);
	
	result += padding;
	
	// commit memory
	if (result + size >= arena->base + arena->commited) {
		s64 commitSize = RoundToMultiple(size + padding, COMMIT_SIZE);
		te_assert(arena->reserved >= arena->commited + commitSize);
		
		MemCommit(arena->base + arena->commited, commitSize);
		arena->commited += commitSize;
	}
	
	arena->size += size + padding;
	
	if (clearToZero)
		MemZero(result, size);
	
	return result;
}

#define ArenaReallocArray(arena, array, oldCount, newCount, type) _ArenaRealloc(arena, array, oldCount * sizeof(type), newCount * sizeof(type), alignof(type), true)
void* _ArenaRealloc(memory_arena* arena, void* memory, s64 size, s64 newSize, s64 alignment, bool clearToZero) {
	void* newMemory = _ArenaPush(arena, newSize, alignment, false);
	MemCopy(newMemory, memory, size);

	if (clearToZero)
		MemZero((u8*)newMemory + size, newSize - size);

	return newMemory;
}

void ArenaClear(memory_arena* arena) {
	arena->size = 0;
}

struct temp_memory_arena {
	memory_arena* arena;
	s64 pos;
};

temp_memory_arena TempArenaBegin(memory_arena* arena) {
	temp_memory_arena tempArena;
	tempArena.arena = arena;
	tempArena.pos = arena->size;
	return tempArena;
}

void TempArenaEnd(temp_memory_arena tempArena) {
	tempArena.arena->size = tempArena.pos;
}

//
// String
//

struct string {
	char* base;
	s64 size;

	char& operator[](s64 i) {
		return base[i];
	} 
};

#define StrFirst(str) (str.base)
#define StrLast(str) (str.base + str.size - 1)

string String(const char* cstr) {
	string str;
	str.base = (char*)cstr;
	str.size = StrLen(cstr);
	return str;
}

string String(char* base, s64 size) {
	string str;
	str.base = base;
	str.size = size;
	return str;
}

struct string_builder {
	string buffer;
	s64 capacity;
};

void Realloc(memory_arena* arena, string_builder* builder, s64 newCapacity) {
	builder->capacity = newCapacity;
	char* newMem = (char*)ArenaPushArray(arena, builder->capacity, char);
	MemCopy(newMem, builder->buffer.base, builder->buffer.size);
	builder->buffer.base = newMem;
}

void IntAppend(memory_arena* arena, string_builder* builder, s64 value) {    
	s64 n = 0, v = Abs(value);
	while(v > 0) {
		v /= 10;
		n++;
	}
	
	n = te_Max(1, n);
	s64 divider = Pow(10, n - 1);

	s64 length = value < 0 ? n + 1 : n;
	
	// realloc
	if (builder->buffer.size + length > builder->capacity) {
		Realloc(arena, builder, (builder->capacity + length) * GROWTH_FACTOR);
	}
	
	char* p = builder->buffer.base + builder->buffer.size;
	
	if (value < 0) {
		*p = '-';
		p++;
	}
	
	v = Abs(value);
		
	for (s64 i = 0; i < n; i++)
	{
		*p = '0' + (v / divider % 10);
		divider /= 10;
		p++;
	}
	
	builder->buffer.size += length;
}

void StrAppend(memory_arena* arena, string_builder* builder, string str) {
	// realloc
	if (builder->buffer.size + str.size > builder->capacity) {
		Realloc(arena, builder, (builder->capacity + str.size) * GROWTH_FACTOR);
	}
	
	MemCopy(builder->buffer.base + builder->buffer.size, str.base, str.size);
	builder->buffer.size += str.size;
}

void StrAppend(memory_arena* arena, string_builder* builder, const char* cstr) {
	StrAppend(arena, builder, String(cstr));
}

void ToCString(char* buffer, s64 bufferSize, string* str) {
	if (bufferSize < str->size + 1)
		return;
		
	MemCopy(buffer, str->base, str->size);
	buffer[str->size] = 0;
}

void ToCString(char* buffer, s64 bufferSize, string_builder* builder) {
	ToCString(buffer, bufferSize, &builder->buffer);
}

//
// Dynamic Array
//

#define ARRAY_DEFAULT_MAX_COUNT 512

template <typename T>
struct array_dynamic {
	T* items;
	s64 count, maxCount;
	
	T& operator[](s64 index) { return items[index]; }
	const T& operator[](s64 index) const { return items[index]; }
};

// template <typename T>
// array_dynamic<T> Array(u64 maxCount = ARRAY_DEFAULT_MAX_COUNT) {
//     array_dynamic<T> array = {0};
//     array.items = (T*)platform_debug_Malloc(maxCount * sizeof(T));
//     array.maxCount = maxCount;
//     return array;
// }

// template <typename T>
// void Push(array_dynamic<T>* array, const T& item) {
//     if (array->count + 1 > array->maxCount) {
//         array->items = (T*)platform_debug_Realloc(array->items, array->count * sizeof(T), (array->maxCount + 1) * GROWTH_FACTOR);
//     }
	
//     array->items[array->count++] = item;
// }

template <typename T>
array_dynamic<T> Array(memory_arena *arena, s64 maxCount = ARRAY_DEFAULT_MAX_COUNT) {
	array_dynamic<T> array = {0};
	array.items = (T*)ArenaPushArray(arena, maxCount, T);
	array.maxCount = maxCount;
	return array;
}    

template <typename T>
void Push(array_dynamic<T>* array, memory_arena* arena, const T& item) {
	s64 newCount = array->count + 1;
	if (newCount > array->maxCount) {
		s64 newMaxCount = newCount * GROWTH_FACTOR;
		array->items = (T*)ArenaReallocArray(arena, array->items, array->count, newMaxCount, T);
		array->maxCount = newMaxCount;
	}
	
	array->items[array->count] = item;
	array->count = newCount;
}

template <typename T>
void Insert(array_dynamic<T>* array, memory_arena* arena, const T& item, s64 index) {
	if (index > array->count)
		return;
	
	s64 newCount = array->count + 1;
	if (newCount > array->maxCount) {
		s64 newMaxCount = newCount * GROWTH_FACTOR;
		array->items = (T*)ArenaReallocArray(arena, array->items, array->count, newMaxCount, T);
		array->count = newMaxCount;
	}
		
	for (s64 i = array->count; i > index; i--) {
		array->items[i] = array->items[i - 1]; 
	}
	array->items[index] = item;
	array->count = newCount;
}

template <typename T>
void Remove(array_dynamic<T>* array, s64 index) {
	if (index >= array->count)
		return;
	
	for (s64 i = index; i < array->count - 1; i++) {
		array->items[i] = array->items[i+1];
	}
	
	array->count--;
}

// меняет порядок элементов при удалении
template <typename T>
void RemoveFast(array_dynamic<T>* array, s64 index) {
	if (index >= array->count)
		return;
	
	// last
	if (index == array->count - 1) {
		array->count--;
		return;
	}
		
	array->items[index] = array->items[array->count - 1];	
	array->count--;
}