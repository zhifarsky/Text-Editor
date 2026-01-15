#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "general.h"
#include "tools/profiler.h"

void _LogError(const char* funcName, i32 line, const char* format, ...) {
	printf("[ERROR] %s (line %d) ", funcName, line);
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	printf("\n");
}

void _LogWarning(const char* funcName, i32 line, const char* format, ...) {
	printf("[WARNING] %s (line %d) ", funcName, line);
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	printf("\n");
}

bool IsPowerOfTwo(i64 value) {
	assert(value >= 0);
	return (value & (value - 1)) == 0;
}

i64 RoundToPowerOfTwo(i64 value) {
	return pow(2, ceil(log2(value)));
}

i64 RoundToMultiple(i64 value, i64 multiple) {
	assert(value >= 0);
	return ((value + multiple - 1) / multiple) * multiple;
}

i64 MemoryGetForwardPadding(void* memory, i64 alignment) {
	assert(alignment >= 0);
	return -(i64)memory & (alignment - 1);
}

void GeneratePowTable(u64* table, i64 count, u64 p) {
	u64 pPow = 1;
	for (i64 i = 0; i < count; i++) {
		table[i] = pPow;
		pPow *= p;
	}
}

// TODO: SIMD?
u64 Hash(const void* data, i64 size) {
	ProfileFunction();

	// p - простое чило, должно быть больше макс. значения символа
	// выбираем значение больше чем 255, т.к. работаем с байтами вместо символов
	// m - должно быть простым числом
	// можно не делить по модулю m, если работаем в u64
	// u64 m = 1e9 + 7;

	u64 p = 257;

	u64 hash = 0;
	u8* bytes = (u8*)data;
	for (i64 i = size - 1; i >= 0; i--) {
		hash = hash * p + bytes[i];
	}

	return hash;
}