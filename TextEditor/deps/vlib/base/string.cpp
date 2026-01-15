#include "string.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define STR_FORMAT_LIMIT 1024

//
// String
//

// TODO: what if out of bounds?
const char& String::operator[](i64 idx) {
	return base[idx];
}

bool String::operator==(String other) const {
	if (this->length != other.length)
		return false;

	return (memcmp(this->base, other.base, this->length)) == 0;
}

bool String::operator!=(String other) const {
	return !(*this == other);
}

bool String::operator<(String other) const {
	for (i64 i = 0; i < this->length && i < other.length; i++) {
		if (this->base[i] < other.base[i]) {
			return true;
			
		}
		else if (this->base[i] > other.base[i]) {
			return false;
			
		}
	}

	if (this->length < other.length) {
		return true;
	}
	else if (this->length > other.length) {
		return false;
		
	}

	return false;
}

String Str(char* base, i64 length) {
	return {.base = (char*)base, .length = length};
}

String Str(const char* cstr) {
	return Str((char*)cstr, (i64)strlen(cstr));
}

String StrAlloc(Arena* arena, const char* base, i64 length) {
	String str;
	str.length = length;
	str.base = ArenaPushArray(arena, length, char);
	memcpy(const_cast<char*>(str.base), base, length);
	return str;
}

String StrAlloc(Arena* arena, String str) {
	return StrAlloc(arena, str.base, str.length);
}

bool StrToCstr(String str, char* buffer, i64 bufferSize) {
	if (str.length < bufferSize) {
		memcpy(buffer, str.base, str.length);
		buffer[str.length] = 0;
		return true;
	}
	return false;
}

const char* StrToCstr(Arena* arena, String str) {
	char* cstr = ArenaPushArray(arena, str.length + 1, char);
	memcpy(cstr, str.base, str.length);
	cstr[str.length] = 0;
	return cstr;
}

u64 GetHash(String str) {
	return Hash(str.base, str.length);
}

const char* StrFirst(String str) {
	return &str[0];
}
const char* StrLast(String str) {
	return &str[str.length - 1]; // при длине 0 индекс будет -1
}

//
// String Builder
//

#define STRING_BUILDER_GROWTH_FACTOR 2

void StrRealloc(Arena* arena, StrBuilder* sb, i64 newCapacity) {
	sb->capacity = newCapacity;
	sb->buffer.base = ArenaReallocArray(arena, sb->buffer.base, sb->buffer.length, sb->capacity, char);
}

void StrAppend(Arena* arena, StrBuilder* sb, char c) {
	// realloc
	if (sb->buffer.length + 1 > sb->capacity) {
		StrRealloc(arena, sb, (sb->capacity + 1) * STRING_BUILDER_GROWTH_FACTOR);
	}

	((char*)sb->buffer.base)[sb->buffer.length] = c;
	sb->buffer.length++;
}

void StrAppend(Arena* arena, StrBuilder* sb, String str) {
	// realloc
	if (sb->buffer.length + str.length > sb->capacity) {
		StrRealloc(arena, sb, (sb->capacity + str.length) * STRING_BUILDER_GROWTH_FACTOR);
	}

	memcpy((char*)(sb->buffer.base + sb->buffer.length), str.base, str.length);
	sb->buffer.length += str.length;
}

void StrAppend(Arena* arena, StrBuilder* sb, const char* cstr) {
	StrAppend(arena, sb, Str(cstr));
}

const char* StrToCstr(Arena* arena, StrBuilder* sb) {
	char* cstr = ArenaPushArray(arena, sb->buffer.length + 1, char);
	memcpy(cstr, sb->buffer.base, sb->buffer.length);
	cstr[sb->buffer.length] = '\0';
	return cstr;
}

String StrBuildF(Arena* arena, const char* format, ...) {
	Scratch scratch = GetScratch(&arena, 1);
	char* tempBuf = ArenaPushArray(scratch.arena, STR_FORMAT_LIMIT, char);
	
	va_list args;
	va_start(args, format);
	i64 len = vsnprintf(tempBuf, STR_FORMAT_LIMIT, format, args);
	va_end(args);
	
	char* buf;
	if (len != -1) {
		buf = ArenaPushArray(arena, len, char);
		memcpy(buf, tempBuf, len);
	}
	else {
		LogError("vsnprintf() failed");
		len = 0;
		buf = 0;
	}
	
	return Str(buf, len);
}
