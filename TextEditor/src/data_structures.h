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
