#pragma once
#include "../base/string.h"
#include "../general.h"

void* os_MemReserve(i64 size);
void os_MemCommit(void* memory, i64 size);
void os_MemFree(void* memory);

u64 os_GetRDTSC();
u64 os_GetTimerFreq();
u64 os_ReadTimer();
i32 os_GetCPUCount();
void os_Sleep(i64 ms);

//
// Files
//

i64 os_GetFileSize(String path);
i64 os_ReadEntireFile(void* buf, String path);
u8* os_ReadEntireFile(Arena* arena, String path, u64* outBytesRead = NULL);

//
// Threads
//

i64 os_AtomicIncI64(volatile i64* value);
