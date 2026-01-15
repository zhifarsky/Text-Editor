#pragma once
#include "editor.h"

#if _DEBUG
#define te_assert(Expression) \
	if (!(Expression)) {        \
		*(int*)0 = 0;             \
	}
#else
#define te_assert(Expression)
#endif

#define InRange(value, min_v, max_v) (((value) >= (min_v)) && ((value) <= (max_v)))

void MemCopy(void* destination, const void* source, i64 size) {
	if (destination && source) {
		u8* dest = (u8*)destination;
		u8* src = (u8*)source;

		while (size--) {
			*dest++ = *src++;
		}
	}
}

void MemSet(void* destination, u8 value, i64 count) {
	u8* dest = (u8*)destination;

	while (count--) {
		*dest++ = value;
	}
}

i64 StrLen(const char* str) {
	const char* p = str;
	while (*p != '\0') {
		p++;
	}
	return p - str;
}

i64 StrLen(const unsigned char* str) {
	return StrLen((const char*)str);
}

bool StrEqual(const char* strA, const char* strB) {
	i64 lenA = StrLen(strA);
	i64 lenB = StrLen(strB);

	if (lenA != lenB)
		return false;

	for (i64 i = 0; i < lenA; i++) {
		if (strA[i] != strB[i])
			return false;
	}

	return true;
}

// работает только с ascii
char ToLower(char c) {
	if (c >= 'A' && c <= 'Z')
		c += 'a' - 'A';
	return c;
}

enum str_find_flags_ {
	StrFind_ToLower = 1 << 0
};
typedef int str_find_flags;

// сколько символов strB входит в strA с начала строки
bool StrFind(const char* strA, const char* strB, str_find_flags flags = 0) {
	i64 lenA = StrLen(strA);
	i64 lenB = StrLen(strB);

	i64 count = 0;
	for (i64 i = 0; i < lenA && i < lenB; i++) {
		char a = strA[i];
		char b = strB[i];
		if (flags & StrFind_ToLower) {
			a = ToLower(a);
			b = ToLower(b);
		}

		if (a == b) {
			count++;
		} else
			break;
	}
	return count;
}

#define te_Min(a, b) ((a) < (b) ? (a) : (b))
#define te_Max(a, b) ((a) > (b) ? (a) : (b))

i64 Abs(i64 value) {
	if (value < 0)
		return -value;
	return value;
}

f32 Pow(f32 x, i32 y) {
	if (y < 0)
		return 1.0f / Pow(x, -y);

	f32 result = 1.0;
	for (i32 i = 0; i < y; i++)
		result *= x;

	return result;
}