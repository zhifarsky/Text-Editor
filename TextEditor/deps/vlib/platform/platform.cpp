#include "platform.h"
#include "../base/arena.h"
#include "../base/string.h"
#include "../tools/profiler.h"

// TODO:
// Выводить сведения об ошибке от ОС

#if WIN_BUILD
#include <windows.h>

void* os_MemReserve(i64 size) {
	ProfileFunction();
	void* res = VirtualAlloc(0, size, MEM_RESERVE, PAGE_READWRITE);
	assert(res);
	return res;
}

void os_MemCommit(void* memory, i64 size) {
	ProfileFunction();
	if (memory && size > 0) {
		void* res = VirtualAlloc(memory, size, MEM_COMMIT, PAGE_READWRITE);
		assert(res);
	}
}

void os_MemFree(void* memory) {
	ProfileFunction();
	if (memory) {
		bool res = VirtualFree(memory, 0, MEM_RELEASE);
		assert(res);
	}
}

u64 os_GetRDTSC() {
	return __rdtsc();
}

u64 os_GetTimerFreq() {
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	return freq.QuadPart;
}

u64 os_ReadTimer() {
	LARGE_INTEGER value;
	QueryPerformanceCounter(&value);
	return value.QuadPart;
}

i32 os_GetCPUCount() {
#if !MULTITHREADING
	SYSTEM_INFO sysinfo;
	GetSystemInfo(&sysinfo);
	return sysinfo.dwNumberOfProcessors;
#else
	return 1;
#endif
}

void os_Sleep(i64 ms) {
	Sleep(ms);
}

i64 os_GetFileSize(String path) {
	i64 size = 0;
	Scratch scratch = GetScratch();
	const char* cstr = StrToCstr(scratch.arena, path);

	HANDLE f = CreateFileA(cstr, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (f != INVALID_HANDLE_VALUE) {
		LARGE_INTEGER tempSize;
		if (GetFileSizeEx(f, &tempSize)) {
			size = tempSize.QuadPart;
		} else {
			LogError("Unable to get file size");
		}
		CloseHandle(f);
	} else {
		LogError("Unable to open file");
	}

	return size;
}

i64 os_ReadEntireFile(void* buf, String path) {
	i64 bytesReadTotal = 0;

	Scratch scratch = GetScratch();
	const char* cstr = StrToCstr(scratch.arena, path);

	i64 fileSize = os_GetFileSize(path);	// TODO: тоже преобразует строку и открывает файл. можно объеденить или перекидывать HANDLE
	i64 bytesRemaining = fileSize;

	if (fileSize) {
		HANDLE f = CreateFileA(cstr, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		if (f != INVALID_HANDLE_VALUE) {
			DWORD chunkSize = Gigabytes(1);
			DWORD bytesRead;

			while (bytesRemaining > 0) {
				DWORD bytesToRead = (bytesRemaining > chunkSize) ? chunkSize : bytesRemaining;

				if (ReadFile(f, (u8*)buf + bytesReadTotal, bytesToRead, &bytesRead, 0)) {
					bytesRemaining -= bytesRead;
					bytesReadTotal += bytesRead;
				} else {
					LogError("Unable to read file");
					break;
				}
			}

			CloseHandle(f);
		} else {
			LogError("Unable to open file");
		}
	} else {
		LogError("File is empty or it doesn't exist");
	}

	return bytesReadTotal;
}

u8* os_ReadEntireFile(Arena* arena, String path, u64* outBytesRead) {
	u8* buf = ArenaPushArray(arena, os_GetFileSize(path), u8);
	i64 bytesRead = os_ReadEntireFile(buf, path);
	if (outBytesRead) {
		*outBytesRead = bytesRead;
	}
	return buf;
}

i64 os_AtomicIncI64(volatile i64* value) {
	return InterlockedIncrement64(value);
}

#endif

#if LINUX_BUILD
#error "Platform layer not implemented"
#endif
