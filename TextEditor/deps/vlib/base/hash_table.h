#pragma once

#if 0

#include <assert.h>
#include <intrin.h>
#include <string.h>
#include "../general.h"
#include "string.h"

#define HT_GROUP_SIZE 16
#define HT_GROWTH_FACTOR 2
#define HT_OVERFLOW_THRESHOLD (7.0f / 8.0f)

#define ClearBit(v, index) ((v) & ~(1 << (index)))

struct control_word {
	u8 slots[HT_GROUP_SIZE];
};

u8 GetSlotMetadata(control_word controlWord, i64 slotIndex);
void SetSlotMetadata(control_word* controlWord, i64 slotIndex, u8 value);

// если старший бит в control_word слоте равен 1: служебная метка (EMPTY или DELETED слот)
// если старший бит равен 0: хранит 7-битный H2 хэш

#define H1(hash) (hash >> 7)
#define H2(hash) (hash & 0b1111111)

#define SLOT_EMPTY (0b10000000)
#define SLOT_DELETED (0b01111111)

bool IsSlotEmpty(u8 controlWordByte);
bool IsSlotDeleted(u8 controlWordByte);
bool IsSlotFull(u8 controlWordByte);

#ifdef VLIB_IMPL

u8 GetSlotMetadata(control_word controlWord, i64 slotIndex) {
	assert(slotIndex >= 0 && slotIndex < HT_GROUP_SIZE);
	return controlWord.slots[slotIndex];
	// return (controlWord >> (slotIndex * 8)) & 0xFF;
}

void SetSlotMetadata(control_word* controlWord, i64 slotIndex, u8 value) {
	assert(slotIndex >= 0 && slotIndex < HT_GROUP_SIZE);
	// ((u8*)controlWord)[slotIndex] = value;
	controlWord->slots[slotIndex] = value;
}

bool IsSlotEmpty(u8 controlWordByte) {
	return controlWordByte == SLOT_EMPTY;
}

bool IsSlotDeleted(u8 controlWordByte) {
	return controlWordByte == SLOT_DELETED;
}

bool IsSlotFull(u8 controlWordByte) {
	return (!IsSlotEmpty(controlWordByte) && !IsSlotDeleted(controlWordByte));
}

#endif

template <typename K, typename V>
struct ht_item {
	K key;
	V value;
};

template <typename K, typename V>
ht_item<K, V> HTItem(const K& key, const V& value) {
	return {.key = key, .value = value};
}

template <typename K, typename V>
struct ht_group {
	ht_item<K, V> slots[HT_GROUP_SIZE];
};

template <typename K, typename V>
struct hash_table {
	ht_group<K, V>* groups;	 // TODO: отделить ключи и значения в отдельные массивы
	control_word* controlWords;
	i64 count, capacity;	// slots count, capacity

	void Init(Arena* arena, i64 capacity_ = 32) {
		capacity = RoundToMultiple(RoundToPowerOfTwo(capacity_), HT_GROUP_SIZE);
		groups = arena->PushZeroArray<ht_group<K, V>>(GroupsCapacity());
		controlWords = arena->PushArray<control_word>(GroupsCapacity());
		memset(controlWords, SLOT_EMPTY, capacity);	 // устанавливаем каждый байт как SLOT_EMPTY

		assert(IsPowerOfTwo(capacity));
	}

	i64 GroupsCapacity() {
		return capacity / HT_GROUP_SIZE;
	}

	ht_item<K, V>* GetSlot(i64 groupIndex, i64 slotIndex) {
		assert(groupIndex >= 0 && groupIndex < this->GroupsCapacity());
		assert(slotIndex >= 0 && slotIndex < HT_GROUP_SIZE);
		return &this->groups[groupIndex].slots[slotIndex];
	};

	V* GetValue(const K& key) {
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
				ht_item<K, V>* slot = GetSlot(groupIndex, index);
				if (slot->key == key) {
					return &slot->value;
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
		// расширение табилцы при достижении порога заполнения
		if (this->count > (this->capacity * HT_OVERFLOW_THRESHOLD)) {
			i64 newCapacity = this->capacity * HT_GROWTH_FACTOR;
			hash_table<K, V> newht;
			newht.Init(arena, newCapacity);

			ht_iterator<K, V> iter = this->GetIterator();
			while (iter.GetPair()) {
				ht_item<K, V>* item = iter.GetPair();
				newht.PushValue(arena, item->key, item->value);
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
					auto slot = GetSlot(groupIndex, index);
					// ключ уже есть, обновляем значение
					if (slot->key == key) {
						slot->value = value;
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
					auto slot = GetSlot(groupIndex, index);
					if (this->count < this->capacity) {
						*slot = HTItem(key, value);
						SetSlotMetadata(&this->controlWords[groupIndex], index, fingerprint);
						this->count++;
						return;
					} else {
						LogError("Hash table insert: item array overflow");
						return;
					}
				}
			}

			lookupCount += HT_GROUP_SIZE;
			if (lookupCount >= this->capacity) {
				LogError("Hash table: item array overflow");
				return;
			}

			// свободный слот не найден, ищем в следующей группе
			groupIndex = Mod(groupIndex + 1, groupsCapacity);
		}
	}

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

		ht_item<K, V>* GetPair() {
			if (!IsValid())
				return NULL;
			return &ht->groups[this->groupIndex].slots[this->slotIndex];
		}

		K* GetKey() {
			auto pair = GetPair();
			return pair != NULL ? &pair->key : NULL;
		}

		V* GetValue() {
			auto pair = GetPair();
			return pair != NULL ? &pair->value : NULL;
		}
	};

	ht_iterator<K, V> GetIterator() {
		if (this->count <= 0) {
			return {0};	 // пустая таблица
		}

		for (i64 i = 0; i < this->GroupsCapacity(); i++) {
			control_word controlWord = this->controlWords[i];
			for (i64 j = 0; j < HT_GROUP_SIZE; j++) {
				u8 slotMetadata = GetSlotMetadata(controlWord, j);
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
