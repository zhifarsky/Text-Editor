#pragma once

#include "general.h"

//
// Platform Services
// TODO: убрать префикс
//

void platform_Print(const char* message);

void platform_StartFrame();
void platform_EndFrame();

void* platform_debug_Malloc(i64 size);
void* platform_debug_Realloc(void* oldMem, i64 oldSize, i64 newSize);
void platform_debug_Free(void* memory);

void* MemReserve(i64 size);
void MemCommit(void* memory, i64 size);

#include "input.h"
#include "tools.h"

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
	void* base;
	i64 size, capacity;
};

event_queue EventQueue(void* base, i64 capacity) {
	event_queue queue;
	queue.base = base;
	queue.size = 0;
	queue.capacity = capacity;
	return queue;
}

#define PUSH_EVENT(queue, event) Push(&queue, &event, sizeof(event))
void Push(event_queue* queue, void* data, i64 size) {
	if (queue->size + size <= queue->capacity) {
		MemCopy(&((u8*)queue->base)[queue->size], data, size);
		queue->size += size;
	}
}

void Clear(event_queue* queue) {
	queue->size = 0;
}

//
// Editor services
//

void EditorUpdate(event_queue* eventQueue, program_input* input);
void EditorRender(program_input* input);
