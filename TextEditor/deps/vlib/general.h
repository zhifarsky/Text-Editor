#pragma once

#include <assert.h>
#include <stdint.h>
#include <string.h>

#if defined(_MSC_VER)
#define MSVC_COMPILER 1
#elif defined(__clang__)
#define CLANG_COMPILER 1
#elif defined(__GNUC__) || defined(__GNUG__)
#define GCC_COMPILER 1
#else
#error "Unknown compiler"
#endif

#if defined(_WIN32)
#define WIN_BUILD 1
#elif defined(__linux__)
#define LINUX_BUILD 1
#elif defined(__APPLE__)
#define MACOS_BUILD 1
#else
#error "Unknown OS target"
#endif

#if defined(_DEBUG)
#pragma message("[VLIB] Debug build.")
#define DEBUG_BUILD 1
#elif defined(NDEBUG)
#pragma message("[VLIB] Release build.")
#define RELEASE_BUILD 1
#else
#warning "[VLIB] Warning: neither _DEBUG nor NDEBUG defined. Treating as debug build."
#define DEBUG_BUILD_ 1
#endif

#define global_variable inline

#if MSVC_COMPILER
#define thread_static __declspec(thread)
#elif GCC_COMPILER
#define thread_static __thread
#endif

typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;

typedef float f32;
typedef double f64;

#define Kilobytes(n) ((n) * 1024LL)
#define Megabytes(n) (Kilobytes(n) * 1024LL)
#define Gigabytes(n) (Megabytes(n) * 1024LL)

#define Hundreds(n) ((n) * 100)
#define Thousands(n) ((n) * 1000)
#define Millions(n) (Thousands(n) * 1000)
#define Billions(n) (Millions(n) * 1000)

#define ArrayCount(array) (sizeof(array) / sizeof((array)[0]))
#define StringsEqual(strA, strB) (strcmp((strA), (strB)) == 0)

#define ZeroStruct(dest) (memset(&(dest), 0, sizeof(dest)))
#define MemZero(dest, size) (memset((dest), 0, (size)))

#define NameConcat2(a, b) a##b
#define NameConcat(a, b) NameConcat2(a, b)

template <typename T>
T* Copy(T* dst, T* src, i64 count) {
	return (T*)memcpy(dst, src, count * sizeof(T));
}

void _LogError(const char* funcName, i32 line, const char* format, ...);
void _LogWarning(const char* funcName, i32 line, const char* format, ...);
#define LogError(format, ...) _LogError(__FUNCTION__, __LINE__, format, ##__VA_ARGS__)
#define LogWarning(format, ...) _LogWarning(__FUNCTION__, __LINE__, format, ##__VA_ARGS__)

bool IsPowerOfTwo(i64 value);
i64 RoundToPowerOfTwo(i64 value);	 // медленная функция
i64 RoundToMultiple(i64 value, i64 multiple);
i64 MemoryGetForwardPadding(void* memory, i64 alignment);	 // работает только со степенями двойки

// полиномиальный хеш
u64 Hash(const void* data, i64 size);

#define Mod(a, b) (IsPowerOfTwo(b) ? ((a) & ((b) - 1)) : ((a) % (b)))
