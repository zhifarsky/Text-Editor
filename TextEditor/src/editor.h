#pragma once
#include <stdint.h>

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;


#include "data_structures.h"

//
// Platform Services
// TODO: убрать префикс
//

void platform_Print(const char* message);

void platform_StartFrame();
void platform_EndFrame();

void* platform_debug_Malloc(s64 size);
void* platform_debug_Realloc(void* oldMem, s64 oldSize, s64 newSize);
void platform_debug_Free(void* memory);

void* MemReserve(s64 size);
void MemCommit(void* memory, s64 size);

//
// Editor services
//

void EditorUpdate(event_queue* eventQueue, program_input* input);
void EditorRender(program_input* input);
