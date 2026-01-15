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

void MemCopy(void* destination, const void* source, s64 size) {
	if (destination && source) {
		u8* dest = (u8*)destination;
		u8* src = (u8*)source;

		while (size--) {
			*dest++ = *src++;
		}
	}
}

void MemSet(void* destination, u8 value, s64 count) {
	u8* dest = (u8*)destination;

	while (count--) {
		*dest++ = value;
	}
}

s64 StrLen(const char* str) {
	const char* p = str;
	while (*p != '\0') {
		p++;
	}
	return p - str;
}

s64 StrLen(const unsigned char* str) {
	return StrLen((const char*)str);
}

bool StrEqual(const char* strA, const char* strB) {
	s64 lenA = StrLen(strA);
	s64 lenB = StrLen(strB);

	if (lenA != lenB)
		return false;

	for (s64 i = 0; i < lenA; i++) {
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
	s64 lenA = StrLen(strA);
	s64 lenB = StrLen(strB);

	s64 count = 0;
	for (s64 i = 0; i < lenA && i < lenB; i++) {
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

s64 Abs(s64 value) {
	if (value < 0)
		return -value;
	return value;
}

f32 Pow(f32 x, s32 y) {
	if (y < 0)
		return 1.0f / Pow(x, -y);

	f32 result = 1.0;
	for (s32 i = 0; i < y; i++)
		result *= x;

	return result;
}