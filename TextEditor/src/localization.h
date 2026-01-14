#pragma once
#include "editor.h"

enum localization_language {
	Lang_ENG,
	Lang_RUS,
	Lang_COUNT
};

const char* g_languageStrings[Lang_COUNT] = {"English", "Русский"};

struct localization_strings {
	const char
			*language,
			*loadFile, *saveFile, *newFile,
			*settings, *fontSize;
};

struct _localization_strings {
	localization_language lang;
	union {
		localization_strings strings;
		const char* stringsArray[sizeof(localization_strings) / sizeof(char*)];
	};
};

// TODO: парсить из файла?
// TODO: подставлять идентификаторы ###... автоматически
_localization_strings g_localizations[Lang_COUNT] = {
		{.lang = Lang_ENG,
		 .strings = {
				 .language = "Language###Language",
				 .loadFile = "Open###Open",
				 .saveFile = "Save###Save",
				 .newFile = "New###New",
				 .settings = "Settings###Settings",
				 .fontSize = "Font size###FontSize"}},
		{
				.lang = Lang_RUS,
				.strings = {.language = "Язык###Language", .loadFile = "Открыть###Open", .saveFile = "Сохранить###Save", .newFile = "Новый###New", .settings = "Настройки###Settings", .fontSize = "Размер шрифта###FontSize"},
		},
};

_localization_strings* g_currentLocalization;

void SetLanguage(localization_language lang) {
	if (g_currentLocalization && g_currentLocalization->lang == lang)
		return;

	auto strings = g_localizations[lang].stringsArray;
	auto stringsDefault = g_localizations[Lang_ENG].stringsArray;

	// если нет перевода строки, меняем на английский перевод
	// если нет английского перевода, выводим "[NOT AVALIABLE]"
	for (size_t i = 0; i < ArrayCount(_localization_strings::stringsArray); i++) {
		if (!strings[i]) {
			if (stringsDefault[i]) {
				strings[i] = stringsDefault[i];
			} else {
				strings[i] = "[NOT AVALIABLE]";
			}
		}
	}

	g_currentLocalization = &g_localizations[lang];
}

const localization_strings& GetStrings() {
	return g_currentLocalization->strings;
}
