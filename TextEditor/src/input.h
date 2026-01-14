#pragma once

void platform_Print(const char* msg);

// UTF-8 code point
struct code_point {
	union {
		u32 v;
		u8 bytes[4];
	};
};

#define CodePoint(value) {.v = value}

s32 GetLength(code_point p) {
	for (s32 i = 3; i >= 0; i--) {
		u8 byte = p.bytes[i];
		if (byte == 0)
			continue;

		if ((byte & 0b11110000) == 0b11110000)
			return 4;
		if ((byte & 0b11100000) == 0b11100000)
			return 3;
		if ((byte & 0b11000000) == 0b11000000)
			return 2;
		if ((byte & 0b10000000) == 0)
			return 1;	 // ascii
	}

	platform_Print("Invalid code point\n");
	return 0;
}

enum te_Key {
	Key_None = 0,

	Key_0 = '0',
	Key_1 = '1',
	Key_2 = '2',
	Key_3 = '3',
	Key_4 = '4',
	Key_5 = '5',
	Key_6 = '6',
	Key_7 = '7',
	Key_8 = '8',
	Key_9 = '9',

	Key_A = 'A',
	Key_B = 'B',
	Key_C = 'C',
	Key_D = 'D',
	Key_E = 'E',
	Key_F = 'F',
	Key_G = 'G',
	Key_H = 'H',
	Key_I = 'I',
	Key_J = 'J',
	Key_K = 'K',
	Key_L = 'L',
	Key_M = 'M',
	Key_N = 'N',
	Key_O = 'O',
	Key_P = 'P',
	Key_Q = 'Q',
	Key_R = 'R',
	Key_S = 'S',
	Key_T = 'T',
	Key_U = 'U',
	Key_V = 'V',
	Key_W = 'W',
	Key_X = 'X',
	Key_Y = 'Y',
	Key_Z = 'Z',

	Key_ArrowLeft,
	Key_ArrowRight,
	Key_ArrowUp,
	Key_ArrowDown,
	Key_Shift,
	Key_Ctrl,
	Key_Alt,
	Key_Esc,
	Key_Enter,

	Key_WheelUp,
	Key_WheelDown,

	Key_COUNT
};

struct button_state {
	s32 halfTransitionsCount;
	bool isDown;
};

struct program_input {
	button_state keys[Key_COUNT];
};

void ProcessButtonInput(button_state* oldState, button_state* newState, bool isDown) {
	newState->isDown = isDown;
	newState->halfTransitionsCount = oldState->isDown != newState->isDown ? 1 : 0;
}

// активируется в один раз когда кнопка нажата
#define IsButtonDown(b) ((b).isDown && (b).halfTransitionsCount > 0)
// активируется в один раз когда кнопка отпущена
#define IsButtonReleased(b) (!(b).isDown && (b).halfTransitionsCount > 0)
// активирован, пока кнопка не отпущена
#define IsButtonPushed(b) ((b).isDown && (b).halfTransitionsCount == 0)

const char* GetKeyString(te_Key key) {
	switch (key) {
		case Key_None:
			return "None";

		case Key_0:
			return "0";
		case Key_1:
			return "1";
		case Key_2:
			return "2";
		case Key_3:
			return "3";
		case Key_4:
			return "4";
		case Key_5:
			return "5";
		case Key_6:
			return "6";
		case Key_7:
			return "7";
		case Key_8:
			return "8";
		case Key_9:
			return "9";

		case Key_A:
			return "A";
		case Key_B:
			return "B";
		case Key_C:
			return "C";
		case Key_D:
			return "D";
		case Key_E:
			return "E";
		case Key_F:
			return "F";
		case Key_G:
			return "G";
		case Key_H:
			return "H";
		case Key_I:
			return "I";
		case Key_J:
			return "J";
		case Key_K:
			return "K";
		case Key_L:
			return "L";
		case Key_M:
			return "M";
		case Key_N:
			return "N";
		case Key_O:
			return "O";
		case Key_P:
			return "P";
		case Key_Q:
			return "Q";
		case Key_R:
			return "R";
		case Key_S:
			return "S";
		case Key_T:
			return "T";
		case Key_U:
			return "U";
		case Key_V:
			return "V";
		case Key_W:
			return "W";
		case Key_X:
			return "X";
		case Key_Y:
			return "Y";
		case Key_Z:
			return "Z";

		case Key_ArrowLeft:
			return "Arrow Left";
		case Key_ArrowRight:
			return "Arrow Right";
		case Key_ArrowUp:
			return "Arrow Up";
		case Key_ArrowDown:
			return "Arrow Down";

		case Key_Shift:
			return "Shift";
		case Key_Ctrl:
			return "Ctrl";
		case Key_Alt:
			return "Alt";

		case Key_Esc:
			return "Esc";
		case Key_Enter:
			return "Enter";

		case Key_WheelUp:
			return "Wheel Up";
		case Key_WheelDown:
			return "Wheel Down";

		default:
			return "[NOT AVALIABLE]";
	}
}

//
// Commands & Hotkeys
//

// TODO:
// окно команд:
// * ремаппинг хоткеев

struct command {
	const char* label;
	bool ctrl, shift, alt;
	te_Key key;
};

enum command_type {
	Command_None,

	Command_ShowCommandPalette,
	Command_OpenSettings,

	Command_New,
	Command_Save,
	Command_Copy,
	Command_Paste,
	Command_SelectAll,

	Command_IncreaseFontSize,
	Command_DecreaseFontSize,

	Command_COUNT,
};

command g_hotkeyMappings[Command_COUNT] = {
		{.label = "None"},

		{.label = "Show commands", .ctrl = 1, .shift = 1, .alt = 0, .key = Key_P},
		{.label = "Open settings"},

		{.label = "New", .ctrl = 1, .shift = 0, .alt = 0, .key = Key_N},
		{.label = "Save", .ctrl = 1, .shift = 0, .alt = 0, .key = Key_S},
		{.label = "Copy", .ctrl = 1, .shift = 0, .alt = 0, .key = Key_C},
		{.label = "Paste", .ctrl = 1, .shift = 0, .alt = 0, .key = Key_V},
		{.label = "Select all", .ctrl = 1, .shift = 0, .alt = 0, .key = Key_A},

		{.label = "Increase font size", .ctrl = 1, .shift = 0, .alt = 0, .key = Key_WheelUp},
		{.label = "Decrease font size", .ctrl = 1, .shift = 0, .alt = 0, .key = Key_WheelDown},
};