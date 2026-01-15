#pragma once
#include "../general.h"
#include "arena.h"
#include <cstdio>

// TODO:
// поддержка string в printf (неудобно каждый раз вызывать StrToCstr)
// поддержка форматирования (и %S для вывода string) для string_builder

//
// String
//

struct String {
	const char* base;
	i64 length;

	const char& operator[](i64 idx);
	bool operator==(String other) const;
	bool operator!=(String other) const;
	bool operator<(String other) const;	 // лексикографическое сравнение
};

String Str(char* base, i64 length);
String Str(const char* cstr);
String StrAlloc(Arena* arena, const char* base, i64 length);
String StrAlloc(Arena* arena, String str);
bool StrToCstr(String str, char* buffer, i64 bufferSize);
const char* StrToCstr(Arena* arena, String str);

const char* StrFirst(String str);
const char* StrLast(String str);

u64 GetHash(String str);

//
// String Builder
//

// инициализируется занулением
// string_builder sb = {0};
struct StrBuilder {
	String buffer;
	i64 capacity;
};

void StrRealloc(Arena* arena, StrBuilder* sb, i64 newCapacity);
void StrAppend(Arena* arena, StrBuilder* sb, char c);
void StrAppend(Arena* arena, StrBuilder* sb, String str);
void StrAppend(Arena* arena, StrBuilder* sb, const char* cstr);
const char* StrToCstr(Arena* arena, StrBuilder* sb);	// выделяет в арене память под строку

String StrBuildF(Arena* arena, const char* format, ...);
