/*
Использование:
	В одном из .cpp файлов включить vlib.cpp или добавить его к компиляции. Не добавлять к компиляции остальные .cpp файлы библиотеки.

Профайлер:
	Определить '#define USE_PROFILER', чтобы использовать профайлер

TODO:
	Переписать хеш-таблицы и перенести на новые арены
	Переименовать структуры (убрать snake_case)?
	Threads, Lanes, Bariers, Atomics, ...
	Возможность отключать работу или компиляцию профайлера
*/

#include "base/arena.h"
#include "base/array.h"
#include "base/string.h"
#include "base/thread_context.h"
#include "general.h"
#include "platform/platform.h"
#include "tools/profiler.h"

// #define HT_VERSION 2

// #if HT_VERSION == 1
// #include "base/hash_table.h"
// #else if HT_VERSION == 2
// #include "base/hash_table_v2.h"
// #endif
