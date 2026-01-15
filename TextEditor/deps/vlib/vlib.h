#pragma once
/*
Использование:
	1. #inlucde "vlib.h"
	2. #include "vlib.cpp" (только в одном translation unit) или добавитть vlib.cpp к компиляции

	3. VlibInit(); - инициализация
	4. VlibDestroy(); - завершение работы

Профайлер:
	Определить '#define USE_PROFILER', чтобы использовать профайлер

TODO:
	Переписать хеш-таблицы и перенести на новые арены
	Переименовать структуры (убрать snake_case)?
	Threads, Lanes, Bariers, Atomics, ...
*/

#include "base/arena.h"
#include "base/array.h"
#include "base/string.h"
#include "base/thread_context.h"
#include "general.h"
#include "platform/platform.h"
#include "tools/profiler.h"

void VlibInit(i64 arenasCommit = ARENA_COMMIT_SIZE, i64 arenasReserve = ARENA_DEFAULT_RESERVE);
void VlibDestroy();

// #define HT_VERSION 2

// #if HT_VERSION == 1
// #include "base/hash_table.h"
// #else if HT_VERSION == 2
// #include "base/hash_table_v2.h"
// #endif
