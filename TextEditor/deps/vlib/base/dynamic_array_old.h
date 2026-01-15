#if 0
#pragma once
#include "../general.h"


// TODO: попробовать вместо шаблонов

// 1. COG, Cheetah или другие инструменты

// 2. Linux kernel linked lists ixplained

// 3. DMS Software Reengineering Toolkit

// 4.
// Определяем макрос один раз в заголовочном файле
// #define DEFINE_ARRAY(type, name) \
//     struct name {                \
//         type* data;              \
//         size_t size;             \
//     };                           \
//     void name##_push(struct name* a, type value) { /* логика */ }

// // Использование:
// DEFINE_ARRAY(int, IntArray);
// DEFINE_ARRAY(float, FloatArray);

// int main() {
//     IntArray my_arr;
//     IntArray_push(&my_arr, 10); // Никаких угловых скобок
// }

//
// Dynamic Array
//

#define ARRAY_DEFAULT_MAX_COUNT 16
#define ARRAY_GROWTH_FACTOR 2

template <typename T>
struct dynamic_array {
	T* items;
	i64 count, maxCount;

	void Init(Arena* arena, i64 maxCount_ = ARRAY_DEFAULT_MAX_COUNT) {
		items = arena->PushArray<T>(maxCount_);
		maxCount = maxCount_;
		count = 0;
	}

	// инициализировать значениями из другого массива
	void Init(Arena* arena, dynamic_array<T>* copyFrom) {
		*this = {0};
		PushArray(arena, copyFrom);
	}

	T& operator[](i64 index) { return items[index]; }

	const T& operator[](i64 index) const { return items[index]; }

	void Reserve(Arena* arena, i64 newCount) {
		if (newCount > this->maxCount) {
			i64 newMaxCount = newCount * ARRAY_GROWTH_FACTOR;
			this->items = arena->ArenaReallocArray<T>(this->items, this->count, newMaxCount);
			this->maxCount = newMaxCount;
		}
	}

	void Push(Arena* arena, const T& item) {
		i64 newCount = this->count + 1;
		this->Reserve(arena, newCount);
		this->items[this->count] = item;
		this->count = newCount;
	}

	void PushArray(Arena* arena, dynamic_array<T>* other) {
		this->Reserve(arena, this->count + other->count);
		for (i64 i = 0; i < other->count; i++) {
			this->Push(arena, other->items[i]);
		}
	}

	void Insert(Arena* arena, const T& item, i64 index) {
		if (index > this->count || index < 0)
			return;

		i64 newCount = this->count + 1;
		this->Reserve(arena, newCount);

		for (i64 i = this->count; i > index; i--) {
			this->items[i] = this->items[i - 1];
		}
		this->items[index] = item;
		this->count = newCount;
	}

	void RemoveAt(i64 index) {
		if (index >= this->count)
			return;

		for (i64 i = index; i < this->count - 1; i++) {
			this->items[i] = this->items[i + 1];
		}

		this->count--;
	}

	// меняет порядок элементов при удалении
	void RemoveAtFast(i64 index) {
		if (index >= this->count)
			return;

		// last
		if (index == this->count - 1) {
			this->count--;
			return;
		}

		this->items[index] = this->items[this->count - 1];
		this->count--;
	}

	// медленная сортировка для отладки
	void Sort(bool (*ALessThanB)(T*, T*), bool descending = false) {
		for (i64 i = 0; i < count; i++) {
			for (i64 j = 0; j < count - 1 - i; j++) {
				if ((!descending && ALessThanB(&items[j + 1], &items[j])) ||
						(descending && !ALessThanB(&items[j + 1], &items[j]))) {
					T temp = items[j + 1];
					items[j + 1] = items[j];
					items[j] = temp;
				}
			}
		}
	}
};

#endif
