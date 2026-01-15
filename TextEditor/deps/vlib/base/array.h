#pragma once
#include <stdio.h>
#include <string.h>

#include "../general.h"
#include "arena.h"

// TODO:
// logging out of bounds? bounds check?
// ArrayPushArray
// ArrayRemoveAt
// ArrayRemoveAtFast
// ArrayInit (with array)
//
//	заменить на алгоритм сортировки на более быстрый

#define ARRAY_GROWTH_FACTOR 2

struct _array_header_ {
	void* items;
	i64 count, capacity;
};

template <typename T>
struct Array {
	T* items;
	i64 count, capacity;

	T& operator[](i64 idx) { return items[idx]; }
	const T& operator[](i64 idx) const { return items[idx]; }
};

void _ArrayShift(Arena* arena, _array_header_* header, i64 idx, i64 count, i64 itemSize, i64 align);
void _ArrayGrow(Arena* arena, _array_header_* header, i64 count, i64 itemSize, i64 align, i64 growthFactor = ARRAY_GROWTH_FACTOR);
void _ArraySort(_array_header_* header, i64 itemSize, i64 align, bool (*compare)(void*, void*), bool descending = false);

template <typename T>
bool ArrayIdxIsValid(Array<T>* array, i64 idx) {
	return (idx >= 0 && idx < array->count);
}

template <typename T, typename F>
void ArrayForeach(Array<T>* array, F func) {
	for (i64 i = 0; i < array->count; i++) {
		func(&array->items[i]);
	}
}

template <typename T>
void ArrayReserve(Arena* arena, Array<T>* array, i64 count) {
	_ArrayGrow(arena, (_array_header_*)array, count, sizeof(T), alignof(T), 1);
}

// count - сколько элементов будет добавлено
// не изменяет count массива, только capacity
// умножет capacity на ARRAY_GROWTH_FACTOR
template <typename T>
void ArrayGrow(Arena* arena, Array<T>* array, i64 count) {
	_ArrayGrow(arena, (_array_header_*)array, count, sizeof(T), alignof(T), ARRAY_GROWTH_FACTOR);
}

// увеличивает array->count на значение агрумента count
template <typename T>
void ArrayShift(Arena* arena, Array<T>* array, i64 idx, i64 count) {
	_ArrayShift(arena, (_array_header_*)array, idx, count, sizeof(T), alignof(T));
}

// Push

template <typename T>
void ArrayPush(Arena* arena, Array<T>* dest, T* items, i64 count) {
	ArrayGrow(arena, dest, count);
	Copy(&dest->items[dest->count], items, count);
	dest->count += count;
}

template <typename T>
void ArrayPush(Arena* arena, Array<T>* dest, Array<T>* src) {
	ArrayPush(arena, dest, src->items, src->count);
}

template <typename T>
void ArrayPush(Arena* arena, Array<T>* array, T item) {
	ArrayPush(arena, array, &item, 1);
}

// Insert

template <typename T>
void ArrayInsert(Arena* arena, Array<T>* array, T* items, i64 count, i64 idx) {
	if (idx >= 0 && idx <= array->count) {	 // one past last index is valid
		ArrayShift(arena, array, idx, count);	 // no need for array->count += count. ArrayShift increments count
		Copy(&array->items[idx], items, count);
	}
}

template <typename T>
void ArrayInsert(Arena* arena, Array<T>* dst, Array<T>* src, i64 idx) {
	ArrayInsert(arena, dst, src->items, src->count, idx);
}

template <typename T>
void ArrayInsert(Arena* arena, Array<T>* array, T item, i64 idx) {
	ArrayInsert(arena, array, &item, 1, idx);
}

// Remove

// TODO: переместить логику в универсальную функцию и вызывать ее в темплейтах

// перемещает все элементы после idx на 1 влево
template <typename T>
void ArrayRemove(Array<T>* array, i64 idx) {
	if (ArrayIdxIsValid(array, idx)) {
		for (i64 i = idx; i < array->count - 1; i++) {
			array->items[i] = array->items[i + 1];
		}
		array->count--;
	}
}

// для удаления меняет порядок элементов
template <typename T>
void ArrayRemoveFast(Array<T>* array, i64 idx) {
	if (ArrayIdxIsValid(array, idx)) {
		// для удаления последнего элемента достаточно уменьшить count
		i64 lastItemIdx = array->count - 1;
		if (idx != lastItemIdx) {
			array->items[idx] = array->items[lastItemIdx];
		}
		array->count--;
	}
}

// Sort

// медленная сортировка для тестовых целей
// compareFunc должна возвращать true, если A < B
template <typename T>
void ArraySortAsc(Array<T>* array, bool (*compareFunc)(T*, T*)) {
	_ArraySort((_array_header_*)array, sizeof(T), alignof(T), (bool (*)(void*, void*))compareFunc, false);
}

template <typename T>
void ArraySortDesc(Array<T>* array, bool (*compareFunc)(T*, T*)) {
	_ArraySort((_array_header_*)array, sizeof(T), alignof(T), (bool (*)(void*, void*))compareFunc, true);
}
