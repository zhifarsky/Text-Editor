#pragma once

#if 0

#include <assert.h>
#include <intrin.h>
#include <string.h>
#include "../general.h"
#include "../tools/profiler.h"

#define HT_GROUP_SIZE 16
#define HT_GROWTH_FACTOR 2
#define HT_OVERFLOW_THRESHOLD (7.0f / 8.0f)

#define ClearBit(v, index) ((v) & ~(1 << (index)))
#define GetBit(v, index) ((v) & (1 << (index)))

struct control_word {
	u8 slots[HT_GROUP_SIZE];
};

// если старший бит в control_word слоте равен 1: служебная метка (EMPTY или DELETED слот)
// если старший бит равен 0: хранит 7-битный H2 хэш

#define H1(hash) (hash >> 7)
#define H2(hash) (hash & 0b1111111)

#define SLOT_EMPTY (0b10000000)
#define SLOT_DELETED (0b11111110)

#define IsSlotEmpty(controlWordByte) ((controlWordByte) == SLOT_EMPTY)
#define IsSlotDeleted(controlWordByte) ((controlWordByte) == SLOT_DELETED)
#define IsSlotFull(controlWordByte) (GetBit((controlWordByte), 7) == 0)

#ifdef VLIB_IMPL

#endif

template <typename K>
struct ht_key_group {
	K slots[HT_GROUP_SIZE];
};

template <typename V>
struct ht_value_group {
	V slots[HT_GROUP_SIZE];
};

template <typename K, typename V>
struct hash_table {
	ht_key_group<K>* keyGroups;
	ht_value_group<V>* valueGroups;
	control_word* controlWords;
	i64 count, capacity;	// slots count, capacity

	void Init(Arena* arena, i64 capacity_ = 32) {
		capacity = RoundToMultiple(RoundToPowerOfTwo(capacity_), HT_GROUP_SIZE);
		keyGroups = arena->PushZeroArray<ht_key_group<K>>(GroupsCapacity());
		valueGroups = arena->PushZeroArray<ht_value_group<V>>(GroupsCapacity());
		controlWords = arena->PushArray<control_word>(GroupsCapacity());
		memset(controlWords, SLOT_EMPTY, capacity);	 // устанавливаем каждый байт как SLOT_EMPTY

		assert(IsPowerOfTwo(capacity));
	}

	i64 GroupsCapacity() {
		return capacity / HT_GROUP_SIZE;
	}

	K* _GetKey(i64 groupIndex, i64 slotIndex) {
		assert(groupIndex >= 0 && groupIndex < this->GroupsCapacity());
		assert(slotIndex >= 0 && slotIndex < HT_GROUP_SIZE);

		return &this->keyGroups[groupIndex].slots[slotIndex];
	}

	V* _GetValue(i64 groupIndex, i64 slotIndex) {
		assert(groupIndex >= 0 && groupIndex < this->GroupsCapacity());
		assert(slotIndex >= 0 && slotIndex < HT_GROUP_SIZE);

		return &this->valueGroups[groupIndex].slots[slotIndex];
	}

	V* GetValue(const K& key) {
		ProfileFunction();

		u64 hash = GetHash(key);
		i64 groupsCapacity = GroupsCapacity();

		i64 groupIndex = Mod(H1(hash), groupsCapacity);
		u8 fingerprint = H2(hash);

		i64 lookupCount = 0;

		__m128i bFingerprint = _mm_set1_epi8(fingerprint);
		__m128i bEmpty = _mm_set1_epi8(SLOT_EMPTY);

		while (true) {
			// __m128i a = _mm_set_epi64x(0, control)
			__m128i a = _mm_loadu_epi8(&this->controlWords[groupIndex]);

			u32 cmpFingerprint = _mm_movemask_epi8(_mm_cmpeq_epi8(a, bFingerprint));
			unsigned long index;
			while (_BitScanForward(&index, cmpFingerprint)) {
				if (*_GetKey(groupIndex, index) == key) {
					return _GetValue(groupIndex, index);
				}

				cmpFingerprint = ClearBit(cmpFingerprint, index);
			}
			// если в группе не найден ключ и есть пусте слоты, ключа в таблице нет
			u32 cmpEmpty = _mm_movemask_epi8(_mm_cmpeq_epi8(a, bEmpty));
			if (cmpEmpty != 0) {
				return NULL;
			}

			lookupCount += HT_GROUP_SIZE;
			// если все слоты просмотрены
			if (lookupCount >= this->count) {
				return NULL;
			}

			// ключ не найден. ищем в следующей группе
			groupIndex = Mod(groupIndex + 1, groupsCapacity);
		}
	}

	bool ContainsValue(const K& key) {
		return this->GetValue(key);
	}

	void PushValue(Arena* arena, const K& key, const V& value) {
		ProfileFunction();

		// расширение таблицы при достижении порога заполнения
		if (this->count > (this->capacity * HT_OVERFLOW_THRESHOLD)) {
			i64 newCapacity = this->capacity * HT_GROWTH_FACTOR;
			hash_table<K, V> newht;
			newht.Init(arena, newCapacity);

			ht_iterator<K, V> iter = this->GetIterator();
			while (iter.IsValid()) {
				newht.PushValue(arena, *iter.GetKey(), *iter.GetValue());
				iter.Next();
			}

			memcpy(this, &newht, sizeof(newht));
		}

		u64 hash = GetHash(key);
		i64 groupsCapacity = GroupsCapacity();

		i64 groupIndex = Mod(H1(hash), groupsCapacity);
		u8 fingerprint = H2(hash);

		i64 lookupCount = 0;

		__m128i bFingerprint = _mm_set1_epi8(fingerprint);
		__m128i bEmpty = _mm_set1_epi8(SLOT_EMPTY);

		// поиск свободного слота в группе
		while (true) {
			// TODO: проверка на SLOT_DELETED
			__m128i a = _mm_loadu_epi8(&this->controlWords[groupIndex]);

			// ищем есть ли этот ключ в таблице
			{
				u32 cmpRes = _mm_movemask_epi8(_mm_cmpeq_epi8(a, bFingerprint));
				unsigned long index;
				// если есть совпадение по h2, проверяем ключи на совпадение
				while (_BitScanForward(&index, cmpRes)) {
					// если ключ совпадает, обновляем значение
					if (*_GetKey(groupIndex, index) == key) {
						*_GetValue(groupIndex, index) = value;
						return;
					}

					cmpRes = ClearBit(cmpRes, index);
				}
			}

			// ключа нет. ищем пустую ячейку для вставки
			{
				u32 cmpRes = _mm_movemask_epi8(_mm_cmpeq_epi8(a, bEmpty));
				unsigned long index;
				// если найден пустой слот, вставляем ключ и значение
				if (_BitScanForward(&index, cmpRes)) {
					if (count < capacity) {
						*_GetKey(groupIndex, index) = key;
						*_GetValue(groupIndex, index) = value;
						controlWords[groupIndex].slots[index] = fingerprint;
						count++;
						return;
					} else {
						LogError("item array overflow");
						return;
					}
				}
			}

			lookupCount += HT_GROUP_SIZE;
			if (lookupCount >= this->capacity) {
				LogError("item array overflow");
				return;
			}

			// свободный слот не найден, ищем в следующей группе
			groupIndex = Mod(groupIndex + 1, groupsCapacity);
		}
	}

	// TODO: сделать
	// V& operator[](const K& key) {
	// }

	template <typename K, typename V>
	struct ht_iterator {
		hash_table<K, V>* ht;
		i64 groupIndex, slotIndex;

		void Next() {
			if (!ht) {
				return;
			}

			for (i64 i = groupIndex; i < ht->GroupsCapacity(); i++) {
				i64 j = i == groupIndex ? slotIndex + 1 : 0;
				__m128i a = _mm_loadu_epi8(&ht->controlWords[i]);
				__m128i bEmpty = _mm_set1_epi8(SLOT_EMPTY);
				u16 cmpFull = ~_mm_movemask_epi8(_mm_cmpeq_epi8(a, bEmpty));	// TDOO: изменить, если будут появятся DELETED слоты

				// нас интересуют только биты, начинающиеся с индекса j
				// зануляем j бит в начале
				cmpFull = cmpFull & (~0U << j);

				unsigned long index;
				// если найден не пустой слот
				if (_BitScanForward(&index, cmpFull)) {
					this->groupIndex = i;
					this->slotIndex = index;
					return;
				}
			}

			// дошел до конца
			ht = NULL;
		}

		bool IsValid() {
			return ht != NULL;
		}

		K* GetKey() {
			if (!IsValid())
				return NULL;
			return ht->_GetKey(groupIndex, slotIndex);
		}

		V* GetValue() {
			if (!IsValid())
				return NULL;
			return ht->_GetValue(groupIndex, slotIndex);
		}
	};

	ht_iterator<K, V> GetIterator() {
		if (this->count <= 0) {
			return {0};	 // пустая таблица
		}

		// ищем первый не пустой слот
		for (i64 i = 0; i < this->GroupsCapacity(); i++) {
			control_word controlWord = this->controlWords[i];
			for (i64 j = 0; j < HT_GROUP_SIZE; j++) {
				u8 slotMetadata = controlWord.slots[j];
				if (!IsSlotEmpty(slotMetadata)) {
					ht_iterator<K, V> iterator;
					iterator.ht = this;
					iterator.groupIndex = i;
					iterator.slotIndex = j;
					return iterator;
				}
			}
		}

		return {0};
	}
};

#endif
